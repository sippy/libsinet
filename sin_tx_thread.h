struct sin_tx_thread *sin_tx_thread_ctor(struct sin_stance *sip, int *sin_err);
void sin_tx_thread_dtor(struct sin_tx_thread *sttp);
struct sin_wi_queue *sin_tx_thread_get_out_queue(struct sin_tx_thread *sttp);

