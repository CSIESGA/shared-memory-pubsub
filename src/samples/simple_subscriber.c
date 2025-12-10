#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <hw4/utils.h>

#include "channel.h"

void
log_msg(void *msg_buf, struct message_metadata m, void *cookie)
{
    static char buf[CHANNEL_MSG_SIZE + 1];
    int is_full_log = (int)cookie;

    puts("==========================================\n");
    printf("Get a message:\n"
           "name: %s\n"
           "len: %"PRIu32"\n"
           "total len: %"PRIu64"\n"
           "offset: %"PRIu64"\n",
           m.name, m.len, m.total_len, m.offset);
    puts("\n==========================================\n");
    
    if (is_full_log) {
        memcpy(buf, msg_buf, CHANNEL_MSG_SIZE);
        buf[m.len] = '\0';
        printf("%s\n", buf);
        puts("\n==========================================\n");
    }

}

int
main(int argc, char *argv[])
{
    if (argc != 3)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <full log (0/1)>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);
    
    if (strlen(argv[1]) > CHANNEL_MAX_NAME_LEN)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <full log (0/1)>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);

    int is_full_log = atoi(argv[2]);
    if (is_full_log != 0 && is_full_log != 1)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <full log (0/1)>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);

    subscriber_node(argv[1], log_msg, (void*)is_full_log);

    return 0;
}
