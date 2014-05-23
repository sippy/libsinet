#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "sin_ip4_icmp.h"

struct icmphdr {
    u_char  icmp_type;              /* type of message, see below */
    u_char  icmp_code;              /* type sub code */
    u_short icmp_cksum;             /* ones complement cksum of struct */
    char    icmphdr_rest[4];
} __attribute__((__packed__));

struct ip4_icmp {
    struct ip iphdr;
    struct icmphdr icmphdr;
} __attribute__((__packed__));

struct ip4_icmp_en10t {
    uint8_t ether_dhost[6];
    uint8_t ether_shost[6];
    uint16_t ether_type;
    struct ip4_icmp ip4_icmp;
} __attribute__((__packed__));

#define SIN_IP4_ICMP_MINLEN	(14 + 20 + 16)

int
sin_ip4_icmp_taste(char *buf, int buflen)
{
    struct ip4_icmp_en10t *p;

    if (buflen < SIN_IP4_ICMP_MINLEN) {
        return (0);
    }
    p = (struct ip4_icmp_en10t *)buf;
    if (p->ether_type != htons(0x0800)) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_v != 0x4) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_p != 0x1) {
        return (0);
    }
    if (p->ip4_icmp.icmphdr.icmp_type != 0x0) {
        return (0);
    }
    return (1);
}
