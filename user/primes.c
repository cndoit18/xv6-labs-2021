#include "kernel/types.h"
#include "user/user.h"

void filter(int* in);
int main(int argc, char* argv[])
{
    int in[2];
    close(0);
    pipe(in);
    if (fork() == 0) {
        filter(in);
        exit(0);
    }
    close(in[0]);
    for (int i = 2; i <= 35; i++) {
        write(in[1], &i, sizeof(i));
    }
    close(in[1]);
    wait(0);
    exit(0);
}

void filter(int* in)
{
    // close upsteam input
	close(in[1]);
    int init;
    if (read(in[0], &init, sizeof(init)) <= 0)
		exit(0);
    printf("prime %d\n", init);

    int downstream[2];
    pipe(downstream);
    if (fork() == 0) {
        filter(downstream);
        exit(0);
    }

    int cur;
    while (read(in[0], &cur, sizeof(cur)) != 0) {
        if (cur % init != 0)
            write(downstream[1], &cur, sizeof(cur));
    }
    close(downstream[0]);
    close(downstream[1]);
	close(in[0]);
    wait(0);
}