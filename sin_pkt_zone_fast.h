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
    uint32_t tmp;
    struct netmap_slot *src, *dst;

    src = &p1->my_ring->slot[p1->zone_idx];
    dst = &p2->my_ring->slot[p2->zone_idx];
    tmp = dst->buf_idx;
    dst->buf_idx = src->buf_idx;
    dst->len = src->len;
    dst->flags = NS_BUF_CHANGED;
    src->buf_idx = tmp;
    src->flags = NS_BUF_CHANGED;
}
