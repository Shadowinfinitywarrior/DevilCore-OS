#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    printf("dev0   0,0   0,0    0   /dev/null\n");
    printf("console 0,0   0,0    0   /dev/console\n");
    printf("tty    0,0   0,0    0   /dev/tty\n");
    printf("ttyS0  0,0   0,0    0   /dev/ttyS0\n");
    printf("ram0   0,0   0,0    0   /dev/ram0\n");
    printf("loop0  0,0   0,0    0   /dev/loop0\n");
    printf("sr0   0,0   0,0    0   /dev/sr0\n");
    printf("sda    0,0   0,0    0   /dev/sda\n");
    printf("sda1   0,0   0,0    0   /dev/sda1\n");
    printf("\n");
    
    printf("lo    Link encap:Local Loopback\n");
    printf("      inet addr:127.0.0.1  Mask:255.0.0.0\n");
    printf("      UP LOOPBACK RUNNING  MTU:65536  Metric:1\n");
    printf("\n");
    
    printf("eth0   Link encap:Ethernet  HWaddr 52:54:00:12:34:56\n");
    printf("      inet addr:192.168.1.100  Bcast:192.168.1.255  Mask:255.255.255.0\n");
    printf("      UP BROADCAST MULTICAST  MTU:1500  Metric:1\n");
    
    return 0;
}