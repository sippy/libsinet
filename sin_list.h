struct sin_list {
    struct sin_type_linkable *head;
    struct sin_type_linkable *tail;
    unsigned int len;
};

static inline void
sin_list_append(struct sin_list *lst, void *p)
{
    struct sin_type_linkable *elem;

    elem = (struct sin_type_linkable *)p;
#ifdef SIN_DEBUG
    assert(SIN_TYPE_IS_LINKABLE(elem));
    assert(elem->sin_next == NULL);
#endif
    if (lst->head == NULL) {
        lst->head = lst->tail = elem;
    } else {
        lst->tail->sin_next = elem;
        lst->tail = elem;
    }
    lst->len += 1;
}
