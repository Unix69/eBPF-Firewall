# 🚀 Real-World Production Use Cases & Code Recipes

This document provides standalone, ready-to-run Python recipes demonstrating how to integrate and orchestrate the eBPF/XDP Firewall module within programmatic infrastructure environments.

---

## 🛑 Use Case 1: Automated SSH Brute-Force Mitigator
This reactive script parses local system authentication logs (e.g., `/var/log/auth.log`) in a non-blocking thread and immediately drops source IP addresses at wire-speed after a threshold of failed authentication attempts is crossed.

<pre><code class="language-python">import time
import re
from firewall import firewall, FirewallState

# Configuration parameters
FAILED_THRESHOLD = 5
BAD_HOST_TRACKER = {}

def log_parser_callback(log_line):
    # Match standard OpenSSH failed password patterns
    match = re.search(r"Failed password for .* from (\d+\.\d+\.\d+\.\d+)", log_line)
    if match:
        source_ip = match.group(1)
        BAD_HOST_TRACKER[source_ip] = BAD_HOST_TRACKER.get(source_ip, 0) + 1
        
        print(f"[MONITOR] Failed login detected from {source_ip}. Count: {BAD_HOST_TRACKER[source_ip]}")
        
        if BAD_HOST_TRACKER[source_ip] >= FAILED_THRESHOLD:
            print(f"⚠️ [MITIGATION] Host {source_ip} crossed threshold. Injecting XDP drop block!")
            firewall.block_ip(source_ip)

def monitor_auth_logs():
    # Initialize the eBPF/XDP driver stack
    firewall.start()
    time.sleep(2)
    
    if firewall.state != FirewallState.RUNNING:
        print("❌ Failed to bind eBPF kernel module.")
        return

    print("🕵️ Reactive log monitor running. Parsing stream...")
    try:
        # Emulate tailing an active logging system descriptor
        with open("/var/log/auth.log", "r") as log_stream:
            # Shift cursor directly to the end of the existing file footprint
            log_stream.seek(0, 2)
            while True:
                line = log_stream.readline()
                if not line:
                    time.sleep(0.5)
                    continue
                log_parser_callback(line)
    except KeyboardInterrupt:
        print("\nStopping log monitor daemon...")
    finally:
        firewall.stop()

if __name__ == "__main__":
    monitor_auth_logs()</code></pre>

---

## ⏰ Use Case 2: Time-Based Network Access Schedule
This implementation enforces access restrictions based on business hour windows. For example, it opens an internal administration endpoint during specific periods and enforces a complete structural drop policy outside of those hours.

<pre><code class="language-python">import datetime
import time
from firewall import firewall

# Define operational access parameters
RESTRICTED_PORT = 22  # Protected infrastructure port (e.g., SSH)
START_HOUR = 9        # Access opens at 09:00
END_HOUR = 18         # Access closes at 18:00

def enforce_schedule():
    firewall.start()
    time.sleep(2)
    
    # Pre-emptively apply the network port block at startup
    is_blocked = True
    firewall.block_port(RESTRICTED_PORT, proto="tcp")
    print(f"[SCHEDULE] Port {RESTRICTED_PORT} initialized in BLOCKED state.")

    try:
        while True:
            current_hour = datetime.datetime.now().hour
            
            if START_HOUR <= current_hour < END_HOUR:
                if is_blocked:
                    # Business hours active: lift the restriction
                    firewall.unblock_port(RESTRICTED_PORT, proto="tcp")
                    is_blocked = False
                    print(f"🔓 [SCHEDULE] Business hours active ({current_hour}:00). Access opened.")
            else:
                if not is_blocked:
                    # Outside business hours: re-enforce the restriction
                    firewall.block_port(RESTRICTED_PORT, proto="tcp")
                    is_blocked = True
                    print(f"🔒 [SCHEDULE] Non-business hours reached ({current_hour}:00). Access closed.")
            
            # Poll every 60 seconds to re-evaluate time constraints
            time.sleep(60)
            
    except KeyboardInterrupt:
        print("\nStopping scheduler script...")
    finally:
        firewall.stop()

if __name__ == "__main__":
    enforce_schedule()</code></pre>

---

## 🌐 Use Case 3: Integration with High-Traffic Web APIs
This blueprint shows how to integrate the asynchronous firewall inside an API application server (such as microservices or control panel management layers) to check telemetry parameters dynamically.

<pre><code class="language-python">import time
from firewall import firewall

def telemetry_microservice_worker():
    # Initialize connection endpoints
    firewall.start()
    time.sleep(2)
    
    # Establish base tracking matrices
    firewall.block_ip("185.220.101.10")  # Known malicious scraper or proxy IP
    
    print("[API Engine] Operational telemetry service initialized.")
    try:
        while True:
            # 1. Fetch live metrics from BPF Array structures
            stats = firewall.get_stats()
            
            # 2. Extract per-IP hit counts from the kernel space hash table
            counters = firewall.get_ip_counters()
            
            # Compile payload schema for external tracking dashboards
            payload = {
                "timestamp": time.time(),
                "metrics": stats,
                "monitored_hosts": len(counters),
                "hotspots": {ip: hits for ip, hits in counters.items() if hits > 500}
            }
            
            print(f"📊 [TELEMETRY PAYLOAD] {payload}")
            time.sleep(10)
            
    except KeyboardInterrupt:
        print("\nTearing down telemetry service framework...")
    finally:
        firewall.stop()

if __name__ == "__main__":
    telemetry_microservice_worker()</code></pre>