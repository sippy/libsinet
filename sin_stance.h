struct sin_pkt;
struct sin_pkt_zone;
struct sin_rx_thread;
struct sin_tx_thread;
struct sin_pkt_sorter;

struct sin_stance {
    struct sin_type t;
    unsigned int sin_nref;
    int netmap_fd;
    void *mem;
    struct netmap_if *nifp;
    struct netmap_ring *tx_phy_ring;
    struct netmap_ring *rx_phy_ring;
    struct sin_pkt_zone *rx_phy_zone;
    struct sin_pkt_zone *tx_phy_zone;
    struct sin_tx_thread *tx_phy_thread;
    struct sin_rx_thread *rx_phy_thread;
    struct sin_pkt_sorter *rx_phy_sort;
    struct netmap_ring *tx_hst_ring;
    struct netmap_ring *rx_hst_ring;
    struct sin_pkt_zone *rx_hst_zone;
    struct sin_pkt_zone *tx_hst_zone;
    struct sin_tx_thread *tx_hst_thread;
    struct sin_rx_thread *rx_hst_thread;
    struct sin_pkt_sorter *rx_hst_sort;
};
