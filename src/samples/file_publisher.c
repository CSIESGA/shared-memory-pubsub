#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>

#include <hw4/utils.h>

#include "channel.h"

struct file {
    char name[CHANNEL_MAX_NAME_LEN + 1];
    int fd;
    uint64_t total_len, offset;
};

char *
basename(char *path)
{
    char *last_slash;

    if (path == NULL || *path == '\0')
        return "";

    char *end = path + strlen(path) - 1;
    while (end > path && *end == '/')
        end--;

    last_slash = end;
    while (last_slash > path && *last_slash != '/')
        last_slash--;

    if (*last_slash == '/')
        return last_slash + 1;
    else
        return path;
}

uint64_t
minu64(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

int
send_file(void *buf, struct message_metadata *out_m, void *cookie)
{
    struct file *f = cookie;

    if (f->offset == f->total_len) {
        printf("Successfully sent the file '%s'\n", f->name);
        return -1;
    }

    strcpy(out_m->name, f->name);
    out_m->total_len = f->total_len;
    out_m->offset = f->offset;
    out_m->len = minu64(f->total_len - f->offset, 4096);

    f->offset += out_m->len;

    if (read(f->fd, buf, out_m->len) != out_m->len) {
        int serrno = errno;
        ERROR_EXIT("read error (%s)\n", strerror(serrno));
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    struct file f;
    struct stat st;

    if (argc != 3 || strlen(argv[1]) > CHANNEL_MAX_NAME_LEN)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <file path>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);

    char *file_name = basename(argv[2]);
    if (strlen(file_name) > CHANNEL_MAX_NAME_LEN)
        ERROR_EXIT("File name too long (<= %d chars)\n", CHANNEL_MAX_NAME_LEN);

    if (stat(argv[2], &st) < 0)
        ERROR_EXIT("stat '%s' error\n", argv[2]);

    if (!S_ISREG(st.st_mode))
        ERROR_EXIT("'%s' is not a regular file\n", argv[2]);

    strcpy(f.name, file_name);
    f.total_len = st.st_size;
    f.offset = 0;
    f.fd = open(argv[2], O_RDONLY);
    if (f.fd < 0)
        ERROR_EXIT("open '%s' error\n", argv[2]);

    publisher_node(argv[1], send_file, &f);

    close(f.fd);

    return 0;
}
