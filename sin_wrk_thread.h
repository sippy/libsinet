struct sin_type_wrk_thread {
    unsigned int sin_type;
    pthread_t tid;
    char *tname;
    struct sin_wi_queue *ctrl_queue;
    struct sin_wi_queue *ctrl_notify_queue;
    struct sin_signal *sigterm;
    void *(*runner)(void *);
    char type_data[0];
};

int sin_wrk_thread_ctor(struct sin_type_wrk_thread *swtp, const char *tname,
  void *(*start_routine)(void *), int *sin_err);
void sin_wrk_thread_dtor(struct sin_type_wrk_thread *swtp);
int sin_wrk_thread_check_ctrl(struct sin_type_wrk_thread *swtp);
void sin_wrk_thread_notify_on_ctrl(struct sin_type_wrk_thread *swtp,
  struct sin_wi_queue *ctrl_notify_queue);

