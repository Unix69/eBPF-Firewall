# 🛡️ Automated High-Performance eBPF/XDP Network Firewall

A production-grade, asynchronous, programmable network security engine leveraging **Extended Berkeley Filter (eBPF)** and **Express Data Path (XDP)** inside the Linux kernel driver layer.

---

## 📊 Repository Badges
<p align="left">
  <img src="https://img.shields.io/badge/Kernel-5.4+-blue.svg?style=flat-square&logo=linux" alt="Linux Kernel">
  <img src="https://img.shields.io/badge/Language-Python%20%2F%20C-orange.svg?style=flat-square" alt="Language">
  <img src="https://img.shields.io/badge/Framework-BCC-red.svg?style=flat-square" alt="Framework">
  <img src="https://img.shields.io/badge/License-GPL%20%2F%20BSD--2-green.svg?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/Build-Passing-brightgreen.svg?style=flat-square" alt="Build">
</p>

---

## 📋 Table of Contents
1. [📚 Documentation Index & Navigation](#-documentation-index--navigation)
2. [✨ Core Features & Capabilities](#-core-features--capabilities)
3. [🛠️ Technical Stack](#%EF%B8%8F-technical-stack)
4. [📐 Architecture and Design Patterns](#-architecture-and-design-patterns)
5. [🚀 Installation & Prerequisites](#-installation--prerequisites)
6. [🚀 Real-World Use Cases & Blueprints](#-real-world-use-cases--blueprints)
7. [⚙️ Configuration & Environment](#%EF%B8%8F-configuration--environment)
8. [💻 Usage Guide & Integration API](#-usage-guide--integration-api)
9. [🔌 Extensibility & Customization](#-extensibility--customization)
10. [📄 License & Resources](#-license--resources)

---

## 📚 Documentation Index & Navigation

To help you navigate the system design, architectural parameters, and open-source practices, our documentation framework is divided into specialized modules. Please refer to these specific files for deep technical investigations:

<table>
  <thead>
    <tr>
      <th>📄 Document Reference</th>
      <th>🎯 Target Technical Scope</th>
      <th>🛠️ Primary Audience</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><a href="./eBPF.md">🔬 eBPF &amp; XDP Deep-Dive (eBPF.md)</a></td>
      <td>Theoretical kernel-space layers, packet context execution loops, XDP verdict tracking, and high-speed atomic memory operations.</td>
      <td>Network Engineers, Kernel Architects</td>
    </tr>
    <tr>
      <td><a href="./INTEGRATION.md">🔌 Advanced Integration (INTEGRATION.md)</a></td>
      <td>Finite State Machine lifecycle tokens, rule serialization matrices, cross-cluster state persistence, and automated self-healing.</td>
      <td>Backend Developers, Devops &amp; SREs</td>
    </tr>
    <tr>
      <td><a href="./USE_CASES.md">🚀 Production Recipes (USE_CASES.md)</a></td>
      <td>Fully detailed, ready-to-run automation recipes including adaptive IDS logic, scheduling hooks, and web telemetry handlers.</td>
      <td>Security Operators, Software Engineers</td>
    </tr>
    <tr>
      <td><a href="./CONTRIBUTING.md">👩‍💻 Collaboration Guide (CONTRIBUTING.md)</a></td>
      <td>Isolated development environment workflows, VM testing safety rules, custom git prefixes, and pull request steps.</td>
      <td>Repository Contributors, Reviewers</td>
    </tr>
  </tbody>
</table>

---

## ✨ Core Features & Capabilities

* **⚡ Wire-Speed IP Blacklisting:** Inbound malicious IPv4 packets are evaluated and dropped before the kernel allocates complex socket buffer (`sk_buff`) data structures, neutralizing resource-exhaustion DDoS/DoS attacks. For theoretical details, visit the [🔬 eBPF Subsystem Documentation](./eBPF.md).
* **🔒 Layer-4 Transport Inspection:** Enforces deep frame introspection to drop unauthorized packets across both **TCP** and **UDP** destination ports.
* **🎛️ Dynamic Default Policy:** Toggles the firewall fallback behavior (PASS or DROP) on-the-fly directly from the user-space controller.
* **📈 Real-Time Telemetry Maps:** Collects global processing metrics (allowed vs. dropped packets) combined with dedicated per-IP hit counters.
* **🩺 Autonomous Self-Healing:** A dedicated background daemon monitors system health, catches runtime faults or interface detachments, and initiates transparent state recovery workflows. Review the full architecture inside the [🔌 Integration Guide](./INTEGRATION.md).

---

## 🛠️ Technical Stack

* **Data Plane (Kernel Space):** Restricted C executed within a secure, JIT-compiled sandbox environment verified by the eBPF Kernel Verifier. Check the source setup inside [xdp_firewall.c](./xdp_firewall.c).
* **Control Plane (User Space):** Object-Oriented Python 3.10+ implementing Finite State Machines, automated exception token handling, and binary network serialization. Look up components inside [firewall.py](./firewall.py).
* **Bindings Subsystem:** BCC (BPF Compiler Collection) API for live compilation, map descriptor polling, and kernel hook anchoring.

---

## 📐 Architecture and Design Patterns

The system maintains a strict separation of concerns, decoupling wire-speed packet processing (Data Plane) from management orchestrations (Control Plane) via transactional **BPF Maps**.

### 🧬 Data Plane & Control Plane Pipeline
<pre>
+--------------------------------------------------------+
|                  USER SPACE (Python)                   |
|   +------------------------------------------------+   |
|   |   Control Plane Orchestrator (Firewall Class)  |   |
|   +-------+--------------------------------+-------+   |
+-----------|--------------------------------|-----------+
            | Reads Telemetry                | Writes Active Rules
            | & Host Traffic Counters        | (blocked_ips, etc.)
+-----------|--------------------------------|-----------+
|           v                                v           |
|     +-----------+                    +-----------+     |
|     | BPF Maps  |                    | BPF Maps  |     |
|     |  (stats)  |                    | (filters) |     |
|     +-----^-----+                    +-----^-----+     |
|           | Atomic                         | Fast      |
|           | Updates                        | Lookups   |
|   +-------+--------------------------------+-------+   |
|   |           eBPF / XDP Data-Plane Module         |   |
|   |         (Restricted C - xdp_firewall.c)        |   |
|   +------------------------------------------------+   |
|                           ^                            |
|                           | Inbound Raw Packets        |
|                  [Network Interface Card]              |
+--------------------------------------------------------+
</pre>

### 1. ⚙️ Kernel Space Subsystem (`xdp_firewall.c`)
The C module extracts packet boundaries, validates Layer 2 Ethernet headers, parses Layer 3 IPv4 addresses, records connection logs inside `ip_counters`, queries rule matrices (`blocked_ips`), resolves transport constraints (TCP/UDP), and applies fallback enforcement logic based on the `default_policy` map.
* *Developer Patch:* Includes a custom `#ifdef __INTELLISENSE__` abstraction layer that mocks BPF structures locally, allowing modern IDEs (like VSCode) to provide full autocomplete capabilities without reporting syntax errors.

### 2. 🐍 User Space Controller (`firewall.py`)
Encapsulates runtime lifecycles inside an explicitly managed Finite State Machine supporting specific operational tokens (`INIT`, `STARTING`, `RUNNING`, `STOPPING`, `STOPPED`, `RESTARTING`, `FAILED`).
* Utilizes network structural libraries (`socket`, `struct`) to reliably translate standard dot-decimal strings into Network Byte Order (Big Endian) 32-bit integers processed by eBPF maps.
* Implements a comprehensive `RecoveryManager` linking custom exceptions (e.g., `FirewallThreadException`) to precise recovery actions (`FirewallThreadRecoveryAction`) to clear stale state objects and securely restart worker threads. More layout patterns are detailed inside the [🔌 Integration Guide](./INTEGRATION.md).

---

## 🚀 Installation & Prerequisites

Because eBPF interacts directly with kernel parameters, administrative privileges (`sudo`) and a modern Linux kernel engine (5.4+) are required. For code safety prerequisites, ensure you check the [👩‍💻 Collaboration Guide](./CONTRIBUTING.md).

### 1. Install System Tools & Libraries (Ubuntu/Debian)
<pre><code class="language-bash">sudo apt update
sudo apt install -y bpfcc-tools libbpfcc-dev linux-headers-$(uname -r) llvm clang</code></pre>

### 2. Install Python Dependencies
Deploy necessary user-space packages using pip tracking our `requirements.txt` file footprint:
<pre><code class="language-bash">sudo pip3 install -r requirements.txt</code></pre>

---

## 🚀 Real-World Use Cases & Blueprints

To help you accelerate production deployment, we have compiled a dedicated cookbook containing complete, ready-to-run automation blueprints. These examples demonstrate how the eBPF/XDP engine interfaces with higher-level business logic.

Detailed implementations can be found in our comprehensive [🚀 USE_CASES.md](./USE_CASES.md) guide, which covers the following scenarios:

### 🕵️ 1. Automated SSH Brute-Force Mitigator
An asynchronous security worker that tails local authentication logs (e.g., `/var/log/auth.log`), detects multi-attempt authentication failures, and instantly injects a high-speed XDP drop block against the offensive host.
* **Core Concepts:** Asynchronous log trailing, regex matching, runtime map insertion.
* **Implementation Blueprint:** See [Use Case 1 in USE_CASES.md](./USE_CASES.md#-use-case-1-automated-ssh-brute-force-mitigator).

### ⏰ 2. Time-Based Network Access Schedule
A script designed to restrict or grant access to exposed infrastructure elements (such as maintenance or database ports) according to explicit office hour schedules.
* **Core Concepts:** Scripted scheduling, automated block cycling, stateful cleanup.
* **Implementation Blueprint:** See [Use Case 2 in USE_CASES.md](./USE_CASES.md#-use-case-2-time-based-network-access-schedule).

### 📊 3. High-Traffic Web API Integration
A telemetry module connecting directly to operational worker instances, collecting tracking logs, and reporting active memory statistics to analytics dashboards.
* **Core Concepts:** Map pooling, JSON web payloads, real-time hotspot auditing.
* **Implementation Blueprint:** See [Use Case 3 in USE_CASES.md](./USE_CASES.md#-use-case-3-integration-with-high-traffic-web-apis).

---

## ⚙️ Configuration & Environment

The component looks for specific environment variables to target interfaces, falling back to secure defaults if none are provided:

| Environment Variable | Target Property Reference | Default Value |
| :--- | :--- | :--- |
| `FIREWALL_INTERFACE` | Network Interface card to hook the XDP driver. | `eth0` |
| `FIREWALL_EBPF_C_FILE`| Relative path pointing to the kernel C file. | `xdp_firewall.c` |
| `FIREWALL_RUN_CYCLE` | Periodic stats gathering and health-check loop delay (seconds). | `5` |

To apply custom parameters, export them before executing the main process:
<pre><code class="language-bash">export FIREWALL_INTERFACE="enp4s0"
export FIREWALL_RUN_CYCLE="3"
sudo -E python3 firewall.py</code></pre>

---

## 💻 Usage Guide & Integration API

### Programmatic Integration API

You can import and control the `Firewall` object within your own backend microservices, daemon setups, or custom management layers. Advanced structural examples are provided in the [🔌 INTEGRATION.md](./INTEGRATION.md) layout.

<pre><code class="language-python">from firewall import firewall, FirewallState
import time

# 1. Register application logging hooks via event listeners
firewall.add_listener(lambda ev: print(f"[FIREWALL-BUS] {ev.message} | State: {ev.type}"))

# 2. Compile the data plane and hook it into the kernel driver
firewall.start()
time.sleep(2) # Allow the compilation pipeline to stabilize

if firewall.state == FirewallState.RUNNING:
    print("\n🚀 Firewall is operational. Applying rules...")
    
    # 3. Block malicious network hosts (Handles endianness conversion automatically)
    firewall.block_ip("192.168.1.15")
    firewall.block_ip("10.0.0.200")
    
    # 4. Bind Layer-4 port rules to safeguard exposed listeners
    firewall.block_port(22, proto="tcp")    # Secure SSH interface
    firewall.block_port(53, proto="udp")    # Protect DNS streams
    
    # 5. Extract global packet processing metrics
    time.sleep(5)
    metrics = firewall.get_stats()
    print(f"\n📊 Traffic Statistics: {metrics}")
    
    # 6. Extract raw host connection hit counters
    counters = firewall.get_ip_counters()
    print(f"🎯 Connection Analytics: {counters}\n")
    
    # 7. Serialize runtime configuration schemas to disk
    firewall.export_rules("running_config.json")
    
    # 8. Lift connection constraints dynamically
    firewall.unblock_ip("192.168.1.15")
    firewall.unblock_port(22, proto="tcp")

# 9. Safely detach the XDP driver hook and restore default network behaviors
print("\n🛑 Shutting down firewall service...")
firewall.stop()</code></pre>

---

## 🔌 Extensibility & Customization

* **🔔 Third-Party Alerting:** Connect notification methods directly into the initialization configurations (`on_block_ip`, `on_stats_read_fail`, etc.) to forward security events to messaging webhooks, or external log collectors.
* **🌐 Network Range Masking (CIDR):** Swap out the standard `BPF_HASH` definition for a `BPF_LPM_TRIE` structure inside `xdp_firewall.c`, and perform key matching with longest-prefix-match helpers to target complete subnets.
* **🔄 Redundant State Synchronization:** Utilize the built-in `snapshot()` and `restore(state_dict)` methods to instantly replicate security rules across a cluster of servers. See explicit serialization details inside the [🔌 INTEGRATION.md](./INTEGRATION.md#%EF%B8%8F-state-persistence--cluster-synchronization) file.

---

## 📄 License & Resources

* **⚙️ Kernel Data Plane:** Distributed under the terms of the **GPL** License, which is mandatory to unlock and execute core internal helper function symbols within the Linux kernel space engine footprint.
* **🐍 Control Plane Wrapper:** Distributed under the terms of the permissive **BSD 2-Clause License** (Copyright (c) 2026, Unix69). You can review the official parameters inside the root repository tracking layout.
  
👉 **Read the full license agreement here:** [./LICENSE](./LICENSE)

### 🌐 Upstream Community Portals
* **🌐 eBPF Project Portal:** [ebpf.io](https://ebpf.io/)
* **💻 BCC Repository:** [iovisor/bcc](https://github.com/iovisor/bcc)
* **📚 XDP Reference Tutorial:** [XDP Project Guides](https://github.com/xdp-project/xdp-tutorial)

---