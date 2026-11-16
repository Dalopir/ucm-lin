#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

char int_to_hex(int n) {
    return n < 10 ? '0'+n : 'A'+(n-10);
}

int main() {
    int fd = open("/dev/display7s", O_WRONLY);
    if (fd == -1)
        return perror("patata"), 1;
    int pid = fork();
    if (pid == -1)
        return perror("patata"), 1;

    if (pid != 0) {
        while (1) {
            for (int i = 0; i < 16; i++) {
                char c = int_to_hex(i);
                if (write(fd, &c, 1) != 1)
                    return perror("patata"), 1;
            }
            sleep(1);
        }
    }else {
        while (1) {
            for (int i = 15; i >= 0; i--) {
                char c = int_to_hex(i);
                if (write(fd, &c, 1) != 1)
                    return perror("patata"), 1;
            }
            sleep(1);
        }
    }
}

