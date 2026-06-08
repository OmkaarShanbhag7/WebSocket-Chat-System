A multi-threaded, real-time WebSocket messaging server built from scratch in C++17 using the Crow microframework and asynchronous network I/O primitives. The system features a decoupled front-end architecture, dynamic in-memory room routing, and explicit synchronization boundaries to guarantee multi-threaded memory safety under load.

## Architectural Overview

The server leverages a decoupled, highly concurrent design to maximize message throughput while maintaining a microscopic memory footprint:

* **Asynchronous I/O Layer:** Powered by `Boost.Asio`, leveraging non-blocking kernel event demultiplexing to monitor active connection descriptors without spawning a thread-per-client.
* **Decoupled Multi-Threaded Architecture:** Incoming event frames are immediately dispatched to an OS-level worker thread pool (`app.multithreaded().run()`).
* **Dual-Index Memory Ledger:** Room assignments and connection mapping are tracked entirely in heap memory using a bidirectional map layout ($O(1)$ `std::unordered_map` and `std::unordered_set`).
* **Non-Blocking Critical Sections:** State modifications are isolated via a `std::mutex`. To eliminate thread blockades, connection pointers are snapshot-copied into a cache-friendly `std::vector` under a brief RAII lock, releasing the mutex *before* performing outbound network I/O operations.

## Project Layout

├── main.cpp          # Core C++ Server Engine & Thread Pools
├── index.html        # Vanilla JS/HTML5 Client Testing Template
├── stress_test.yaml  # Artillery Load-Testing Profile Specification
└── build/            # Compiled Binaries & Makefiles
