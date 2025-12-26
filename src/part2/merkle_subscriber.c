// // #include <stdlib.h>
// // #include <string.h>

// // #include <pthread.h>

// // #include <hw4/utils.h>
// // #include <hw4/sha256.h>

// // #include "channel.h"

// // void
// // process_block(void *buffer, struct message_metadata m, void *cookie)
// // {
// // }

// // void *
// // worker(void *args __maybe_unused)
// // {
// // }

// // void
// // spawn_workers(int nr_threads)
// // {
// //     /*
// //      * Note: This function currently does not store thread IDs.
// //      *       It's just a prototype, feel free to modify.
// //      */

// //     pthread_t t;

// //     for (int i = 0; i < nr_threads; i++) {
// //         if (pthread_create(&t, NULL, worker, NULL) != 0)
// //             ERROR_EXIT("`pthread_create` failed\n");
// //     }
// // }

// // int
// // main(int argc, char *argv[])
// // {
// //     if (argc != 3)
// //         ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
// //                    argv[0], CHANNEL_MAX_NAME_LEN);
// //     if (strlen(argv[1]) > CHANNEL_MAX_NAME_LEN)
// //         ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
// //                    argv[0], CHANNEL_MAX_NAME_LEN);

// //     int nr_threads = atoi(argv[2]);
// //     if (nr_threads < 1 || nr_threads > 32)
// //         ERROR_EXIT("Usage: %s <channel name (<= %d chars)> <threads num ([1:32])>\n",
// //                    argv[0], CHANNEL_MAX_NAME_LEN);

// //     spawn_workers(nr_threads);
// //     subscriber_node(argv[1], process_block, NULL);

// //     return 0;
// // }
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "channel.h"
#include "hw4/sha256.h"
#include "hw4/utils.h"
/*
 * Merkle Tree Subscriber (SP 2025 HW4 Part 2)
 *
 * Spec essentials:
 * - process_block(): compute the leaf hash (layer 0) and enqueue a task.
 * - worker threads: build internal nodes (including promotion when no sibling).
 * - Output ONLY the root:
 *     [ANSWER] filename: hash
 *
 * IMPORTANT: Internal node hash is SHA256(H_left || H_right), where H_* are
 * the 64-hex-character strings (concatenation of the hex strings). fileciteturn5file0L46-L123
 */

#define BLOCK_SIZE CHANNEL_MSG_SIZE

struct layer_store {
    uint64_t slots;           /* number of possible nodes in this layer */
    uint8_t *present;         /* [slots] */
    char *hashes;             /* [slots][SHA256_LEN+1] as a flat buffer */
};

struct file_ctx {
    char name[CHANNEL_MAX_NAME_LEN + 1];
    uint64_t total_len;
    uint64_t nblocks;
    uint32_t root_layer;

    pthread_mutex_t mu;
    int done;
    int printed;
    uint32_t refcnt;          /* number of tasks in flight */

    struct layer_store *layers; /* [root_layer+1] */

    struct file_ctx *next;    /* global list */
};

struct task {
    struct file_ctx *f;
    uint32_t layer;
    uint64_t index;           /* block index (offset / 4KB) at current layer base */
    char hash[SHA256_LEN + 1];
    struct task *next;
};

/* ---------------- global file registry ---------------- */
static pthread_mutex_t g_files_mu = PTHREAD_MUTEX_INITIALIZER;
static struct file_ctx *g_files = NULL;

/* ---------------- task queue ---------------- */
static pthread_mutex_t g_q_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_q_cv = PTHREAD_COND_INITIALIZER;
static struct task *g_q_head = NULL;
static struct task *g_q_tail = NULL;

static uint64_t ceil_div_u64(uint64_t a, uint64_t b) {
    return (a + b - 1) / b;
}

static uint32_t compute_root_layer(uint64_t nblocks) {
    uint32_t l = 0;
    while ((1ULL << l) < nblocks) l++;
    return l;
}

static struct file_ctx *file_create(const char *name, uint64_t total_len) {
    struct file_ctx *f = (struct file_ctx *)calloc(1, sizeof(*f));
    if (!f) exit(2);

    size_t n = 0;
    while (n < CHANNEL_MAX_NAME_LEN && name[n] != "\0"[0]) n++;
    memcpy(f->name, name, n);
    f->name[n] = "\0"[0];


    f->total_len = total_len;
    f->nblocks = ceil_div_u64(total_len, BLOCK_SIZE);
    if (f->nblocks == 0) f->nblocks = 1; /* defensive */
    f->root_layer = compute_root_layer(f->nblocks);

    pthread_mutex_init(&f->mu, NULL);
    f->done = 0;
    f->printed = 0;
    f->refcnt = 0;

    f->layers = (struct layer_store *)calloc(f->root_layer + 1, sizeof(struct layer_store));
    if (!f->layers) exit(2);

    for (uint32_t l = 0; l <= f->root_layer; l++) {
        uint64_t stride = 1ULL << l;
        uint64_t slots = ceil_div_u64(f->nblocks, stride);
        f->layers[l].slots = slots;
        f->layers[l].present = (uint8_t *)calloc(slots, sizeof(uint8_t));
        f->layers[l].hashes = (char *)calloc(slots, (SHA256_LEN + 1));
        if (!f->layers[l].present || !f->layers[l].hashes) exit(2);
    }

    return f;
}

static void file_free(struct file_ctx *f) {
    if (!f) return;
    for (uint32_t l = 0; l <= f->root_layer; l++) {
        free(f->layers[l].present);
        free(f->layers[l].hashes);
    }
    free(f->layers);
    pthread_mutex_destroy(&f->mu);
    free(f);
}

static struct file_ctx *files_get_or_create(const char *name, uint64_t total_len) {
    pthread_mutex_lock(&g_files_mu);
    struct file_ctx *cur = g_files;
    while (cur) {
        if (strncmp(cur->name, name, CHANNEL_MAX_NAME_LEN) == 0) {
            pthread_mutex_unlock(&g_files_mu);
            return cur;
        }
        cur = cur->next;
    }

    struct file_ctx *f = file_create(name, total_len);
    f->next = g_files;
    g_files = f;
    pthread_mutex_unlock(&g_files_mu);
    return f;
}

static void files_remove_if_present(struct file_ctx *f) {
    pthread_mutex_lock(&g_files_mu);
    struct file_ctx **pp = &g_files;
    while (*pp) {
        if (*pp == f) {
            *pp = f->next;
            break;
        }
        pp = &((*pp)->next);
    }
    pthread_mutex_unlock(&g_files_mu);
}

/* --------------- hashing helpers --------------- */
static void hash_concat_hex(const char left[SHA256_LEN + 1], const char right[SHA256_LEN + 1],
                            char out[SHA256_LEN + 1]) {
    char buf[SHA256_LEN * 2];
    memcpy(buf, left, SHA256_LEN);
    memcpy(buf + SHA256_LEN, right, SHA256_LEN);
    sha256(buf, (uint64_t)(SHA256_LEN * 2), out);
}

/* --------------- task queue helpers --------------- */
static void enqueue_task(struct task *t) {
    pthread_mutex_lock(&g_q_mu);
    t->next = NULL;
    if (!g_q_tail) {
        g_q_head = g_q_tail = t;
    } else {
        g_q_tail->next = t;
        g_q_tail = t;
    }
    pthread_cond_signal(&g_q_cv);
    pthread_mutex_unlock(&g_q_mu);
}

static struct task *dequeue_task(void) {
    pthread_mutex_lock(&g_q_mu);
    while (!g_q_head) {
        pthread_cond_wait(&g_q_cv, &g_q_mu);
    }
    struct task *t = g_q_head;
    g_q_head = t->next;
    if (!g_q_head) g_q_tail = NULL;
    pthread_mutex_unlock(&g_q_mu);
    return t;
}

/* --------------- core reduction logic --------------- */
static void reduce_up(struct file_ctx *f, uint32_t layer, uint64_t index, char hash[SHA256_LEN + 1]) {
    while (1) {
        /* Root condition: when 2^layer >= total blocks. fileciteturn5file0L138-L142 */
        if ((1ULL << layer) >= f->nblocks) {
            pthread_mutex_lock(&f->mu);
            if (!f->printed) {
                f->printed = 1;
                pthread_mutex_unlock(&f->mu);
                printf("[ANSWER] %s: %s\n", f->name, hash);
                fflush(stdout);
                pthread_mutex_lock(&f->mu);
            }
            f->done = 1;
            pthread_mutex_unlock(&f->mu);
            return;
        }

        uint64_t stride = 1ULL << layer;
        /* Normalize to the left node index of this pair */
        uint64_t pair = (index / stride) & ~1ULL;
        uint64_t left_index = pair * stride;
        uint64_t right_index = left_index + stride;

        if (right_index >= f->nblocks) {
            /* No sibling -> promotion without computation. fileciteturn5file0L124-L136 */
            index = left_index;
            layer++;
            continue;
        }

        uint64_t left_pos = left_index >> layer;
        uint64_t right_pos = right_index >> layer;

        int is_left = (index == left_index);
        uint64_t my_pos = is_left ? left_pos : right_pos;
        uint64_t sib_pos = is_left ? right_pos : left_pos;

        char sib_hash[SHA256_LEN + 1];
        char my_hash[SHA256_LEN + 1];

        pthread_mutex_lock(&f->mu);
        if (f->done) {
            pthread_mutex_unlock(&f->mu);
            return;
        }

        if (!f->layers[layer].present[sib_pos]) {
            /* Sibling not ready: store current and stop. */
            f->layers[layer].present[my_pos] = 1;
            memcpy(f->layers[layer].hashes + my_pos * (SHA256_LEN + 1), hash, SHA256_LEN + 1);
            pthread_mutex_unlock(&f->mu);
            return;
        }

        /* Sibling ready: fetch sibling, clear sibling slot, compute parent and continue */
        memcpy(sib_hash, f->layers[layer].hashes + sib_pos * (SHA256_LEN + 1), SHA256_LEN + 1);
        f->layers[layer].present[sib_pos] = 0;
        pthread_mutex_unlock(&f->mu);

        memcpy(my_hash, hash, SHA256_LEN + 1);

        char parent[SHA256_LEN + 1];
        if (is_left) {
            hash_concat_hex(my_hash, sib_hash, parent);
        } else {
            hash_concat_hex(sib_hash, my_hash, parent);
        }

        memcpy(hash, parent, SHA256_LEN + 1);
        index = left_index;
        layer++;
    }
}

/* --------------- API callbacks --------------- */

struct subscriber_cookie {
    /* empty for now */
    int dummy;
};

void process_block(void *buffer, struct message_metadata m, void *cookie) {
    (void)cookie;

    /* Identify file and compute leaf hash. fileciteturn5file0L164-L177 */
    struct file_ctx *f = files_get_or_create(m.name, m.total_len);

    pthread_mutex_lock(&f->mu);
    if (f->done) {
        pthread_mutex_unlock(&f->mu);
        return;
    }
    f->refcnt++;
    pthread_mutex_unlock(&f->mu);

    char leaf[SHA256_LEN + 1];
    sha256(buffer, (uint64_t)m.len, leaf);

    uint64_t index = (uint64_t)(m.offset / BLOCK_SIZE);

    struct task *t = (struct task *)malloc(sizeof(*t));
    if (!t) exit(2);
    t->f = f;
    t->layer = 0;
    t->index = index;
    memcpy(t->hash, leaf, SHA256_LEN + 1);

    enqueue_task(t);
}

static void *worker(void *args) {
    (void)args;
    while (1) {
        struct task *t = dequeue_task();
        struct file_ctx *f = t->f;

        char h[SHA256_LEN + 1];
        memcpy(h, t->hash, SHA256_LEN + 1);
        uint32_t layer = t->layer;
        uint64_t index = t->index;
        free(t);

        reduce_up(f, layer, index, h);

        int do_free = 0;
        pthread_mutex_lock(&f->mu);
        if (f->refcnt > 0) f->refcnt--;
        if (f->done && f->refcnt == 0) do_free = 1;
        pthread_mutex_unlock(&f->mu);

        if (do_free) {
            files_remove_if_present(f);
            file_free(f);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <channel name (<= 20 chars)> <threads num ([1:32])>\n", argv[0]);
        exit(1);
    }

    char *channel_name = argv[1];
    int nthreads = atoi(argv[2]);
    if (nthreads < 1) nthreads = 1;
    if (nthreads > 32) nthreads = 32;

    pthread_t th[32];
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&th[i], NULL, worker, NULL) != 0) {
            fprintf(stderr, "[ERROR:%s:%d]: pthread_create failed\n", __FILE__, __LINE__);
            exit(2);
        }
    }

    struct subscriber_cookie ck;
    ck.dummy = 0;
    subscriber_node(channel_name, process_block, &ck);

    return 0;
}
