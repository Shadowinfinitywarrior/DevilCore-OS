void _start(void) {
    volatile unsigned short *vga = (volatile unsigned short *)0xB8000;
    const char *msg = "DevilCore OS v1.0 - Kernel Loaded OK!        ";
    int i = 0, j = 0;
    while(msg[i]) {
        vga[j++] = (unsigned short)(msg[i] | 0x0F00);
        i++;
    }
    for(;;) { }
}
