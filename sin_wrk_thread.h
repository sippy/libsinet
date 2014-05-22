struct sin_type_wrk_thread {
    unsigned int sin_type;
    pthread_t tid;
    const char *tname;
    char type_data[0];
};
