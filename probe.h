#ifndef __PROBE_H_
#define __PROBE_H_

#include <stdint.h>
#include <stdio.h>

#include <rte_lcore.h>
#include <rte_ethdev.h>
#include <rte_ip.h>

#define NETFLOW_APP_NAME        "Netflow DPDK"

#define MAX_PKT_BURST   16

typedef struct rte_eth_stats    eth_stats_t;

typedef enum { PACKET_CONSUMED = 0, UNKNOWN_PACKET = 0xEEEE, DROP_PACKET = 0xFFFE, FREE_PACKET = 0xFFFF } pktType_e;

typedef struct pkt_stats_s {
    uint64_t            arp_pkts;           /**< Number of ARP packets received */
    uint64_t            echo_pkts;          /**< Number of ICMP echo requests received */
    uint64_t            ip_pkts;            /**< Number of IPv4 packets received */
    uint64_t            ipv6_pkts;          /**< Number of IPv6 packets received */
    uint64_t            vlan_pkts;          /**< Number of VLAN packets received */
    uint64_t            dropped_pkts;       /**< Hyperscan dropped packets */
    uint64_t            unknown_pkts;       /**< Number of Unknown packets */
    uint64_t            tx_failed;          /**< Transmits that failed to send */
} pkt_stats_t;


typedef struct port_info_s {
    uint16_t                pid;                    /**< Port ID value */
    
    pkt_stats_t             stats;                      /**< Statistics for a number of stats */

    eth_stats_t             init_stats;             /**< Initial packet statistics      */
    eth_stats_t             port_stats;             /**< current port statistics        */
    eth_stats_t             rate_stats;             /**< current packet rate statistics */

    struct rte_eth_link     link;                   /**< Link information link speed and duplex */
} port_info_t;

typedef struct probe_s {
    //struct cmdline        *cli;
    char*                   *hostname;              /* hostname */
    uint8_t                 nb_ports;
    uint16_t                nb_rxd;
    uint16_t                nb_txd;
    uint16_t                portNum;
    struct ether_addr       ports_eth_addr[RTE_MAX_ETHPORTS];

    // port to lcore mapping
    //l2p_t                   *l2p;

    /* Statistics */
    port_info_t             info[RTE_MAX_ETHPORTS];     /**< Port Information                 */

} probe_t;

extern int launch_probe(__attribute__ ((unused)) void * arg);

#endif