#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <stdint.h>

#define CHANNEL_SHM_SIZE (1 << 20)
#define CHANNEL_MSG_SIZE (1 << 12)

#define CHANNEL_MAX_NR_PUBLISHERS 256
#define CHANNEL_MAX_NR_SUBSCRIBERS 256

#define CHANNEL_MAX_NAME_LEN 20

struct message_metadata {
    char name[CHANNEL_MAX_NAME_LEN + 1];
    uint32_t len;
    uint64_t total_len;
    uint64_t offset;
};

typedef int (*produce_function_type)(void *, struct message_metadata *, void *);
typedef void (*consume_function_type)(void *, struct message_metadata, void *);

void publisher_node(char *channel_name, produce_function_type pf, void *cookie);
void subscriber_node(char *channel_name, consume_function_type cf, void *cookie);

#endif /* __CHANNEL_H__ */
