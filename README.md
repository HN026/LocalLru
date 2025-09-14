# LocalLRU

A high-performance, thread-safe LRU (Least Recently Used) cache implementation in C++ using thread-local storage for lock-free operations.

## Overview

LocalLRU is a C++ implementation inspired by the Rust crate [a-agmon/LocalLRU](https://github.com/a-agmon/LocalLRU). It provides a fast, thread-safe LRU cache that eliminates contention by using thread-local storage, where each thread maintains its own independent cache instance.

### Key Features

- **Lock-free operations**: Each thread has its own cache store using thread-local storage
- **O(1) complexity**: Both get and put operations are O(1) average case
- **TTL support**: Optional time-to-live expiration for cached items
- **Type-safe**: Template-based design supporting any value type
- **Memory efficient**: Automatic eviction based on capacity limits
- **Modern C++**: Requires C++20 standard

## Design Philosophy

The core principle is to avoid lock contention in multi-threaded environments by giving each thread its own cache instance. This approach provides:

- **Zero synchronization overhead** for cache operations
- **Linear scalability** with thread count
- **Predictable performance** without lock contention
- **Simple API** similar to traditional caches

## Installation

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.10 or higher
- libcurl (for examples that fetch real-time data)

### Building

```bash
git clone <repository-url>
cd LocalLRU
mkdir build && cd build
cmake ..
make
```

## Usage

### Basic Usage

```cpp
#include "locallru/local_lru.hpp"

// Initialize a cache for string values with capacity=100, no TTL
auto cache = locallru::LocalCache<std::string>::initialize(100, 0);

// Add items
cache.add_item("key1", "value1");
cache.add_item("key2", "value2");

// Get items
auto value = cache.get_item("key1");
if (value) {
    std::cout << "Found: " << *value << std::endl;
}

// Remove items
bool removed = cache.remove_item("key1");
```

### Working with Custom Types

```cpp
struct UserData {
    std::string name;
    int age;
    std::vector<std::string> tags;
};

// Initialize cache for custom struct with TTL of 300 seconds
auto user_cache = locallru::LocalCache<UserData>::initialize(1000, 300);

// Store custom data
UserData user{"Alice", 30, {"developer", "cpp"}};
user_cache.add_item("user:123", user);

// Retrieve data
auto retrieved_user = user_cache.get_item("user:123");
if (retrieved_user) {
    std::cout << "User: " << retrieved_user->name << std::endl;
}
```

### TTL (Time-To-Live) Support

```cpp
// Cache with 60-second TTL
auto cache = locallru::LocalCache<std::string>::initialize(100, 60);

cache.add_item("temp_key", "temp_value");

// Item will expire after 60 seconds
std::this_thread::sleep_for(std::chrono::seconds(61));
auto expired = cache.get_item("temp_key"); // Returns std::nullopt
```

## API Reference

### LocalCache<T>

#### Static Methods

- `static LocalCache<T> initialize(std::size_t capacity, std::uint64_t ttl_seconds)`
  - Sets global defaults for future thread-local stores
  - `capacity`: Maximum number of items per thread-local cache
  - `ttl_seconds`: Time-to-live in seconds (0 = no expiration)
  - Returns a lightweight cache handle

#### Instance Methods

- `void add_item(const std::string& key, const T& value)`
  - Adds or updates an item in the cache
  
- `std::optional<T> get_item(const std::string& key)`
  - Retrieves an item if present and not expired
  
- `bool remove_item(const std::string& key)`
  - Removes an item, returns true if item was present
  
- `std::size_t size() const`
  - Returns current number of items in thread-local cache
  
- `std::size_t capacity() const`
  - Returns maximum capacity of thread-local cache
  
- `std::uint64_t ttl_seconds() const`
  - Returns TTL setting of thread-local cache
  
- `void clear()`
  - Removes all items from thread-local cache

## Performance Comparison

The project includes a trading demo that compares lock-free vs. lock-based cache performance:

```bash
# Run the trading benchmark
./build/trading_demo
```

This benchmark processes stock price data and compares:
- **Lock-free cache** (LocalLRU): Uses thread-local storage
- **Lock-based cache**: Traditional mutex-protected cache

Results are written to `results/trading_benchmark.log`.

## Examples

### Trading Demo

The `examples/trading_demo.cpp` demonstrates real-world usage by:
1. Loading stock price data from CSV files
2. Benchmarking both lock-free and lock-based caches
3. Processing multiple symbols concurrently
4. Measuring performance differences

### Running Examples

```bash
# Ensure data directory exists and contains CSV files
mkdir -p data

# Build and run
cd build
make
./trading_demo
```

## Project Structure

```
LocalLRU/
├── include/locallru/
│   └── local_lru.hpp          # Main LRU cache implementation
├── src/
│   └── lock_cache.hpp         # Lock-based cache for comparison
├── examples/
│   └── trading_demo.cpp       # Performance benchmark example
├── scripts/
│   ├── fetch_data.py          # Data fetching utilities
│   └── plot_results.py        # Results visualization
├── data/                      # CSV data files (gitignored)
├── results/                   # Benchmark results (gitignored)
└── CMakeLists.txt            # Build configuration
```

## Thread Safety

- Each thread gets its own independent cache instance
- No synchronization required for cache operations
- Global configuration (via `initialize()`) uses atomic operations
- Thread-local stores are created lazily on first access

## Memory Management

- Automatic eviction when capacity is reached (LRU policy)
- TTL-based expiration during read/write operations
- RAII-based resource management
- No memory leaks with proper usage

## Comparison with Other Approaches

| Approach | Pros | Cons |
|----------|------|------|
| **Thread-Local (LocalLRU)** | Lock-free, linear scaling, predictable performance | Memory per thread, no cross-thread sharing |
| **Global + Mutex** | Memory efficient, cross-thread sharing | Lock contention, poor scaling |
| **Lock-free Global** | No locks, sharing possible | Complex implementation, ABA problems |

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes with tests
4. Submit a pull request

## Acknowledgments

- Inspired by [a-agmon/LocalLRU](https://github.com/a-agmon/LocalLRU) Rust crate
- Uses modern C++20 features for optimal performance
- Designed for high-frequency trading and real-time applications