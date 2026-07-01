#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/types.h>



/*
 * ========================================================================
 * VSCODE INTELLISENSE COMPATIBILITY LAYER
 * ========================================================================
 * This block is ONLY read by VSCode IntelliSense to suppress "undefined" errors.
 * Real BCC compilation completely ignores this block, preventing conflicts.
 */
#ifdef __INTELLISENSE__
    /* We simulate the structures so VSCode knows these objects have a '.lookup' method */
    struct dummy_map {
        void* (*lookup)(void* key);
        int (*update)(void* key, void* value);
        int (*delete)(void* key);
    };
    #define BPF_HASH(name, key_t, val_t)   struct dummy_map name
    #define BPF_ARRAY(name, val_t, size)   struct dummy_map name
    #define lock_xadd(ptr, val)            ((void)0)
    #define SEC(name)                      __attribute__((section(name)))
#endif

#ifndef SEC
    #define SEC(name) __attribute__((section(name), used))
#endif


/* * Explicitly define map sizes using preprocessor identifiers.
 * This fixes the VSCode IntelliSense warning where it expects an identifier 
 * instead of a raw integer literal inside the BCC macro parameters.
 */
#define STATS_MAP_SIZE          2
#define DEFAULT_POLICY_MAP_SIZE 1

/*
 * ========================================================================
 * eBPF MAP DEFINITIONS (Compatible with the Python Firewall class bindings)
 * ========================================================================
 * CRITICAL FOR BCC: Keep plain token types (u32, u8, u16, u64) inside the macros.
 * BCC text parser handles these directly. Do NOT use typedefs before these macros.
 */

/* * Map containing the list of blocked IPv4 addresses. */
BPF_HASH(blocked_ips, u32, u8);

/* * Map containing the list of blocked TCP ports. */
BPF_HASH(blocked_tcp_ports, u16, u8);

/* * Map containing the list of blocked UDP ports. */
BPF_HASH(blocked_udp_ports, u16, u8);

/* * Map for tracking packet statistics. */
BPF_ARRAY(stats, u64, STATS_MAP_SIZE);

/* * Map for counting traffic hits from individual IP addresses. */
BPF_HASH(ip_counters, u32, u64);

/* * Map for the firewall default rule configuration policy. */
BPF_ARRAY(default_policy, u8, DEFAULT_POLICY_MAP_SIZE);


/*
 * ========================================================================
 * HELPER FUNCTIONS
 * ========================================================================
 */

/* * Increments a 64-bit counter safely inside a BPF Array map.
 * Used primarily for updating the global traffic analytics tables.
 * FIXED: Replaced 'u32' with native '__u32' for seamless compilation, 
 * and fixed the typo 'cou64unter' back to 'counter'.
 */
static inline void increment_stats_counter(__u32 index) {
    __u64 *counter = stats.lookup(&index);
    if (counter) {
        lock_xadd(counter, 1); /* Atomically increments the value to prevent multi-core race conditions */
    }
}


/*
 * ========================================================================
 * MAIN XDP PROGRAM HOOK
 * ========================================================================
 */

int xdp_firewall(struct xdp_md *ctx) {
    /* Define memory pointers to frame boundaries inside the socket buffer network packet */
    void *data_end = (void *)(long)ctx->data_end;
    void *data     = (void *)(long)ctx->data;

    /* Parse Layer 2: Ethernet Header validation and safety bounds checks */
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) {
        return XDP_PASS; /* Packet is corrupted or truncated, let the standard network stack drop or handle it */
    }

    /* Verify if the Layer 3 protocol carried inside the frame is IPv4 */
    if (eth->h_proto != bpf_htons(ETH_P_IP)) {
        return XDP_PASS; /* Non-IP traffic (e.g. ARP, IPv6) bypasses this specific firewall module */
    }

    /* Parse Layer 3: IPv4 Header extraction */
    struct iphdr *ip = data + sizeof(struct ethhdr);
    if ((void *)(ip + 1) > data_end) {
        return XDP_PASS; /* Malformed IP header length boundary overflow protection */
    }

    /* Track the incoming Source IP inside the per-IP tracking table */
    __u32 src_ip = ip->saddr;
    __u64 *ip_counter = ip_counters.lookup(&src_ip);
    if (ip_counter) {
        lock_xadd(ip_counter, 1); /* Increment existing connection tracking records */
    } else {
        __u64 initial_count = 1;
        ip_counters.update(&src_ip, &initial_count); /* Initialize tracking record slot for new host */
    }

    /* Check if the Source IP address is registered inside the blocked list hash map */
    __u8 *ip_blocked = blocked_ips.lookup(&src_ip);
    if (ip_blocked) {
        /* Source IP matched a block rule. Increment dropped index statistic and drop packet immediately */
        increment_stats_counter(1);
        return XDP_DROP;
    }

    /* Parse Layer 4: Identify Transport Layer protocols (TCP or UDP) to enforce port filtering rules */
    if (ip->protocol == IPPROTO_TCP) {
        /* Extract TCP structural segment offsets from IP header bounds */
        struct tcphdr *tcp = (void *)ip + (ip->ihl * 4);
        if ((void *)(tcp + 1) > data_end) {
            return XDP_PASS; /* Incomplete or fragmented TCP header context */
        }

        /* Look up destination port directly inside the network byte ordered maps registers */
        __u16 dest_port = bpf_ntohs(tcp->dest);
        __u8 *tcp_port_blocked = blocked_tcp_ports.lookup(&dest_port);
        if (tcp_port_blocked) {
            increment_stats_counter(1); /* Destination port is blocked, log violation counter */
            return XDP_DROP;            /* Deny transmission path */
        }
    } 
    else if (ip->protocol == IPPROTO_UDP) {
        /* Extract UDP structural datagram offsets from IP header bounds */
        struct udphdr *udp = (void *)ip + (ip->ihl * 4);
        if ((void *)(udp + 1) > data_end) {
            return XDP_PASS; /* Incomplete or fragmented UDP datagram protocol wrapper */
        }

        /* Look up destination port directly inside the network byte ordered maps registers */
        __u16 dest_port = bpf_ntohs(udp->dest);
        __u8 *udp_port_blocked = blocked_udp_ports.lookup(&dest_port);
        if (udp_port_blocked) {
            increment_stats_counter(1); /* Destination port is blocked, log violation counter */
            return XDP_DROP;            /* Deny transmission path */
        }
    }

    /* Default Policy Fallback Enforcement Layer */
    __u32 policy_idx = 0;
    __u8 *policy = default_policy.lookup(&policy_idx);
    if (policy && *policy == 1) {
        /* Normal operation flow: policy is set to ALLOW. Track packet acceptance metadata and pass up */
        increment_stats_counter(0);
        return XDP_PASS;
    }

    /* If default policy map is unavailable or modified away from 1, default safety action drops the frame */
    increment_stats_counter(1);
    return XDP_DROP;
}

/* Required open source software component kernel license attribute identifier */
char _license[] SEC("license") = "GPL";