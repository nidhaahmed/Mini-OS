#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("My PID: %d\n", getpid());
    printf("My Parent PID: %d\n", getppid());

    return 0;
}