struct sin_pkt_sorter;
struct netmap_ring;
struct sin_pkt_zone;

struct sin_rx_thread *sin_rx_thread_ctor(struct netmap_ring *rx_ring,
  struct sin_pkt_zone *rx_zone, struct sin_pkt_sorter *rx_sort, int *sin_err);
void sin_rx_thread_dtor(struct sin_rx_thread *srtp);
