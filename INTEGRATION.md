# 🔌 Advanced Software Integration & State Architecture Guide

This guide details how to programmatically embed the `Firewall` class into complex software architectures (such as cloud security agents, system daemons, or cluster control panels) and outlines its automated recovery state machines.

---

## 🏗️ Architectural Life Cycle & State Machine

The Python `Firewall` core manages setup orchestration via an internal **Finite State Machine (FSM)**. To guarantee network stability during transitions or unexpected crashes, state progressions follow strict pathways governed by structural exception handlers:

<pre>
    [INIT]
      │
      ▼  (start() invoked)
  [STARTING] ──► (JIT Compilation / Hook Attach)
      │
      ├──► Success ──► [RUNNING] &lt;──┐ (Recovery Success)
      │                   │         │
      └──► Failure        ▼         │
            │       (Fault Detected)│
            ▼             │         │
        [FAILED] ◄──── [RESTARTING] ┘
            ▲             │
            │             ▼ (Unrecoverable Drift)
            └─────────────┘
</pre>

When integrating the object into an application server (e.g., a FastAPI backend or an enterprise monitoring agent), you can track these mutations by querying the state enum directly:

<pre><code class="language-python">from firewall import firewall, FirewallState

if firewall.state == FirewallState.RUNNING:
    # Trigger application hooks safely
    pass</code></pre>

---

## 🛠️ State Persistence & Cluster Synchronization

The firewall supports exporting its entire rule configuration state to disk or memory, allowing you to synchronize security rules across multiple servers or restore settings after a clean reboot.

### 1. Serializing Active States to JSON
The `.snapshot()` and `.export_rules()` mechanisms extract live configurations from the active kernel tables without disrupting packet filtering operations:

<pre><code class="language-python"># Create an in-memory dictionary snapshot
current_snapshot = firewall.snapshot()
print(current_snapshot)

# Dump configurations directly to the local storage layer
firewall.export_rules("cluster_policy.json")</code></pre>

### 2. Live Runtime Restoration
When launching a cluster backup or restarting a daemon process, you can instantly apply an existing configuration back into the kernel space:

<pre><code class="language-python"># Import rule schema and inject parameters during active execution
firewall.restore_from_file("cluster_policy.json")</code></pre>

---

## 🩺 Autonomous Fault Injection & Recovery Handling

The firewall includes a built-in `RecoveryManager` designed to automatically handle software exceptions and network drifts, preventing single points of failure (SPF) by mapping specific errors to dedicated remediation handlers:

### 🧩 Registered Error Actions

1. **`FirewallUnloadedException` ➔ `FirewallUnloadedRecoveryAction`**
   * *Trigger:* Occurs if an external command detaches the XDP hook from the network interface while the script is running.
   * *Remediation:* Re-compiles the C source file, re-attaches the XDP hook to the driver, and flushes the cached user-space rules back into the new kernel tables.
2. **`FirewallThreadException` ➔ `FirewallThreadRecoveryAction`**
   * *Trigger:* Occurs if the background statistics-polling thread crashes due to a resource constraint or signal interruption.
   * *Remediation:* Completely cleans up the stalled thread context, allocates a fresh, detached monitoring worker loop, and restarts the background polling mechanism.
3. **`InvalidIPAddressException` ➔ `InvalidIPRecoveryAction`**
   * *Trigger:* Occurs if a management tool tries to inject a corrupted or malformed string (e.g., `"999.999.999.999"`) into the drop rules.
   * *Remediation:* Blocks the invalid entry from entering the kernel memory maps, alerts the application space via an event broadcast, and keeps the rest of the engine running smoothly.