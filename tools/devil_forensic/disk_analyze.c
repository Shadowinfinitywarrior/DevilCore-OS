# tools/devil_forensic/disk_analyze.c
// Disk forensic analysis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static void analyze_partition(const char *device) {
    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", device);
        return;
    }
    
    unsigned char buf[512];
    read(fd, buf, sizeof(buf));
    
    printf("MBR Signature: %02X%02X\n", buf[510], buf[511]);
    
    // Check partitions
    for (int i = 0; i < 4; i++) {
        int offset = 446 + i * 16;
        if (buf[offset] != 0) {
            printf("Partition %d: type=0x%02X start=%d size=%d\n",
                i+1, buf[offset+4],
                *(unsigned int*)&buf[offset+8],
                *(unsigned int*)&buf[offset+12]);
        }
    }
    
    close(fd);
}

static void file_carve(const char *file, const char *output) {
    FILE *in = fopen(file, "rb");
    FILE *out = fopen(output, "wb");
    if (!in || !out) return;
    
    unsigned char buf[4096];
    int in_file = 0;
    
    while (fread(buf, 1, sizeof(buf), in) > 0) {
        // Look for file signatures
        fwrite(buf, 1, sizeof(buf), out);
    }
    
    fclose(in);
    fclose(out);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: devil_forensic <command> [args]\n");
        return 1;
    }
    
    if (!strcmp(argv[1], "analyze")) {
        analyze_partition(argv[2]);
    } else if (!strcmp(argv[1], "carve")) {
        file_carve(argv[2], argv[3]);
    }
    
    return 0;
}