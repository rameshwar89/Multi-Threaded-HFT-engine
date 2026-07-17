# Multi-Threaded High-Frequency Trading (HFT) Engine

## Live Demo

https://github.com/user-attachments/assets/6564a3d6-4316-4530-81ed-34b7dda38a6a

A blazingly fast, multi-threaded high-frequency trading (HFT) engine built for extreme low-latency order matching. This project features a custom C++ limit order book, a Node.js WebSocket bridge for real-time frontend updates, and asynchronous MySQL persistence via Redis Pub/Sub to ensure the core trading loop never blocks.

## Architecture Overview

This project is designed to eliminate I/O bottlenecks by isolating the critical trading path from database persistence and client communication.

1. **Frontend (Browser)**: A sleek dashboard for submitting orders and viewing real-time latency metrics.
2. **WebSocket Bridge (`bridge.js`)**: A Node.js server that translates WebSocket messages into padded 64-byte TCP packets to feed into the C++ engine.
3. **C++ Matching Engine (`hftengine`)**: The core component. Uses a pre-allocated memory pool and a lock-free ring buffer (SPSC) to process orders with sub-microsecond latency. It handles matching and broadcasts execution times back to the TCP clients.
4. **Redis Pub/Sub**: When trades match, the C++ engine asynchronously publishes the trade data to a Redis channel.
5. **Node.js Worker (`worker.js`)**: Subscribes to the Redis channel, batches trades in-memory, and flushes them to MySQL in large bulk inserts to minimize database locking.
6. **MySQL Database**: Persists all matched trades.

## Core Architecture Highlights

### The Hot Path vs. Cold Path
To achieve sub-microsecond latency, the system strictly isolates the time-critical matching engine from the slow database persistence layer:
- **The Hot Path (C++ Engine):** Orders are ingested via TCP, matched in the limit order book, and executions are broadcast back to the client immediately. The engine *never* blocks to wait for a database write.
- **The Cold Path (Redis + Node.js Worker):** When a trade occurs, the C++ engine fires an asynchronous, fire-and-forget `PUBLISH` event to Redis. A background Node.js worker subscribes to this stream, batches the trades in memory, and performs bulk `INSERT` operations into MySQL.

### Lock-Free Ring Buffer (SPSC)
Incoming TCP packets are received by a dedicated networking thread and pushed into a Single-Producer Single-Consumer (SPSC) lock-free ring buffer. The core matching thread continuously consumes from this buffer, entirely decoupling network I/O delays from the order matching logic.

### Pre-Allocated Memory Pool
Dynamic memory allocation (`malloc`/`new`) is disastrous for HFT latency. The engine uses a custom memory pool that pre-allocates 1,000,000 `order` objects at startup. The matching loop simply pops available pointers from a free-list, reducing heap allocation overhead to exactly zero during active trading.

## Performance

- **Pre-allocated Memory Pool**: Eliminates `malloc`/`new` overhead during the trading loop.
- **Lock-Free Ring Buffer**: Decouples network I/O (TCP socket reads) from the core order matching thread.
- **Batched Database Writes**: Prevents the database from becoming a bottleneck during high-volume spikes (e.g., 10,000 orders/second).

## Prerequisites

- **C++17** compatible compiler (`g++`)
- **Make**
- **Node.js** (v20+)
- **Redis Server** (running on `localhost:6379`)
- **MySQL Server** (running on `localhost:3306`)
- `libhiredis-dev` (for C++ Redis integration)

## Installation & Setup

### 1. Database Setup
Create the MySQL database and user:
```sql
CREATE DATABASE hft;
CREATE TABLE hft.trades (
    id INT AUTO_INCREMENT PRIMARY KEY,
    side CHAR(1),
    price INT,
    quantity INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE USER 'hft_user'@'localhost' IDENTIFIED BY 'password';
GRANT ALL PRIVILEGES ON hft.* TO 'hft_user'@'localhost';
FLUSH PRIVILEGES;
```

### 2. Compile the C++ Engine
```bash
cd server
make clean
make
```

### 3. Install Node.js Dependencies
```bash
cd client
npm install ws
cd ../worker
npm install redis mysql2
```

## Running the Engine Locally

You will need to open 4 separate terminals to run all the components of this microservice architecture.

**Terminal 1: The C++ HFT Engine**
```bash
./server/hftengine
```

**Terminal 2: The MySQL Persistence Worker**
```bash
node worker/worker.js
```

**Terminal 3: The WebSocket Bridge**
```bash
node client/bridge.js
```

**Terminal 4: The Frontend Dashboard**
```bash
cd client
npx serve
```
*(Open the provided `localhost` link in your browser to view the trading dashboard).*

## Load Testing

To simulate high-frequency trading volumes, you can run the provided Python load tester. It spins up 10 threads and bombards the engine with 10,000 random orders in under a second.

```bash
python3 client/load_tester.py
```
You will immediately see the latency metrics in your frontend dashboard, and `worker.js` will log bulk flushes to MySQL.
