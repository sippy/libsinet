static inline void
sin_pkt_zone_ret_pkt(struct sin_pkt *pkt, struct sin_pkt_zone *pzone)
{

#ifdef SIN_DEBUG
    assert(pzone->first[pkt->zone_idx] == NULL);
#endif
    pzone->first[pkt->zone_idx] = pkt;
}

static inline void
sin_pkt_zone_swap(struct sin_pkt *p1, struct sin_pkt *p2)
{
    struct netmap_slot *src, *dst;

    src = p1->my_slot;
    dst = p2->my_slot;

    _SIN_SWAP_I(dst->buf_idx, src->buf_idx);
    _SIN_SWAP_P(p1->buf, p2->buf);
    dst->flags = NS_BUF_CHANGED;
    src->flags = NS_BUF_CHANGED;
    p2->len = dst->len = p1->len;
}

static inline void
sin_pkt_zone_copy(struct sin_pkt *p1, struct sin_pkt *p2)
{
    struct netmap_slot *dst;

    dst = p2->my_slot;

    sin_memcpy(p1->buf, p2->buf, p1->len);
    p2->len = dst->len = p1->len;
    dst->flags = NS_BUF_CHANGED;
}
