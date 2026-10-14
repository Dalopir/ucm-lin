#include <sys/syscall.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdio.h>
#define __NR_ledctl 452

long ledctl(unsigned int leds) {
    return (long) syscall(__NR_ledctl);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ledctl_invoke <argumento_de_ledctl>\n");
        return -1;
    }

    unsigned int arg = atoi(argv[1]);

    return 0;
}
