#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "voodoo_trace_format.h"

int main(int argc, char **argv) {
  FILE *f;
  voodoo_trace_header_t hdr;
  voodoo_trace_entry_t ent;
  uint32_t i = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: %s trace.bin\n", argv[0]);
    return 2;
  }

  f = fopen(argv[1], "rb");
  if (!f) {
    perror("fopen");
    return 1;
  }
  if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
    fprintf(stderr, "short header\n");
    fclose(f);
    return 1;
  }
  if (hdr.magic != VOODOO_TRACE_MAGIC) {
    fprintf(stderr, "bad magic\n");
    fclose(f);
    return 1;
  }

  while (fread(&ent, sizeof(ent), 1, f) == 1) {
    if (ent.addr == 0x104 || ent.addr == 0x110 || ent.addr == 0x124 || ent.addr == 0x148 ||
        (ent.addr >= 0x080 && ent.addr <= 0x0fc)) {
      printf("%u ts=%u..%u cmd=%u count=%u addr=0x%03x data=0x%08x\n",
             i,
             ent.timestamp,
             ent.timestamp_end,
             ent.cmd_type,
             ent.count,
             ent.addr,
             ent.data);
    }
    ++i;
  }

  fclose(f);
  return 0;
}
