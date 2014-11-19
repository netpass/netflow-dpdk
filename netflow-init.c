#include "netflow-init.h"

#include "probe.h"
#include "rte_table_netflow.h"

static void
print_ethaddr(const char *name, const struct ether_addr *eth_addr)
{
    printf ("%s%02X:%02X:%02X:%02X:%02X:%02X\n", name,
        eth_addr->addr_bytes[0],
        eth_addr->addr_bytes[1],
        eth_addr->addr_bytes[2],
        eth_addr->addr_bytes[3],
        eth_addr->addr_bytes[4],
        eth_addr->addr_bytes[5]);
}

#define NETFLOW_HASH_ENTRIES 4 * 1024 * 1024

static void
setup_netflow_table(probe_t* p)
{
    struct rte_table_netflow_params param = {
        .n_entries = NETFLOW_HASH_ENTRIES,
        .offset = 0,
        .f_hash = rte_hash_crc_4byte,
        .seed = 0,
    };
   
    
    int i,j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            p->table[i][j] = (struct rte_table_netflow *)rte_table_netflow_create(&param, j, sizeof(hashBucket_t));
        }
    }
}   

int
init_memory(unsigned nb_mbuf, uint8_t pid)
{
    uint8_t lid;    // lcore_id
    int sid;        // socket_id
    int ret;
    uint32_t    q;  
    char s[64];

    RTE_LCORE_FOREACH_SLAVE(lid) {
        if (rte_lcore_is_enabled(lid) == 0)
            continue;
        if (numa_on)
            sid = rte_lcore_to_socket_id(lid);
        else
            sid = 0;

        /* mempool */
        if (pktmbuf_pool[sid] == NULL) {
            snprintf(s, sizeof(s), "netflow_pool_%d", sid);
            pktmbuf_pool[sid] =
                rte_mempool_create(s, nb_mbuf, MBUF_SIZE, MEMPOOL_CACHE_SIZE,
                    sizeof(struct rte_pktmbuf_pool_private),
                    rte_pktmbuf_pool_init, NULL,
                    rte_pktmbuf_init, NULL,
                    sid, 0);
            if (pktmbuf_pool[sid] == NULL)
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool on socket(%d)\n", sid);
        }

        /* mbuf pool */
        for (q = 0; q < 1; q++) {
            ret = rte_eth_rx_queue_setup(pid, q, 512, sid, &rx_conf, pktmbuf_pool[sid]);
            if (ret < 0)
                rte_exit(EXIT_FAILURE, "Failed to rx_queue_setup\n");
        }
        for (q = 0; q < 1; q++) {
            ret = rte_eth_tx_queue_setup(pid, q, 128, sid, &tx_conf);
        }
    }

}

int
netflow_init(probe_t *probe)
{
    probe->nb_ports = rte_eth_dev_count();
    uint8_t pid;    // port_id
    uint8_t ret;

    RTE_LOG(DEBUG, PMD, "Number of ports: %d\n", probe->nb_ports);    

    /* init Port */
    for (pid = 0; pid < probe->nb_ports; pid++) {
        RTE_LOG(DEBUG, PMD, "Init Port(%d)\n", pid);

        ret = rte_eth_dev_configure(pid, probe->nb_queues, 1, &port_conf);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%d\n", ret, pid);

            rte_eth_macaddr_get(pid, &probe->ports_eth_addr[pid]);
            print_ethaddr("MAC address:", &probe->ports_eth_addr[pid]);

        /* init memory per port */
        if (init_memory(NB_MBUF, pid) < 0)
            rte_exit(EXIT_FAILURE, "Fail to initialize memory\n");

        /* start device */
        ret = rte_eth_dev_start(pid);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Fail to start dev\n");

        rte_eth_promiscuous_enable(pid);
    }

    /* netflow hash table init */
    setup_netflow_table(probe);

    /* setup netflow collector information */
    probe->collector.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&probe->collector.servaddr, sizeof(probe->collector.servaddr));
    probe->collector.servaddr.sin_family = AF_INET;
    probe->collector.servaddr.sin_addr.s_addr = inet_addr(probe->collector.addr);
    probe->collector.servaddr.sin_port = rte_cpu_to_be_16(probe->collector.port);

 
printf("----------- MEMORY_SEGMENTS -----------\n");
rte_dump_physmem_layout(stdout);
printf("--------- END_MEMORY_SEGMENTS ---------\n");
printf("------------ MEMORY_ZONES -------------\n");
rte_memzone_dump(stdout);
printf("---------- END_MEMORY_ZONES -----------\n");
printf("---------- TAIL_QUEUES ----------------\n");
rte_dump_tailq(stdout);
 
    return 0;
}
