struct sin_pkt_zone {
    struct sin_type t;
    struct sin_pkt **first;
    struct sin_pkt **last;
    struct sin_pkt **curr;
};

struct sin_pkt_zone *sin_pkt_zone_ctor(struct netmap_ring *ring, int *sin_err);
void sin_pkt_zone_dtor(struct sin_pkt_zone *);
