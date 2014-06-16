struct sin_pkt;
struct sin_pkt_zone;
struct sin_rx_thread;
struct sin_tx_thread;
struct sin_pkt_sorter;
struct sin_ringmon_thread;

struct wrk_set {
    struct netmap_ring *tx_ring;
    struct netmap_ring *rx_ring;
    struct sin_pkt_zone *rx_zone;
    struct sin_pkt_zone *tx_zone;
    struct sin_tx_thread *tx_thread;
    struct sin_rx_thread *rx_thread;
    struct sin_pkt_sorter *rx_sort;
};

struct sin_stance {
    struct sin_type t;
    unsigned int sin_nref;
    int netmap_fd;
    int nrings;
    void *mem;
    struct netmap_if *nifp;
    struct wrk_set *phy;
    struct wrk_set hst;
    struct sin_ringmon_thread *rx_mon_thread;
};
