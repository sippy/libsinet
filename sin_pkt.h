struct sin_pkt_zone;

struct sin_pkt
{
    struct sin_type_linkable t;
    struct sin_pkt_zone *my_zone;
    unsigned int zone_idx;
    struct timeval ts;
    char *buf;
    unsigned int len;
};

struct sin_pkt *sin_pkt_ctor(struct sin_pkt_zone *my_zone, int zone_idx,
  int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);

