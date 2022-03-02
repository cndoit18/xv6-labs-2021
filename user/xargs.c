#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define MAXLINE 512
int main(int argc, char* argv[])
{
    char *param[MAXARG], **point;
    char buf[MAXLINE], c, *p;

    if (argc < 2) {
        fprintf(2, "Usage: xargs params...\n");
        exit(1);
    }
    memset(param, 0, MAXARG);
    point = param;
    for (int i = 1; i < argc; i++)
        *(point++) = argv[i];

    char* cp = buf;
    while (read(0, &c, sizeof(char)) == sizeof(char)) {
        if (c == '\n') {
            *(cp++) = 0;
            if (fork() == 0) {
                p = buf;
                do {
                    if (*p == ' ') {
                        *(p++) = 0;
                    }
                    *(point++) = p;
                } while ((p = strchr(buf, ' ')));
                exec(argv[1], param);
                fprintf(2, "xargs: failed to exec %s\n", argv[1]);
                exit(1);
            }
            cp = buf;
            continue;
        }
        *(cp++) = c;
    }

    wait(0);
    exit(0);
}