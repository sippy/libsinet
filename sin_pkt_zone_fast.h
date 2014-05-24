static inline void
sin_pkt_zone_ret_pkt(struct sin_pkt *pkt, struct sin_pkt_zone *pzone)
{

#ifdef SIN_DEBUG
    assert(pzone->first[pkt->zone_idx] == NULL);
#endif
    pzone->first[pkt->zone_idx] = pkt;
} 
