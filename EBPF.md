# 🔬 Architectural Deep-Dive: Understanding eBPF & XDP Data Planes

This document breaks down the underlying kernel mechanisms leveraged by this firewall, explaining why it delivers unmatched performance compared to traditional user-space network filtering frameworks.

---

## 💡 What is eBPF?

**eBPF (Extended Berkeley Packet Filter)** is an architectural paradigm inside the Linux kernel that allows running sandboxed programs within the operating system core without changing kernel source files or loading risky, unstable kernel modules (`.ko`).

Historically, adding customized features to the networking stack meant writing out-of-tree kernel modules. If a module contained a memory management bug, it caused a severe **kernel panic** and crashed the system.

eBPF mitigates this risk completely via a multi-stage validation lifecycle:
1. **Bytecode Generation:** Restricted C code is compiled into eBPF bytecode using LLVM/Clang.
2. **The Kernel Verifier:** Before loading the bytecode, the kernel performs static analysis to ensure the code cannot loop endlessly, read out-of-bounds memory, or destabilize core system registers.
3. **Just-In-Time (JIT) Translation:** Once validated, the bytecode is compiled directly into native CPU hardware instructions (x86_64, ARM64), achieving near-zero processing overhead.

---

## 🚀 The XDP (Express Data Path) Performance Advantage

When processing network streams via standard networking layers (such as `iptables`, `nftables`, or standard user-space sockets), the operating system processes frames through multiple high-overhead steps:

<pre>
[ Raw Network Wire ] 
        │
        ▼
[ NIC Driver Registers DMA ]
        │
        ▼  &lt;─── ALLOCATES Complex sk_buff Struct &amp; Parses Whole Stack Here!
[ Linux Kernel Network Subsystem (sk_buff) ]
        │
        ├─► [ iptables / Netfilter Rules ]
        │
        ▼  &lt;─── Context Switches across Kernel-to-User Barrier!
[ User Space Application Socket ] (e.g., Python Firewall Process)
</pre>

This model requires allocating a complex **`sk_buff` (Socket Buffer)** memory metadata structure for every frame, incurring a noticeable performance penalty under heavy traffic load.

**XDP changes this paradigm completely.** It attaches the eBPF program directly to the lowest possible layer: inside the network interface driver, right when the network card receives the packet DMA ring buffer.

<pre>
[ Raw Network Wire ]
        │
        ▼
[ NIC Driver Registers DMA ] ──► [ XDP HOOK EXECUTES HERE ]
                                         │
                         ┌───────────────┴───────────────┐
                         ▼ (If Matched Rule)             ▼ (If Legitimate Traffic)
                   [ XDP_DROP ]                    [ XDP_PASS ]
               (Dropped at Wire Speed)                   │
                                                         ▼
                                            [ Standard Kernel Stack ]
</pre>

By intercepting packets here, the firewall can drop unwanted traffic (`XDP_DROP`) immediately, before any socket buffer allocation or deep kernel parsing takes place. This ensures predictable, sub-microsecond filtering latency and unparalleled resilience against volumetric DDoS floods.

---

## 🏁 Program Verdict Codes

After evaluating an incoming packet, the function `xdp_firewall` returns an explicit verdict token to the network driver interface:

* **`XDP_PASS`**: Indicates that the traffic is safe and verified. The packet is passed along to the standard Linux networking subsystem, which handles traditional IP routing, TCP state tracking, and application-layer delivery.
* **`XDP_DROP`**: Instructs the network driver to immediately discard the packet and reuse its memory page. This completely isolates user space from malicious traffic, maintaining high system performance even during active network attacks.

---

## 🔄 Stateful Bridging via BPF Maps

Because eBPF data planes operate inside a strict sandbox, they cannot call arbitrary user-space libraries or hold cross-packet global state variables. Instead, they share state across the kernel-user space boundary using **BPF Maps**.

Maps are high-performance, key-value memory arrays managed by the kernel, designed to be thread-safe across multi-core systems. This firewall utilizes two main map types:

1. **📂 `BPF_HASH` (Hash Tables):**
   * Used for `blocked_ips`, `blocked_tcp_ports`, and `blocked_udp_ports`.
   * The Python user space modifies these tables asynchronously to manage active rules.
   * The XDP module reads them via high-speed `.lookup()` queries for every incoming packet.
2. **📊 `BPF_ARRAY` (Index-Based Arrays):**
   * Used for tracking global `stats` counters.
   * The kernel increments the counters atomically using hardware instructions (`lock_xadd`) to prevent multi-core race conditions.
   * The Python daemon polls this array periodically to collect and display real-time traffic statistics.