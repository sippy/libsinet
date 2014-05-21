struct sin_stance {
    unsigned int sin_type;
    unsigned int sin_nref;
    int netmap_fd;
    void *mem;
    struct netmap_if *nifp;
    struct netmap_ring *tx_ring;
    struct netmap_ring *rx_ring;
};

void _sip_set_errno(struct sin_stance *sip, int sip_errno);

