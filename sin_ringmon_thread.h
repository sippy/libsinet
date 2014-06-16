struct sin_pkt_sorter;
struct netmap_ring;
struct sin_pkt_zone;

struct sin_ringmon_thread *sin_ringmon_thread_ctor(const char *tnane,
  int netmap_fd, int *sin_err);
void sin_ringmon_thread_dtor(struct sin_ringmon_thread *srmtp);
