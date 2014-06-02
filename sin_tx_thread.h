struct sin_tx_thread *sin_tx_thread_ctor(struct netmap_ring *tx_ring,
  struct sin_pkt_zone *tx_zone, int *sin_err);
void sin_tx_thread_dtor(struct sin_tx_thread *sttp);
struct sin_wi_queue *sin_tx_thread_get_out_queue(struct sin_tx_thread *sttp);

