struct sin_pkt_zone_pvt;
struct sin_list;

struct sin_pkt_zone {
    struct sin_type t;
    int netmap_fd;
    struct sin_pkt **first;
    struct sin_pkt **last;
    struct sin_pkt_zone_pvt *pvt;
};

struct sin_pkt_zone *sin_pkt_zone_ctor(struct netmap_ring *ring, int netmap_fd,
  int *sin_err);
void sin_pkt_zone_dtor(struct sin_pkt_zone *);
int sin_pkt_zone_lock(struct sin_pkt_zone *spzp);
int sin_pkt_zone_unlock(struct sin_pkt_zone *spzp);
void sin_pkt_zone_ret_all(struct sin_list *pl, void *arg);

