from pathlib import Path
import argparse
import hashlib
import json
import struct

import zlib


def region(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError('ELF/profile region outside input')
    return data[offset:offset + size]


def string(data, offset):
    if not 0 <= offset < len(data):
        raise ValueError('string offset outside table')
    end = data.find(b'\0', offset)
    if end < 0:
        raise ValueError('unterminated string')
    return data[offset:end].decode('utf-8')


def uleb(data, offset):
    value = 0
    for shift in range(0, 64, 7):
        byte = region(data, offset, 1)[0]
        if shift == 63 and byte & 127 > 1:
            raise ValueError('oversized ULEB128')
        offset += 1
        value |= (byte & 127) << shift
        if not byte & 128:
            return value, offset
    raise ValueError('oversized ULEB128')


def profile(binary):
    data = Path(binary).read_bytes()
    header = region(data, 0, 52)
    if header[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<HH', header, 16) != (2, 40):
        raise ValueError('expected non-PIE ELF32 little-endian ARM executable')
    shoff, = struct.unpack_from('<I', header, 32)
    entry_size, count, names_index = struct.unpack_from('<HHH', header, 46)
    if entry_size != 40 or not count or names_index >= count:
        raise ValueError('unsupported ELF section table')
    sections = [struct.unpack('<10I', region(data, shoff + i * 40, 40)) for i in range(count)]
    def contents(section):
        return region(data, section[4], section[5])
    section_names = contents(sections[names_index])
    required = ('__llvm_prf_cnts', '__llvm_prf_data', '__llvm_prf_names', '.symtab')
    by_name = {}
    for section in sections:
        name = string(section_names, section[0])
        if name in required and name in by_name:
            raise ValueError('duplicate profile section')
        by_name[name] = section
    if any(name not in by_name for name in required):
        raise ValueError('missing instrumentation profile sections')
    symbols = by_name['.symtab']
    if symbols[9] != 16 or symbols[6] >= count or symbols[5] % 16:
        raise ValueError('invalid symbol table')
    symbol_names = contents(sections[symbols[6]])
    version = None
    for name, address, size, info, other, index in struct.iter_unpack('<IIIBBH', contents(symbols)):
        if string(symbol_names, name) != '__llvm_profile_raw_version':
            continue
        if index >= count or size != 8:
            raise ValueError('invalid profile version symbol')
        section = sections[index]
        version, = struct.unpack('<Q', region(contents(section), address - section[3], 8))
    if version != ((1 << 56) | 10):
        raise ValueError('expected LLVM 21 IR instrumentation version 10')
    names_data = contents(by_name['__llvm_prf_names'])
    names = {}
    offset = 0
    while offset < len(names_data) and any(names_data[offset:]):
        raw_size, offset = uleb(names_data, offset)
        compressed_size, offset = uleb(names_data, offset)
        if not 0 < raw_size <= 64 * 1024 * 1024:
            raise ValueError('invalid profile name block size')
        payload = region(names_data, offset, compressed_size or raw_size)
        offset += len(payload)
        if compressed_size:
            decoder = zlib.decompressobj()
            payload = decoder.decompress(payload, raw_size + 1)
            if len(payload) != raw_size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
                raise ValueError('invalid compressed profile names')
        for raw_name in payload.split(b'\x01'):
            name = raw_name.decode('utf-8')
            if not name or '\n' in name or '\r' in name:
                raise ValueError('invalid profile function name')
            key = int.from_bytes(hashlib.md5(raw_name).digest()[:8], 'little')
            if key in names and names[key] != name:
                raise ValueError('profile name hash collision')
            names[key] = name
    counters = by_name['__llvm_prf_cnts']
    records_section = by_name['__llvm_prf_data']
    records_data = contents(records_section)
    if counters[5] % 8 or len(records_data) % 48:
        raise ValueError('invalid profile record/counter alignment')
    records = {}
    for offset in range(0, len(records_data), 48):
        name_hash, function_hash, relative = struct.unpack_from('<QQi', records_data, offset)
        number, = struct.unpack_from('<I', records_data, offset + 32)
        bitmap, = struct.unpack_from('<I', records_data, offset + 44)
        sites = struct.unpack_from('<3H', records_data, offset + 36)
        if name_hash not in names or not number or bitmap or any(sites):
            raise ValueError('unsupported or malformed function profile record')
        address = (records_section[3] + offset + relative) & 0xffffffff
        start = address - counters[3]
        if start < 0 or start % 8 or start + number * 8 > counters[5]:
            raise ValueError('profile counters outside declared section')
        key = (names[name_hash], function_hash)
        value = (start, number)
        if key in records and records[key] != value:
            raise ValueError('conflicting duplicate function profile')
        records[key] = value
    return {'address': counters[3], 'size': counters[5], 'functions': len(records), 'version': version}, records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('counters', nargs='?', type=Path)
    parser.add_argument('--describe', action='store_true')
    args = parser.parse_args()
    if args.describe == (args.counters is not None):
        parser.error('supply either --describe or a counter snapshot')
    try:
        description, records = profile(args.binary)
        if args.describe:
            print(json.dumps(description))
            return
        counters = args.counters.read_bytes()
        if len(counters) != description['size']:
            raise ValueError('counter snapshot size mismatch')
        print(':ir')
        for (name, function_hash), (offset, number) in records.items():
            values = [value[0] for value in struct.iter_unpack('<Q', region(counters, offset, number * 8))]
            print(name, function_hash, number, *values, sep='\n', end='\n\n')
    except (OSError, ValueError, struct.error, zlib.error) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
