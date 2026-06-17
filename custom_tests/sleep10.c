#include <stdio.h>
#include <unistd.h>

int
main()
{
    printf("sleep10 started by PID %d\n", getpid());
    fflush(stdout);
    sleep(10);
    printf("sleep10 finished by PID %d\n", getpid());
    fflush(stdout);
    return 0;
}
