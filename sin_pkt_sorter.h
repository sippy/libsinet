struct sin_pkt_sorter *sin_pkt_sorter_ctor(
  void (*consume_default)(struct sin_list *, void *), void *consume_arg,
  int *sin_err);
void sin_pkt_sorter_dtor(struct sin_pkt_sorter *);
int sin_pkt_sorter_reg(struct sin_pkt_sorter *inst,
  int (*taste)(struct sin_pkt *), void (*consume)(struct sin_list *, void *),
  void *consume_arg, int *sin_err);

void sin_pkt_sorter_proc(struct sin_pkt_sorter *spsp, struct sin_list *pl);

