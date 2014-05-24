#ifndef _SIN_WI_QUEUE_H_
#define _SIN_WI_QUEUE_H_

struct sin_wi_queue;
struct sin_list;

struct sin_wi_queue *sin_wi_queue_ctor(int *sin_err,
  const char *format, ...);
void sin_wi_queue_dtor(struct sin_wi_queue *queue);

void sin_wi_queue_put_item(void *wi, struct sin_wi_queue *);
void sin_wi_queue_put_items(struct sin_list *lst, struct sin_wi_queue *);
int sin_wi_queue_pump(struct sin_wi_queue *);
void *sin_wi_queue_get_item(struct sin_wi_queue *queue, int waitok,
  int return_on_wake);
struct sin_list *sin_wi_queue_get_items(struct sin_wi_queue *queue,
  struct sin_list *lst, int waitok, int return_on_wake);

#if 0
void rtpp_queue_pump(struct rtpp_queue *);

struct rtpp_wi *rtpp_queue_get_item(struct rtpp_queue *queue, int return_on_wake);
int rtpp_queue_get_items(struct rtpp_queue *, struct rtpp_wi **, int, int);
int rtpp_queue_get_length(struct rtpp_queue *);
#endif

#endif

