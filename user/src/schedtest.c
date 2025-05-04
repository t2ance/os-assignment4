#include "../lib/user.h"

void worker() {
    for (int i = 0; i < 200; i++) {
        // printf("worker %d: %d\n", getpid(), i);

        for (long j = 0; j < 5000000; j++) {
            asm volatile("" : : : "memory");
            // do nothing
        }

        sleep(1);

        // sleep(100);
    }
}

uint64 rtime() {
    uint64 x;
    asm volatile("csrr %0, time" : "=r"(x));
    return x;
}

#define NTIMES 10
#define NPROCS 8

int main(void) {
    struct {
        int pid;
        int timeused;
    } results[NPROCS];

    // run the test 10 times
    for (int j = 0; j < 10; j++) {
        printf("schedtest: running %d-th test\n", j);

        // fork `NPROCS` times
        for (int i = 0; i < NPROCS; i++) {
            int pid = fork();
            if (pid < 0) {
                printf("init: fork failed\n");
                exit(1);
            }
            if (pid == 0) {
                // child process

                // set priority
                setpriority(i);  // 0 - 7
                printf("worker %d: priority %d\n", getpid(), i);

                uint64 start = rtime();
                printf("worker %d: starts at %d\n", getpid(), start);
                worker();
                uint64 end = rtime();
                printf("worker %d: exits  at %d\n", getpid(), end);

                // calculate the time used as the exit code
                int used = (end - start) / 10000;
                exit(used);
            } else {
                // parent
                results[i].pid      = pid;
                results[i].timeused = 0;
            }
        }

        // wait all children
        int ret, pid;
        while ((pid = wait(-1, &ret)) > 0) {
            for (int i = 0; i < NPROCS; i++) {
                if (results[i].pid == pid) {
                    results[i].timeused = ret;
                    break;
                }
            }
        }

        // check results: lower priority should take shorter time
        for (int i = 0; i < NPROCS; i++) {
            printf("schedtest: worker %d: pid %d used %d time\n", i, results[i].pid, results[i].timeused);
        }
        for (int i = 0; i < NPROCS - 1; i++) {
            // we allow some error (50 time units)
            assert(results[i].timeused <= results[i + 1].timeused + 50);
        }
    }

    printf("\n\n===\nschedtest: 10-times tests passed\n===\n\n");

    return 0;
}