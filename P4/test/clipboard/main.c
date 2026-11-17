#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CONSUMER_CNT 8
#define BUF_SZ 128
#define TICK_US 20000

static pid_t consumers[CONSUMER_CNT];

static void kill_consumers(void) {
    for (int i = 0; i < CONSUMER_CNT; i++)
        kill(consumers[i], SIGTERM);
    for (int i = 0; i < CONSUMER_CNT; i++)
        waitpid(consumers[i], NULL, 0);
}

int producer() {
    int fd = open("/dev/clipboard_update", O_RDWR);
    if (fd == -1)
        return perror("Could not open the file"), 1;

    int next_task[CONSUMER_CNT];
    memset(next_task, 0, sizeof(next_task));

    while (1) {
        int id = rand()%CONSUMER_CNT;
        int task = next_task[id]++;

        char buf[BUF_SZ];
        if (snprintf(buf, BUF_SZ, "%d %d", id, task) >= BUF_SZ) {
            fprintf(stderr, "Buffer too small\n");
            kill_consumers();
            return 1;
        }
        int n = strlen(buf);

        printf("Producer: %s\n", buf);
        usleep(TICK_US);
        if (pwrite(fd, buf, n, 0) != n) {
            fprintf(stderr, "Write didn't work properly\n");
            kill_consumers();
            return 1;
        }

        while (1) {
            if ((n = pread(fd, buf, BUF_SZ-1, 0)) == -1) {
                fprintf(stderr, "Read failed\n");
                kill_consumers();
                return 1;
            }
            buf[n] = '\0';
            if (!strcmp(buf, "ok"))
                break;
            printf("Not ok: \"%s\"\n", buf);
            kill_consumers();
            return 1;
        }
    }

    return 0;
}

int consumer(int id) {
    int fd = open("/dev/clipboard_update", O_RDWR);
    if (fd == -1)
        return perror("Could not open the file"), 1;

    int task = 0;
    while (1) {
        while (1) {
            char buf[BUF_SZ];
            int n;
            if ((n = pread(fd, buf, BUF_SZ-1, 0)) == -1) {
                fprintf(stderr, "Consumer %d: Read failed\n", id);
                return 1;
            }
            buf[n] = '\0';

            int c_id, c_task;
            if (sscanf(buf, "%d %d", &c_id, &c_task) == 2 && c_id == id) {
                if (c_task != task) {
                    fprintf(stderr, "Consumer %d: Incorrect task %d, expected %d\n", id, c_task, task);
                    return 1;
                }
                task++;

                printf("Consumer %d: ok\n", id);
                usleep(TICK_US);
                pwrite(fd, "ok", 3, 0);
                printf("Written\n");
                break;
            }
        }
    }

    return 0;
}

int main() {
    for (int i = 0; i < CONSUMER_CNT; i++) {
        int pid = fork();
        if (pid == -1)
            return perror("Could not fork"), 1;
        if (pid == 0)
            return consumer(i);
        else
            consumers[i] = pid;
    }
    return producer();
}
