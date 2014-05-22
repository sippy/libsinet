struct sin_pkt;
struct netmap_slot;

struct sin_pkt *sin_pkt_ctor(struct netmap_slot *nm_slot, int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);

