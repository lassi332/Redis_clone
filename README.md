# Redis Clone (Gedis)

A lightweight, performant, in-memory key-value store clone of Redis built from scratch in C++. This project implements the Redis Serialization Protocol (RESP) and supports fundamental Redis commands, data structures, key expiration, and persistence.

## Features

- **RESP (Redis Serialization Protocol) Parser**: Fully compliant parsing of simple strings, errors, integers, bulk strings, and arrays.
- **TCP Server**: Multithreaded TCP server handling multiple concurrent client connections.
- **Key-Value Store**: Core operations (`GET`, `SET`, `DEL`, `EXISTS`) with thread-safe storage.
- **Key Expiration (TTL)**: Active and passive eviction of expired keys using `EX` and `PX` options.
- **Advanced Data Structures**: Basic implementations of Lists, Hashes, and Sets.
- **Persistence**: Append-Only File (AOF) logging for durability and crash recovery.

## Getting Started

### Prerequisites

- A C++17 compliant compiler (`clang++` or `g++`)
- `make` build tool

### Installation & Build

Clone this repository:
```bash
git clone https://github.com/lassi332/Redis_clone.git
cd Redis_clone
```

Build the project using `make`:
```bash
make
```

### Running the Server

Start the server on the default Redis port (`6379`):
```bash
./gedis-server
```

### Connecting with `redis-cli`

Once the server is running, you can connect to it using the standard Redis command-line interface:
```bash
redis-cli -p 6379
127.0.0.1:6379> PING
PONG
127.0.0.1:6379> SET mykey "hello"
OK
127.0.0.1:6379> GET mykey
"hello"
```

## Project Structure

```text
.
├── README.md           # Project overview and usage
├── ROADMAP.md          # Milestones, progress, and architectural context
├── Makefile            # Build script
├── src/
│   ├── main.cpp        # Server entry point
│   ├── server.hpp      # TCP Server declaration
│   ├── server.cpp      # TCP Server implementation
│   ├── resp.hpp        # RESP Parser declaration
│   ├── resp.cpp        # RESP Parser implementation
│   ├── store.hpp       # In-memory Store declaration
│   └── store.cpp       # In-memory Store implementation
```

## License

This project is open-source and available under the MIT License.
