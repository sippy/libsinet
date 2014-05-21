struct sin_stance {
    unsigned int sin_type;
    int netmap_fd;
    void *mem;
    struct netmap_if *nifp;
    struct netmap_ring *tx_ring;
    struct netmap_ring *rx_ring;
};
