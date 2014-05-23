struct netmap_slot;

struct sin_pkt
{
    struct sin_type_linkable t;
    struct netmap_slot *nm_slot;
    struct timeval ts;
    char *buf;
    int len;
};

struct sin_pkt *sin_pkt_ctor(struct netmap_slot *nm_slot, int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);

