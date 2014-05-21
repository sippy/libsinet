struct sin_pkt;

struct sin_pkt_zone;

struct sin_stance {
    unsigned int sin_type;
    unsigned int sin_nref;
    int netmap_fd;
    void *mem;
    struct netmap_if *nifp;
    struct netmap_ring *tx_ring;
    struct netmap_ring *rx_ring;
    struct sin_pkt_zone *rx_free;
    struct sin_pkt_zone *tx_free;
};
