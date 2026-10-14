#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define __NR_ledctl 452
long ledctl(unsigned int leds) {
    return (long) syscall(__NR_ledctl, leds);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <leds>\n", argv[0]);
        return 1;
    }

    char *end;
    long leds = strtol(argv[1], &end, 0);
    if (errno != 0 || end == argv[1] || *end != '\0' || leds < 0 || leds > 7) {
        fprintf(stderr, "<leds> debe ser un número entre 0-7\n");
        return 1;
    }

    if (ledctl(leds) != 0) {
        perror("ledctl");
        return 1;
    }

    printf("El estado de los leds ha sido modificado correctamente\n");

    return 0;
}
