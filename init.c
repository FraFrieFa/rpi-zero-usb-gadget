struct timespec32 { int tv_sec; int tv_nsec; };

struct linux_dirent64 {
    unsigned long long d_ino;
    long long          d_off;
    unsigned short     d_reclen;
    unsigned char      d_type;
    char               d_name[];
};

__attribute__((noinline))
static long sys_open(const char *path, unsigned long flags, unsigned long mode) {
    register long           r7 asm("r7") = 5;
    register long           r0 asm("r0") = (long)path;
    register unsigned long  r1 asm("r1") = flags;
    register unsigned long  r2 asm("r2") = mode;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_close(int fd) {
    register long r7 asm("r7") = 6;
    register long r0 asm("r0") = fd;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_read(int fd, void *buf, unsigned long n) {
    register long          r7 asm("r7") = 3;
    register long          r0 asm("r0") = fd;
    register void         *r1 asm("r1") = buf;
    register unsigned long r2 asm("r2") = n;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_dup2(unsigned long oldfd, unsigned long newfd) {
    register long          r7 asm("r7") = 63;
    register unsigned long r0 asm("r0") = oldfd;
    register unsigned long r1 asm("r1") = newfd;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_write(int fd, const void *buf, unsigned long n) {
    register long          r7 asm("r7") = 4;
    register long          r0 asm("r0") = fd;
    register const void   *r1 asm("r1") = buf;
    register unsigned long r2 asm("r2") = n;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_mkdir(const char *path, unsigned long mode) {
    register long          r7 asm("r7") = 39;
    register long          r0 asm("r0") = (long)path;
    register unsigned long r1 asm("r1") = mode;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_symlink(const char *oldpath, const char *newpath) {
    register long        r7 asm("r7") = 83;
    register long        r0 asm("r0") = (long)oldpath;
    register const char *r1 asm("r1") = newpath;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_getdents64(unsigned int fd, struct linux_dirent64 *dirp,
                           unsigned int count) {
    register long                    r7 asm("r7") = 217;
    register unsigned int            r0 asm("r0") = fd;
    register struct linux_dirent64  *r1 asm("r1") = dirp;
    register unsigned int            r2 asm("r2") = count;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_nanosleep(const struct timespec32 *rq) {
    register long r7 asm("r7") = 162;
    register long r0 asm("r0") = (long)rq;
    register long r1 asm("r1") = 0;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1) : "memory");
    return r0;
}

__attribute__((noinline))
static long sys_mount(const char *source, const char *target, const char *type,
                      unsigned long flags, const void *data) {
    register long           r7 asm("r7") = 21;
    register const char    *r0 asm("r0") = source;
    register const char    *r1 asm("r1") = target;
    register const char    *r2 asm("r2") = type;
    register unsigned long  r3 asm("r3") = flags;
    register const void    *r4 asm("r4") = data;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4) : "memory");
    return (long)r0;
}

__attribute__((noinline))
static void sys_reboot(void) {
    register long          r7 asm("r7") = 88;
    register unsigned long r0 asm("r0") = 0xfee1dead;
    register unsigned long r1 asm("r1") = 672274793;
    register unsigned long r2 asm("r2") = 0x01234567;
    register long          r3 asm("r3") = 0;
    asm volatile("svc #0" : : "r"(r7), "r"(r0), "r"(r1), "r"(r2), "r"(r3) : "memory");
}

static void setup_console(void) {
    long fd = sys_open("/dev/console", 2, 0);
    if (fd < 0)
        return;

    sys_dup2((unsigned long)fd, 0);
    sys_dup2((unsigned long)fd, 1);
    sys_dup2((unsigned long)fd, 2);
}

static void writestr(const char *s) {
    unsigned n = 0;
    while (s[n]) n++;
    sys_write(1, s, n);
}

static void writehex(unsigned long v) {
    char buf[9];
    for (int i = 0; i < 8; i++)
        buf[i] = "0123456789abcdef"[(v >> (28 - i * 4)) & 0xf];
    buf[8] = '\n';
    sys_write(1, buf, 9);
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static unsigned strlen(const char *s) {
    unsigned n = 0;
    while (s[n]) n++;
    return n;
}

static long write_file(const char *path, const char *value) {
    long fd = sys_open(path, 1 /* O_WRONLY */, 0);
    if (fd < 0)
        return fd;

    long ret = sys_write((int)fd, value, strlen(value));
    sys_close((int)fd);
    return ret;
}

static int wait_for_path(const char *path) {
    struct timespec32 ts = { .tv_sec = 1, .tv_nsec = 0 };

    for (int i = 0; i < 20; i++) {
        long fd = sys_open(path, 0 /* O_RDONLY */, 0);
        if (fd >= 0) {
            sys_close((int)fd);
            return 0;
        }
        sys_nanosleep(&ts);
    }

    return -1;
}

static int first_udc(char *out, unsigned outlen) {
    char buf[512];
    long fd = sys_open("/sys/class/udc", 0 /* O_RDONLY */, 0);
    if (fd < 0)
        return -1;

    long nread = sys_getdents64((unsigned int)fd, (struct linux_dirent64 *)buf, sizeof(buf));
    sys_close((int)fd);
    if (nread <= 0)
        return -1;

    for (long off = 0; off < nread;) {
        struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + off);
        if (!streq(d->d_name, ".") && !streq(d->d_name, "..")) {
            unsigned i = 0;
            while (i + 1 < outlen && d->d_name[i]) {
                out[i] = d->d_name[i];
                i++;
            }
            out[i] = 0;
            return 0;
        }
        off += d->d_reclen;
    }

    return -1;
}

static long mkdirp_configfs_gadget(void) {
    long ret = 0;
    ret |= sys_mkdir("/sys", 0755);
    ret |= sys_mkdir("/sys/kernel", 0755);
    ret |= sys_mkdir("/sys/kernel/config", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/strings", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/strings/0x409", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/configs", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/configs/c.1", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/configs/c.1/strings", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/functions", 0755);
    ret |= sys_mkdir("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", 0755);
    return ret;
}

static int setup_gadget(void) {
    char udc[128];
    long ret;

    sys_mkdir("/sys", 0755);
    ret = sys_mount("sysfs", "/sys", "sysfs", 0, "");
    writestr("sysfs: ");
    writehex((unsigned long)ret);

    sys_mkdir("/sys/kernel/config", 0755);
    ret = sys_mount("configfs", "/sys/kernel/config", "configfs", 0, "");
    writestr("configfs: ");
    writehex((unsigned long)ret);

    ret = mkdirp_configfs_gadget();
    writestr("mkdir configfs gadget: ");
    writehex((unsigned long)ret);

    write_file("/sys/kernel/config/usb_gadget/g1/idVendor", "0x0a5c\n");
    write_file("/sys/kernel/config/usb_gadget/g1/idProduct", "0x0001\n");
    write_file("/sys/kernel/config/usb_gadget/g1/strings/0x409/serialnumber", "rpizero\n");
    write_file("/sys/kernel/config/usb_gadget/g1/strings/0x409/manufacturer", "Linux\n");
    write_file("/sys/kernel/config/usb_gadget/g1/strings/0x409/product", "Mass Storage Gadget\n");
    write_file("/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409/configuration", "mass_storage\n");
    write_file("/sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower", "250\n");
    write_file("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/stall", "0\n");
    write_file("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/removable", "1\n");
    write_file("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/ro", "0\n");
    ret = write_file("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", "/dev/mmcblk0\n");
    writestr("lun file: ");
    writehex((unsigned long)ret);

    ret = sys_symlink("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0",
                      "/sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.0");
    writestr("link function: ");
    writehex((unsigned long)ret);

    if (first_udc(udc, sizeof(udc)) < 0) {
        writestr("udc missing\n");
        return -1;
    }

    ret = write_file("/sys/kernel/config/usb_gadget/g1/UDC", udc);
    writestr("bind udc: ");
    writehex((unsigned long)ret);
    return ret < 0 ? -1 : 0;
}

static void wait_for_eject(void) {
    char buf[64];
    struct timespec32 ts = { .tv_sec = 1, .tv_nsec = 0 };

    for (;;) {
        long fd = sys_open("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file",
                           0 /* O_RDONLY */, 0);
        if (fd >= 0) {
            long n = sys_read((int)fd, buf, sizeof(buf));
            sys_close((int)fd);
            if (n == 0) {
                writestr("lun ejected\n");
                return;
            }
        }
        sys_nanosleep(&ts);
    }
}

void _start(void) {
    long ret = sys_mount("devtmpfs", "/dev", "devtmpfs", 0, "");

    setup_console();
    writestr("\001rpizero linux\n");
    writestr("devtmpfs: ");
    writehex((unsigned long)ret);

    if (wait_for_path("/dev/mmcblk0") == 0) {
        writestr("mmcblk0 ready\n");
        if (setup_gadget() == 0)
            wait_for_eject();
    } else {
        writestr("mmcblk0 missing\n");
    }

    writestr("rebooting\n");
    sys_reboot();

    for (;;)
        asm volatile("wfi" ::: "memory");
}
