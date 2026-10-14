#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#define __NR_lin_hello 451

long lin_hello(void) {
    return (long) syscall(__NR_lin_hello);
}

int main (void) {
    printf("El código de retorno de la llamada my_syscall es %ld\n", lin_hello());
    return 0;
}
