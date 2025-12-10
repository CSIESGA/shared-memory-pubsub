#ifndef __LIST_H__
#define __LIST_H__

struct list_head {
    struct list_head *prev, *nxt;
};

#define LIST_HEAD(head) struct list_head head = {&(head), &(head)}

static inline void
INIT_LIST_HEAD(struct list_head *head)
{
    head->nxt = head->prev = head;
}

static inline int
list_empty(const struct list_head *head)
{
    return head->nxt == head;
}

static inline int
list_is_singular(const struct list_head *head)
{
    return (head->nxt->nxt == head) && (!list_empty(head));
}

static inline void
list_add(struct list_head *new_, struct list_head *head)
{
    new_->nxt = head->nxt;
    new_->prev = head;
    head->nxt->prev = new_;
    head->nxt = new_;
}

static inline void
list_add_tail(struct list_head *new_, struct list_head *head)
{
    new_->prev = head->prev;
    new_->nxt = head;
    head->prev->nxt = new_;
    head->prev = new_;
}

static inline void
__list_del_entry(struct list_head *entry)
{
    entry->prev->nxt = entry->nxt;
    entry->nxt->prev = entry->prev;
}

static inline void
list_del_init(struct list_head *entry)
{
    __list_del_entry(entry);
    entry->nxt = entry->prev = entry;
}

static inline void
list_move(struct list_head *list, struct list_head *head)
{
    __list_del_entry(list);
    list_add(list, head);
}

static inline void
list_move_tail(struct list_head *list, struct list_head *head)
{
    __list_del_entry(list);
    list_add_tail(list, head);
}

#ifndef offsetof
#define offsetof(type, member) ((size_t) &(((type *)0)->member))
#endif /* offsetof */

#define container_of(ptr, type, member) ({                       \
        const __typeof__(((type *) 0)->member) *(__ptr) = (ptr); \
        (type *) (((char *)__ptr) - offsetof(type, member));     \
    })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(head, type, member) \
    list_entry((head)->nxt, type, member);

#define list_for_each(pos, head) \
    struct list_head *__head = (head); for (pos = __head->nxt; pos != __head; pos = pos->nxt)

#define list_for_each_safe(pos, n, head) \
    struct list_head *__head = (head);   \
    for (pos = __head->nxt, n = pos->nxt; pos != __head; pos = n, n = pos->nxt)

#define list_for_each_entry(pos, head, member) \
    struct list_head *__head = (head), *__pos; \
    for (__pos = __head->nxt, pos = list_entry(__pos, __typeof__(*pos), member); \
         __pos != __head; __pos = __pos->nxt, pos = list_entry(__pos, __typeof__(*pos), member))

#endif /* __LIST_H__ */
