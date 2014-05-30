struct sin_pkt_zone;
struct netmap_ring;

struct sin_pkt
{
    struct sin_type_linkable t;
    struct sin_pkt_zone *my_zone;
    struct netmap_ring *my_ring;
    struct netmap_slot *my_slot;
    unsigned int zone_idx;
    struct timeval *ts;
    char *buf;
    unsigned int len;
};

struct sin_pkt *sin_pkt_ctor(struct sin_pkt_zone *my_zone,
  int zone_idx, struct netmap_ring *my_ring, int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);

