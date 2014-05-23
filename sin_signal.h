struct sin_signal;

struct sin_signal *sin_signal_ctor(int signum, int *sin_err);
void sin_signal_dtor(struct sin_signal *ssign);
int sin_signal_get_signum(struct sin_signal *ssign);

