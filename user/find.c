#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* strstr(const char* str1, const char* str2)
{
    char* cp = (char*)str1;
    char *s1, *s2;
    if (!*str2)
        return (char*)str1;
    while (*cp) {
        s1 = cp;
        s2 = (char*)str2;
        while (*s2 && !(*s1 - *s2))
            s1++, s2++;
        if (!*s2)
            return cp;

        cp++;
    }

    return 0;
}

void find(char* path, char* name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    case T_FILE:
        if (strstr(path, name) != 0)
            printf("%s\n", path);
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0) {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            find(buf, name);
        }
        break;
    }
    close(fd);
}

int main(int argc, char* argv[])
{

    if (argc != 3) {
        fprintf(2, "Usage: find files...\n");
        exit(1);
    }
    find(argv[1], argv[2]);

    exit(0);
}
