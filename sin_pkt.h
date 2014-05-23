struct netmap_slot;

struct sin_pkt
{
    struct sin_type_linkable t;
    int zone_idx;
    struct timeval ts;
    char *buf;
    int len;
};

struct sin_pkt *sin_pkt_ctor(int zone_idx, int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);

