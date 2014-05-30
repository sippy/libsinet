struct sin_type {
    unsigned int sin_type;
    char type_data[0];
};

struct sin_type_linkable {
    unsigned int sin_type;
    struct sin_type_linkable *sin_next;
    char type_data[0];
};

#define _SIN_TYPE_SINSTANCE     693750532
#define _SIN_TYPE_SOCKET        4061681943
#define _SIN_TYPE_ADDR          489855194
#define _SIN_TYPE_QUEUE         1319882625
#define _SIN_TYPE_EVENT         3336537370
#define _SIN_TYPE_PKT_ZONE      720778432
#define _SIN_TYPE_PKT           639956139
#define _SIN_TYPE_WI_QUEUE      1938993589
#define _SIN_TYPE_WRK_THREAD    1612654994
#define _SIN_TYPE_SIGNAL        229112560
#define _SIN_TYPE_PKT_SORTER	1943693179
#define _SIN_TYPE_ITERABLE      962174450

#ifdef SIN_DEBUG
# define SIN_TYPE_ASSERT(ssp, model_type) \
  assert((ssp)->t.sin_type == (model_type))
#else
# define SIN_TYPE_ASSERT(ssp, model_type) {}
#endif

#define SIN_TYPE_SET(ssp, type) {(ssp)->t.sin_type = (type);}

#define SIN_TYPE_LINK(cp, np) (cp)->t.sin_next = (struct sin_type_linkable *)(np)

#define SIN_TYPE_IS_LINKABLE(stp)  ((stp)->sin_type == _SIN_TYPE_PKT || \
  (stp)->sin_type == _SIN_TYPE_SIGNAL || (stp)->sin_type == _SIN_TYPE_ITERABLE)

#define SIN_ITER_NEXT(stlp)	((void *)((stlp)->t.sin_next))
