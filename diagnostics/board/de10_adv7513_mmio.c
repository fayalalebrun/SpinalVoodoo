#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define IC_CON              0x00
#define IC_TAR              0x04
#define IC_DATA_CMD         0x10
#define IC_SS_SCL_HCNT      0x14
#define IC_SS_SCL_LCNT      0x18
#define IC_FS_SCL_HCNT      0x1c
#define IC_FS_SCL_LCNT      0x20
#define IC_INTR_STAT        0x2c
#define IC_RAW_INTR_STAT    0x34
#define IC_CLR_INTR         0x40
#define IC_CLR_TX_ABRT      0x54
#define IC_ENABLE           0x6c
#define IC_STATUS           0x70
#define IC_TXFLR            0x74
#define IC_RXFLR            0x78
#define IC_TX_ABRT_SOURCE   0x80
#define IC_ENABLE_STATUS    0x9c
#define IC_COMP_TYPE        0xfc

#define IC_DATA_CMD_READ    (1u << 8)
#define IC_DATA_CMD_STOP    (1u << 9)
#define IC_DATA_CMD_RESTART (1u << 10)

static const uint32_t i2c_bases[] = {
    0xffc04000u,
    0xffc05000u,
    0xffc06000u,
    0xffc07000u,
};

static const struct {
    uint8_t reg;
    uint8_t val;
} adv_init[] = {
    {0x98, 0x03}, {0xd6, 0xd0}, {0x41, 0x10}, {0x9a, 0x70},
    {0x9c, 0x30}, {0x9d, 0x61}, {0xa2, 0xa4}, {0xa3, 0xa4},
    {0xe0, 0xd0}, {0x35, 0x40}, {0x36, 0xd9}, {0x37, 0x0a},
    {0x38, 0x00}, {0x39, 0x2d}, {0x3a, 0x00}, {0x16, 0x38},
    {0x17, 0x62}, {0x3b, 0x40}, {0x3c, 0x02}, {0x43, 0x7e},
    {0x45, 0x70}, {0x48, 0x08}, {0x49, 0xa8}, {0x40, 0x00},
    {0x4a, 0x80}, {0x4c, 0x00},
    {0x55, 0x10}, {0x56, 0x08}, {0x57, 0x08}, {0x59, 0x00},
    {0x73, 0x01}, {0x94, 0x80}, {0x99, 0x02}, {0x9b, 0x18},
    {0x9f, 0x00}, {0xa1, 0x00}, {0xa4, 0x08}, {0xa5, 0x04},
    {0xa6, 0x00}, {0xa7, 0x00}, {0xa8, 0x00}, {0xa9, 0x00},
    {0xaa, 0x00}, {0xab, 0x40}, {0xaf, 0x06}, {0xb9, 0x00},
    {0xba, 0x60}, {0xbb, 0x00}, {0xde, 0x9c}, {0xe2, 0x01},
    {0xe4, 0x60}, {0xfa, 0x7d}, {0x0a, 0x00}, {0x0b, 0x0e},
    {0x0c, 0x04}, {0x0d, 0x10}, {0x14, 0x02}, {0x15, 0x20},
    {0x01, 0x00}, {0x02, 0x18}, {0x03, 0x00}, {0x07, 0x01},
    {0x08, 0x22}, {0x09, 0x0a},
};

static const struct {
    uint8_t reg;
    uint8_t val;
} adv_terasic_init[] = {
    {0x98, 0x03}, {0x01, 0x00}, {0x02, 0x18}, {0x03, 0x00},
    {0x14, 0x70}, {0x15, 0x20}, {0x16, 0x30}, {0x18, 0x46},
    {0x40, 0x80}, {0x41, 0x10}, {0x49, 0xa8}, {0x55, 0x10},
    {0x56, 0x08}, {0x96, 0xf6}, {0x73, 0x07}, {0x76, 0x1f},
    {0x98, 0x03}, {0x99, 0x02}, {0x9a, 0xe0}, {0x9c, 0x30},
    {0x9d, 0x61}, {0xa2, 0xa4}, {0xa3, 0xa4}, {0xa5, 0x04},
    {0xab, 0x40}, {0xaf, 0x16}, {0xba, 0x60}, {0xd1, 0xff},
    {0xde, 0x10}, {0xe4, 0x60}, {0xfa, 0x7d},
};

static volatile uint32_t *regs;

static uint32_t rd(uint32_t off) { return regs[off / 4]; }
static void wr(uint32_t off, uint32_t val) { regs[off / 4] = val; }

static int wait_until(uint32_t off, uint32_t mask, uint32_t val, int timeout_us) {
    while (timeout_us-- > 0) {
        if ((rd(off) & mask) == val) return 0;
        usleep(1);
    }
    return -1;
}

static int wait_tx_not_full(void) {
    return wait_until(IC_STATUS, 1u << 1, 1u << 1, 5000);
}

static int wait_rx_not_empty(void) {
    return wait_until(IC_STATUS, 1u << 3, 1u << 3, 5000);
}

static int wait_tx_empty(void) {
    return wait_until(IC_TXFLR, 0xffffffffu, 0, 5000);
}

static int wait_idle(void) {
    return wait_until(IC_STATUS, 1u, 0, 5000);
}

static int wait_transfer_done(void) {
    if (wait_tx_empty() < 0) return -1;
    return wait_idle();
}

static void clear_abort(void) {
    (void)rd(IC_CLR_TX_ABRT);
    (void)rd(IC_CLR_INTR);
}

static int has_abort(void) {
    return (rd(IC_RAW_INTR_STAT) & (1u << 6)) != 0;
}

static int controller_init(uint8_t addr) {
    wr(IC_ENABLE, 0);
    if (wait_until(IC_ENABLE_STATUS, 1u, 0, 5000) < 0) return -1;

    wr(IC_CON, (1u << 0) | (1u << 1) | (1u << 5) | (1u << 6));
    wr(IC_TAR, addr);
    wr(IC_SS_SCL_HCNT, 500);
    wr(IC_SS_SCL_LCNT, 600);
    wr(IC_FS_SCL_HCNT, 100);
    wr(IC_FS_SCL_LCNT, 130);
    clear_abort();
    wr(IC_ENABLE, 1);
    if (wait_until(IC_ENABLE_STATUS, 1u, 1u, 5000) < 0) return -1;
    return 0;
}

static int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    if (controller_init(addr) < 0) return -1;
    if (wait_tx_not_full() < 0) return -1;
    wr(IC_DATA_CMD, reg | IC_DATA_CMD_RESTART);
    if (wait_tx_not_full() < 0) return -1;
    wr(IC_DATA_CMD, val | IC_DATA_CMD_STOP);
    if (wait_transfer_done() < 0) return -1;
    if (has_abort()) {
        uint32_t src = rd(IC_TX_ABRT_SOURCE);
        clear_abort();
        errno = EIO;
        return -(int)(src ? src : 1);
    }
    return 0;
}

static int i2c_read_reg(uint8_t addr, uint8_t reg) {
    if (controller_init(addr) < 0) return -1;
    if (wait_tx_not_full() < 0) return -1;
    wr(IC_DATA_CMD, reg | IC_DATA_CMD_RESTART);
    if (wait_tx_not_full() < 0) return -1;
    wr(IC_DATA_CMD, IC_DATA_CMD_READ | IC_DATA_CMD_RESTART | IC_DATA_CMD_STOP);
    if (wait_rx_not_empty() < 0) return -1;
    int val = (int)(rd(IC_DATA_CMD) & 0xff);
    if (wait_idle() < 0) return -1;
    if (has_abort()) {
        uint32_t src = rd(IC_TX_ABRT_SOURCE);
        clear_abort();
        errno = EIO;
        return -(int)(src ? src : 1);
    }
    return val;
}

static int map_base(uint32_t base) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    regs = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
    close(fd);
    if (regs == MAP_FAILED) {
        regs = NULL;
        return -1;
    }
    return 0;
}

static void unmap_base(void) {
    if (regs) munmap((void *)regs, 0x1000);
    regs = NULL;
}

static int use_base(uint32_t base) {
    if (map_base(base) < 0) {
        fprintf(stderr, "map 0x%08x failed: %s\n", base, strerror(errno));
        return -1;
    }
    uint32_t comp = rd(IC_COMP_TYPE);
    if (comp != 0x44570140u && comp != 0x44570110u) {
        printf("base 0x%08x: unexpected comp_type 0x%08x\n", base, comp);
    }
    return 0;
}

static void scan_one(uint32_t base) {
    if (use_base(base) < 0) return;
    printf("base 0x%08x:", base);
    for (int a = 0x03; a <= 0x77; ++a) {
        int v = i2c_read_reg((uint8_t)a, 0x00);
        if (v >= 0) printf(" %02x=%02x", a, v);
    }
    printf("\n");
    unmap_base();
}

static void dump_one(uint32_t base) {
    if (use_base(base) < 0) return;
    printf("base 0x%08x: comp=%08x con=%08x en=%08x enstat=%08x stat=%08x rawintr=%08x txflr=%08x rxflr=%08x abrt=%08x\n",
           base, rd(IC_COMP_TYPE), rd(IC_CON), rd(IC_ENABLE), rd(IC_ENABLE_STATUS),
           rd(IC_STATUS), rd(IC_RAW_INTR_STAT), rd(IC_TXFLR), rd(IC_RXFLR),
           rd(IC_TX_ABRT_SOURCE));
    unmap_base();
}

static int write_adv_table(uint32_t base, const void *table, size_t count, int dvi_mode) {
    const struct { uint8_t reg; uint8_t val; } *init = table;

    if (use_base(base) < 0) return 1;
    int id0 = i2c_read_reg(0x39, 0x00);
    int pwr = i2c_read_reg(0x39, 0x41);
    int stat = i2c_read_reg(0x39, 0x42);
    printf("before: reg00=%02x reg41=%02x reg42=%02x\n", id0 & 0xff, pwr & 0xff, stat & 0xff);
    if (id0 < 0 && pwr < 0 && stat < 0) {
        fprintf(stderr, "ADV7513 not readable at 0x39 on base 0x%08x\n", base);
        unmap_base();
        return 2;
    }
    for (size_t i = 0; i < count; ++i) {
        int r = i2c_write_reg(0x39, init[i].reg, init[i].val);
        if (r < 0) {
            fprintf(stderr, "write %02x=%02x failed: abort=0x%x\n", init[i].reg, init[i].val, -r);
            unmap_base();
            return 3;
        }
    }
    if (dvi_mode) {
        int r = i2c_write_reg(0x39, 0xaf, 0x04);
        if (r < 0) {
            fprintf(stderr, "write af=04 failed: abort=0x%x\n", -r);
            unmap_base();
            return 3;
        }
    }
    {
        int r = i2c_write_reg(0x39, 0x41, 0x00);
        if (r < 0) {
            fprintf(stderr, "write 41=00 failed: abort=0x%x\n", -r);
            unmap_base();
            return 3;
        }
    }
    {
        int r = i2c_write_reg(0x39, 0xd6, 0xd0);
        if (r < 0) {
            fprintf(stderr, "write d6=d0 failed: abort=0x%x\n", -r);
            unmap_base();
            return 3;
        }
    }
    id0 = i2c_read_reg(0x39, 0x00);
    pwr = i2c_read_reg(0x39, 0x41);
    stat = i2c_read_reg(0x39, 0x42);
    int fmt = i2c_read_reg(0x39, 0x16);
    int mode = i2c_read_reg(0x39, 0x3b);
    int vic = i2c_read_reg(0x39, 0x3c);
    int detected_vic = i2c_read_reg(0x39, 0x3e);
    int aux_vic = i2c_read_reg(0x39, 0x3f);
    int int0 = i2c_read_reg(0x39, 0x96);
    int int1 = i2c_read_reg(0x39, 0x97);
    int pll = i2c_read_reg(0x39, 0x9e);
    int hpd = i2c_read_reg(0x39, 0xd6);
    int ddc = i2c_read_reg(0x39, 0xc8);
    printf("after:  reg00=%02x reg16=%02x reg3b=%02x reg3c=%02x reg3e=%02x reg3f=%02x reg41=%02x reg42=%02x reg96=%02x reg97=%02x reg9e=%02x regc8=%02x regd6=%02x\n",
           id0 & 0xff, fmt & 0xff, mode & 0xff, vic & 0xff, detected_vic & 0xff,
           aux_vic & 0xff, pwr & 0xff, stat & 0xff, int0 & 0xff, int1 & 0xff,
           pll & 0xff, ddc & 0xff, hpd & 0xff);
    unmap_base();
    return 0;
}

static int init_adv_mode(uint32_t base, int dvi_mode) {
    return write_adv_table(base, adv_init, sizeof(adv_init) / sizeof(adv_init[0]), dvi_mode);
}

static int init_adv_terasic(uint32_t base) {
    return write_adv_table(base, adv_terasic_init, sizeof(adv_terasic_init) / sizeof(adv_terasic_init[0]), 0);
}

static int video_status(uint32_t base) {
    static const uint8_t regs_to_read[] = {
        0x00, 0x15, 0x16, 0x17, 0x3b, 0x3c, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x44, 0x48, 0x55, 0x56, 0x57,
        0x59, 0x96, 0x97, 0x9d, 0x9e, 0xa1, 0xaf, 0xba,
        0xc8, 0xd6, 0xde, 0xe4, 0xfa,
    };

    if (use_base(base) < 0) return 1;

    int id0 = i2c_read_reg(0x39, 0x00);
    if (id0 < 0) {
        fprintf(stderr, "ADV7513 not readable at 0x39 on base 0x%08x\n", base);
        unmap_base();
        return 2;
    }

    for (size_t i = 0; i < sizeof(regs_to_read) / sizeof(regs_to_read[0]); ++i) {
        uint8_t reg = regs_to_read[i];
        int val = i2c_read_reg(0x39, reg);
        printf("reg%02x=%02x%s", reg, val & 0xff,
               (i & 7) == 7 || i + 1 == sizeof(regs_to_read) / sizeof(regs_to_read[0]) ? "\n" : " ");
    }

    int stat = i2c_read_reg(0x39, 0x42);
    int int0 = i2c_read_reg(0x39, 0x96);
    int int1 = i2c_read_reg(0x39, 0x97);
    printf("decoded: powered_down=%d hpd=%d monitor=%d edid_ready_irq=%d ddc_error_irq=%d\n",
           (i2c_read_reg(0x39, 0x41) & 0x40) ? 1 : 0,
           (stat & 0x40) ? 1 : 0,
           (stat & 0x20) ? 1 : 0,
           (int0 & 0x04) ? 1 : 0,
           (int1 & 0x80) ? 1 : 0);

    unmap_base();
    return 0;
}

static int init_adv(uint32_t base) {
    return init_adv_mode(base, 0);
}

static int poke_adv(uint32_t base, uint8_t reg, uint8_t val) {
    if (use_base(base) < 0) return 1;
    int before = i2c_read_reg(0x39, reg);
    int wr_res = i2c_write_reg(0x39, reg, val);
    int after = i2c_read_reg(0x39, reg);
    printf("poke: reg%02x before=%02x write=%02x result=%d after=%02x\n",
           reg, before & 0xff, val, wr_res, after & 0xff);
    unmap_base();
    return wr_res < 0 ? 1 : 0;
}

static int read_edid(uint32_t base) {
    if (use_base(base) < 0) return 1;
    (void)i2c_write_reg(0x39, 0x41, 0x00);
    (void)i2c_write_reg(0x39, 0xd6, 0xd0);
    (void)i2c_write_reg(0x39, 0x43, 0x7e);
    (void)i2c_write_reg(0x39, 0x45, 0x70);
    (void)i2c_write_reg(0x39, 0xc4, 0x00);

    int stat = i2c_read_reg(0x39, 0x42);
    int ddc_before = i2c_read_reg(0x39, 0xc8);
    int int0_before = i2c_read_reg(0x39, 0x96);
    int int1_before = i2c_read_reg(0x39, 0x97);
    printf("edid-start: reg42=%02x regc8=%02x reg96=%02x reg97=%02x\n",
           stat & 0xff, ddc_before & 0xff, int0_before & 0xff, int1_before & 0xff);

    for (int i = 0; i < 10; ++i) {
        (void)i2c_write_reg(0x39, 0xc9, 0x03);
        (void)i2c_write_reg(0x39, 0xc9, 0x13);
        usleep(10000);
    }

    uint8_t edid[128];
    int ok = 1;
    for (int attempt = 0; attempt < 20; ++attempt) {
        ok = 1;
        for (int i = 0; i < 128; ++i) {
            int v = i2c_read_reg(0x3f, (uint8_t)i);
            if (v < 0) {
                fprintf(stderr, "edid read failed at attempt %d byte %d\n", attempt, i);
                ok = 0;
                break;
            }
            edid[i] = (uint8_t)v;
        }
        if (ok && memcmp(edid, "\x00\xff\xff\xff\xff\xff\xff\x00", 8) == 0) break;
        usleep(100000);
    }

    stat = i2c_read_reg(0x39, 0x42);
    int ddc_after = i2c_read_reg(0x39, 0xc8);
    int int0_after = i2c_read_reg(0x39, 0x96);
    int int1_after = i2c_read_reg(0x39, 0x97);
    printf("edid-end:   reg42=%02x regc8=%02x reg96=%02x reg97=%02x\n",
           stat & 0xff, ddc_after & 0xff, int0_after & 0xff, int1_after & 0xff);

    if (ok) {
        for (int i = 0; i < 128; ++i) {
            printf("%02x%s", edid[i], (i & 15) == 15 ? "\n" : " ");
        }
        ok = memcmp(edid, "\x00\xff\xff\xff\xff\xff\xff\x00", 8) == 0;
        printf("edid_header=%s\n", ok ? "valid" : "invalid");
    }
    unmap_base();
    return ok ? 0 : 1;
}

static void print_adv_status(const char *prefix) {
    int stat = i2c_read_reg(0x39, 0x42);
    int ddc = i2c_read_reg(0x39, 0xc8);
    int int0 = i2c_read_reg(0x39, 0x96);
    int int1 = i2c_read_reg(0x39, 0x97);
    int hpd = i2c_read_reg(0x39, 0xd6);
    printf("%s: reg42=%02x hpd=%d monitor=%d regc8=%02x reg96=%02x reg97=%02x regd6=%02x\n",
           prefix,
           stat & 0xff,
           (stat >= 0 && (stat & 0x40)) ? 1 : 0,
           (stat >= 0 && (stat & 0x20)) ? 1 : 0,
           ddc & 0xff,
           int0 & 0xff,
           int1 & 0xff,
           hpd & 0xff);
}

static int hpd_diag(uint32_t base) {
    static const uint8_t modes[] = {0x00, 0x10, 0x40, 0x50, 0x80, 0x90, 0xc0, 0xd0};

    if (use_base(base) < 0) return 1;

    int id0 = i2c_read_reg(0x39, 0x00);
    if (id0 < 0) {
        fprintf(stderr, "ADV7513 not readable at 0x39 on base 0x%08x\n", base);
        unmap_base();
        return 2;
    }

    print_adv_status("initial");
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        uint8_t mode = modes[i];
        int r = i2c_write_reg(0x39, 0xd6, mode);
        if (r < 0) {
            fprintf(stderr, "write d6=%02x failed: abort=0x%x\n", mode, -r);
            unmap_base();
            return 3;
        }
        usleep(100000);
        char label[32];
        snprintf(label, sizeof(label), "d6=%02x", mode);
        print_adv_status(label);
    }

    (void)i2c_write_reg(0x39, 0xd6, 0xd0);
    unmap_base();
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "dump") == 0) {
        for (size_t i = 0; i < sizeof(i2c_bases) / sizeof(i2c_bases[0]); ++i) dump_one(i2c_bases[i]);
        return 0;
    }
    if (argc == 1 || strcmp(argv[1], "scan") == 0) {
        if (argc > 2) {
            scan_one((uint32_t)strtoul(argv[2], NULL, 0));
        } else {
            for (size_t i = 0; i < sizeof(i2c_bases) / sizeof(i2c_bases[0]); ++i) scan_one(i2c_bases[i]);
        }
        return 0;
    }
    if (strcmp(argv[1], "init") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return init_adv(base);
    }
    if (strcmp(argv[1], "dvi") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return init_adv_mode(base, 1);
    }
    if (strcmp(argv[1], "terasic") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return init_adv_terasic(base);
    }
    if (strcmp(argv[1], "poke") == 0 && argc > 4) {
        uint32_t base = (uint32_t)strtoul(argv[2], NULL, 0);
        uint8_t reg = (uint8_t)strtoul(argv[3], NULL, 0);
        uint8_t val = (uint8_t)strtoul(argv[4], NULL, 0);
        return poke_adv(base, reg, val);
    }
    if (strcmp(argv[1], "edid") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return read_edid(base);
    }
    if (strcmp(argv[1], "status") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return video_status(base);
    }
    if (strcmp(argv[1], "hpd") == 0) {
        uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 0xffc06000u;
        return hpd_diag(base);
    }
    fprintf(stderr, "usage: %s [dump|scan [base]|init [base]|dvi [base]|terasic [base]|poke base reg val|edid [base]|status [base]|hpd [base]]\n", argv[0]);
    return 1;
}
