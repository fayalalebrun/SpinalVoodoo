import memory;
import striped_memory;

fn fnv_word(hash: u32, value: u32) -> u32 {
    let hash = (hash ^ (value[0+:u8] as u32)) * u32:0x01000193;
    let hash = (hash ^ (value[8+:u8] as u32)) * u32:0x01000193;
    let hash = (hash ^ (value[16+:u8] as u32)) * u32:0x01000193;
    (hash ^ (value[24+:u8] as u32)) * u32:0x01000193
}

fn block_hash
    (hash: u32, v0: u32, v1: u32, v2: u32, v3: u32, v4: u32, v5: u32, v6: u32, v7: u32) -> u32 {
    fnv_word(
        fnv_word(
            fnv_word(fnv_word(fnv_word(fnv_word(fnv_word(fnv_word(hash, v0), v1), v2), v3), v4), v5),
            v6), v7)
}

fn block_command(block: memory::MemoryBlockWrite, bank: u3, offset: u2) -> memory::MemoryCommand {
    let index = (bank as u6) + ((offset as u6) << u6:3);
    memory::MemoryCommand {
        write: u1:1,
        address: (block.address + (index as u22)) >> u8:3,
        data: block.data[index],
        mask: block.masks[index],
    }
}

pub proc banked_word_memory<SIZE: u32> {
    command_r: chan<memory::MemoryCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    block_r: chan<memory::MemoryBlockWrite> in;
    block_response_s: chan<memory::MemoryBlockResponse> out;
    c0s: chan<memory::MemoryCommand> out;
    c0r: chan<memory::MemoryResponse> in;
    c1s: chan<memory::MemoryCommand> out;
    c1r: chan<memory::MemoryResponse> in;
    c2s: chan<memory::MemoryCommand> out;
    c2r: chan<memory::MemoryResponse> in;
    c3s: chan<memory::MemoryCommand> out;
    c3r: chan<memory::MemoryResponse> in;
    c4s: chan<memory::MemoryCommand> out;
    c4r: chan<memory::MemoryResponse> in;
    c5s: chan<memory::MemoryCommand> out;
    c5r: chan<memory::MemoryResponse> in;
    c6s: chan<memory::MemoryCommand> out;
    c6r: chan<memory::MemoryResponse> in;
    c7s: chan<memory::MemoryCommand> out;
    c7r: chan<memory::MemoryResponse> in;

    config(
        command_r: chan<memory::MemoryCommand> in,
        response_s: chan<memory::MemoryResponse> out,
        block_r: chan<memory::MemoryBlockWrite> in,
        block_response_s: chan<memory::MemoryBlockResponse> out,
    ) {
        let (c0s, c0c) = chan<memory::MemoryCommand>("command0");
        let (r0c, c0r) = chan<memory::MemoryResponse>("response0");
        let (c1s, c1c) = chan<memory::MemoryCommand>("command1");
        let (r1c, c1r) = chan<memory::MemoryResponse>("response1");
        let (c2s, c2c) = chan<memory::MemoryCommand>("command2");
        let (r2c, c2r) = chan<memory::MemoryResponse>("response2");
        let (c3s, c3c) = chan<memory::MemoryCommand>("command3");
        let (r3c, c3r) = chan<memory::MemoryResponse>("response3");
        let (c4s, c4c) = chan<memory::MemoryCommand>("command4");
        let (r4c, c4r) = chan<memory::MemoryResponse>("response4");
        let (c5s, c5c) = chan<memory::MemoryCommand>("command5");
        let (r5c, c5r) = chan<memory::MemoryResponse>("response5");
        let (c6s, c6c) = chan<memory::MemoryCommand>("command6");
        let (r6c, c6r) = chan<memory::MemoryResponse>("response6");
        let (c7s, c7c) = chan<memory::MemoryCommand>("command7");
        let (r7c, c7r) = chan<memory::MemoryResponse>("response7");
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c0c, r0c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c1c, r1c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c2c, r2c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c3c, r3c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c4c, r4c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c5c, r5c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c6c, r6c);
        spawn striped_memory::striped_word_memory<{(SIZE + u32:7) / u32:8}>(c7c, r7c);
        (
            command_r, response_s, block_r, block_response_s, c0s, c0r, c1s, c1r, c2s, c2r, c3s,
            c3r, c4s, c4r, c5s, c5r, c6s, c6r, c7s, c7r,
        )
    }

    init { () }

    next(state: ()) {
        let default_command =
            memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 };
        let default_block = memory::MemoryBlockWrite {
            address: u22:0, data: u32[32]:[u32:0, ...], masks: u4[32]:[u4:0, ...],
        };
        let (tok, command, command_valid) = recv_non_blocking(join(), command_r, default_command);
        let (tok, write_block, write_block_valid) = recv_non_blocking(tok, block_r, default_block);
        let bank = command.address[0+:u3];
        let hash_block = command.write == u1:0 && command.mask == u4:0xf;
        let scalar_valid = command_valid && !write_block_valid;
        let local = memory::MemoryCommand { address: command.address >> u8:3, ..command };
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c0s, write_block_valid, block_command(write_block, u3:0, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c1s, write_block_valid, block_command(write_block, u3:1, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c2s, write_block_valid, block_command(write_block, u3:2, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c3s, write_block_valid, block_command(write_block, u3:3, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c4s, write_block_valid, block_command(write_block, u3:4, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c5s, write_block_valid, block_command(write_block, u3:5, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c6s, write_block_valid, block_command(write_block, u3:6, i as u2))
        }(tok);
        let tok = unroll_for! (i, tok): (u3, token) in u3:0..u3:4 {
            send_if(tok, c7s, write_block_valid, block_command(write_block, u3:7, i as u2))
        }(tok);
        let tok = send_if(tok, c0s, scalar_valid && (hash_block || bank == u3:0), local);
        let tok = send_if(tok, c1s, scalar_valid && (hash_block || bank == u3:1), local);
        let tok = send_if(tok, c2s, scalar_valid && (hash_block || bank == u3:2), local);
        let tok = send_if(tok, c3s, scalar_valid && (hash_block || bank == u3:3), local);
        let tok = send_if(tok, c4s, scalar_valid && (hash_block || bank == u3:4), local);
        let tok = send_if(tok, c5s, scalar_valid && (hash_block || bank == u3:5), local);
        let tok = send_if(tok, c6s, scalar_valid && (hash_block || bank == u3:6), local);
        let tok = send_if(tok, c7s, scalar_valid && (hash_block || bank == u3:7), local);
        let default = memory::MemoryResponse { data: u32:0, success: u1:0 };
        let (tok, r0) = recv_if(tok, c0r, scalar_valid && (hash_block || bank == u3:0), default);
        let (tok, r1) = recv_if(tok, c1r, scalar_valid && (hash_block || bank == u3:1), default);
        let (tok, r2) = recv_if(tok, c2r, scalar_valid && (hash_block || bank == u3:2), default);
        let (tok, r3) = recv_if(tok, c3r, scalar_valid && (hash_block || bank == u3:3), default);
        let (tok, r4) = recv_if(tok, c4r, scalar_valid && (hash_block || bank == u3:4), default);
        let (tok, r5) = recv_if(tok, c5r, scalar_valid && (hash_block || bank == u3:5), default);
        let (tok, r6) = recv_if(tok, c6r, scalar_valid && (hash_block || bank == u3:6), default);
        let (tok, r7) = recv_if(tok, c7r, scalar_valid && (hash_block || bank == u3:7), default);
        let selected = match bank {
            u3:0 => r0, u3:1 => r1, u3:2 => r2, u3:3 => r3,
            u3:4 => r4, u3:5 => r5, u3:6 => r6, _ => r7,
        };
        let response = if hash_block {
            memory::MemoryResponse {
                data: block_hash(
                    command.data, r0.data, r1.data, r2.data, r3.data, r4.data, r5.data, r6.data,
                    r7.data),
                success: u1:1,
            }
        } else { selected };
        let tok = send_if(tok, response_s, scalar_valid, response);
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c0r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, u1:1));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c1r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c2r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c3r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c4r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c5r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c6r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let (tok, block_success) = unroll_for! (_, accumulator):
            (u3, (token, u1)) in u3:0..u3:4 {
            let (tok, success) = accumulator;
            let (tok, child) = recv_if(tok, c7r, write_block_valid, default);
            (tok, success && child.success)
        }((tok, block_success));
        let tok = send_if(
            tok, block_response_s, write_block_valid,
            memory::MemoryBlockResponse { success: block_success });
        state
    }
}

#[test]
fn hashes_interleaved_block() {
    assert_eq(
        block_hash(u32:0x811c9dc5, u32:0x04030201, u32:0, u32:0, u32:0, u32:0, u32:0, u32:0, u32:0),
        u32:0xa2f8452d);
}

#[test_proc]
proc banked_word_memory_test {
    command_s: chan<memory::MemoryCommand> out;
    response_r: chan<memory::MemoryResponse> in;
    block_s: chan<memory::MemoryBlockWrite> out;
    block_response_r: chan<memory::MemoryBlockResponse> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (command_s, command_r) = chan<memory::MemoryCommand>("command");
        let (response_s, response_r) = chan<memory::MemoryResponse>("response");
        let (block_s, block_r) = chan<memory::MemoryBlockWrite>("block");
        let (block_response_s, block_response_r) =
            chan<memory::MemoryBlockResponse>("block_response");
        spawn banked_word_memory<u32:64>(
            command_r, response_s, block_r, block_response_s);
        (command_s, response_r, block_s, block_response_r, terminator)
    }

    init { () }

    next(state: ()) {
        let block_data = update(
            update(u32[32]:[u32:0, ...], u32:0, u32:0x11111111),
            u32:31, u32:0xeeeeeeee);
        let tok = send(
            join(), block_s,
            memory::MemoryBlockWrite {
                address: u22:0,
                data: block_data,
                masks: update(u4[32]:[u4:0xf, ...], u32:0, u4:0x5),
            });
        let (tok, block_response) = recv(tok, block_response_r);
        assert_eq(block_response, memory::MemoryBlockResponse { success: u1:1 });
        let tok = send(
            tok, command_s,
            memory::MemoryCommand { write: u1:0, address: u22:31, data: u32:0, mask: u4:0 });
        let (tok, response) = recv(tok, response_r);
        assert_eq(response.data, u32:0xeeeeeeee);
        let tok = send(
            tok, command_s,
            memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 });
        let (tok, response) = recv(tok, response_r);
        assert_eq(response.data, u32:0x00110011);
        let tok = send(
            tok, command_s,
            memory::MemoryCommand {
                write: u1:1,
                address: u22:1,
                data: u32:0x12345678,
                mask: u4:0xf,
            });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            memory::MemoryCommand {
                write: u1:1,
                address: u22:9,
                data: u32:0xabcdef01,
                mask: u4:0xf,
            });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            memory::MemoryCommand { write: u1:0, address: u22:1, data: u32:0, mask: u4:0 });
        let (tok, response) = recv(tok, response_r);
        assert_eq(response.data, u32:0x12345678);
        let tok = send(
            tok, command_s,
            memory::MemoryCommand { write: u1:0, address: u22:9, data: u32:0, mask: u4:0 });
        let (tok, response) = recv(tok, response_r);
        assert_eq(response.data, u32:0xabcdef01);
        send(tok, terminator, true);
    }
}
