## libsinet - S is for "Speed"


This is libsinet, a lightweight userland implementation of the UDP/IPv4 stack
designed to plug into the [netmap](https://code.google.com/p/netmap/) framework.

### Design Objectives

 - multithread-safe, multithread-optimized
 - provably small CPU run-time overhead
 - high-PPS optimized
 - as close as socket(2)/kqueue(2) API (aka "netold") as possible
 - UDP/IPv4/layerII initially, IPv6 / TCP in mind

### Differences between libsinet and netold

 - Void pointer is used as an object descriptor instead of int
 - Error reporting is performed into user supplied int buffer
   user has an option to ignore that by providing NULL pointer instead
 - to facilitate zero-copying, all i/o is performed by library-managed 
   buffers linked directly into kernel rings. Fixed and limited number
   of those buffers are allocated during the ```sin_init()```. It is
   the applications duty to use those wisely, making sure that each each
   buffer produced by the ```sin_recv()``` call is either going to be consumed
   by the ```sin_send()``` method, or recycled by invoking 
   ```sin_pkt_release()```. If application needs more packets for sending
   data out than it gets from```sin_recv()``` then additional packets may
   be produced by ```sin_pkg_alloc()```

### Caveats

-  In order for pass-through mode to work between hardware and software rings, hardware checksum off-loading must be disabled. As an example, checksum off-loading can be disabled for the em(4) driver as follows:

```
    # ifconfig emX -rxcsum
    # ifconfig emX -txcsum
```
