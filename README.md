# High-Throughput Multi-Tenant WebSocket Messaging Engine

A highly concurrent, multi-tenant real-time messaging server engineered in C++20 using the Crow microframework, asynchronous network I/O primitives, and a structured JSON application protocol. 

The engine implements dynamic in-memory room routing, structural capacity caps, and optimized thread-synchronization boundaries to guarantee absolute memory safety under aggressive load profiles.

## Architectural & Concurrency Highlights

* **Structured Application Protocol:** Shifted from raw text streaming to a type-safe, structured JSON communication packet design utilizing Crow's native `crow::json::rvalue` data mapping parser.
* **The "Lock-and-Drop" Synchronization Pattern:** Solved thread-blockade vulnerabilities by keeping critical sections microscopic. Outbound message targets are snapshot-copied from global state maps into a continuous, cache-friendly `std::vector` under a brief RAII `std::lock_guard`, releasing the mutex instantly *before* executing heavy outbound network socket transmissions (`.send_text()`).
* **Bidirectional Index Topography:** Leverages a hybrid memory layer ($O(1)$ `std::unordered_map` and `std::unordered_set`) to maintain instantaneous user lookup tracks while optimizing sequential data iteration mechanics across multi-core CPU cache lines.
* **Deterministic Resource Management:** Explicitly detaches connection handles during the `.on_close()` lifecycle phase, neutralizing memory leaks and preventing dangling pointer mutations (`Segmentation Faults`).

## Protocol Specification (JSON Command Plane)

### 1. Join/Switch Room Context
Moves a connection pointer out of its current room registry and drops it into a target room if space permits.
* **Request Packet:**
  ```json
  {
    "action": "join_room",
    "room_name": "crypto_desk"
  }
Success Frame: {"status": "success", "message": "Successfully migrated to room: crypto_desk"}

Cap Rejection Frame: {"status" : "denied" , "error": "Target room capacity maxed"}

### 2. Targeted Cross-Room Message Broadcast
Routes an outbound string payload to all active client references within a specific room set while cleanly isolating unrelated channel states.

Request Packet:

JSON
{
  "action": "send_message",
  "room_name": "crypto_desk",
  "message": "Market velocity spiking."
}
Broadcast Distribution Payload: {"from_room": "crypto_desk", "msg": "Market velocity spiking."}

Project Tree Structure
Plaintext
├── main.cpp          # Core C++ Concurrent Engine & JSON Dispatcher
├── index.html        # Multitenant JavaScript Testing Interface
├── stress_test.yaml  # Artillery Load-Testing Configuration Profile
└── build/            # Compiled Target Binaries & Makefiles
Running & Testing the Architecture

1. Rebuild and Launch the Core System
Bash
mkdir -p build && cd build
cmake ..
make
./server
2. Boot the Client Interface Domain
To bypass browser security sandboxes restricting local file system script access (file://), serve the static asset interface over a dedicated local loop domain:

Bash
python3 -m http.server 3000
Open multiple isolated browser tabs side-by-side to http://localhost:3000 to test room segregation and capacity thresholds (MAX_ROOM_CAPACITY safety limits).

3. Verify System Stress Thresholds
Bash
npx artillery run stress_test.yaml
