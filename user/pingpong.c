#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
	int pipes[2];
	int pid;
	char c = '\0';
	pipe(pipes);
	if ((pid = fork()) == 0) {
		close(0);
		// sub-processes
		read(pipes[0], &c, sizeof(char));
		fprintf(1, "%d: received ping\n", getpid());
		write(pipes[1], &c, sizeof(char));
		close(pipes[0]);
		close(pipes[1]);
	} else {
		close(0);
		write(pipes[1], &c, sizeof(char));
		read(pipes[0], &c, sizeof(char));
		fprintf(1, "%d: received pong\n", getpid());
		close(pipes[0]);
		close(pipes[1]);
		wait(0);
	}
	exit(0);
}