#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <hw4/utils.h>
#include <hw4/sha256.h>

#include "channel.h"

void
process_block(void *buffer, struct message_metadata m, void *cookie)
{
}

void *
worker(void *args __maybe_unused)
{
}

void
spawn_workers(int nr_threads)
{
    /*
     * Note: This function currently does not store thread IDs.
     *       It's just a prototype, feel free to modify.
     */

    pthread_t t;

    for (int i = 0; i < nr_threads; i++) {
        if (pthread_create(&t, NULL, worker, NULL) != 0)
            ERROR_EXIT("`pthread_create` failed\n");
    }
}

int
main(int argc, char *argv[])
{
    if (argc != 3)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);
    if (strlen(argv[1]) > CHANNEL_MAX_NAME_LEN)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);

    int nr_threads = atoi(argv[2]);
    if (nr_threads < 1 || nr_threads > 32)
        ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
                   argv[0], CHANNEL_MAX_NAME_LEN);

    spawn_workers(nr_threads);
    subscriber_node(argv[1], process_block, NULL);

    return 0;
}
