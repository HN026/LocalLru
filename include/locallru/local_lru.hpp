#pragma once
#include <unordered_map>
#include <list>
#include <chrono>
#include <optional>
#include <string>
#include <atomic>
#include <memory>
#include <utility>
#include <type_traits>

// -----------------------------------------------------------------------------
// local_lru.hpp
// A simple, fast, thread-safe (by design) and lock-free LRU cache using
// thread-local storage, loosely mirroring the API/semantics of
// a-agmon/LocalLRU (Rust) but implemented in modern C++.
// -----------------------------------------------------------------------------
// Key design points (to match the Rust crate's behavior):
// - Each thread has its own cache store (TLS). No locks are used.
// - LocalCache<T>::initialize(capacity, ttl_seconds) only sets the global
// default parameters used by *future* thread-local stores. It does not
// construct the cache. A thread-local store is constructed lazily on first
// cache access (get/add) and captures the *current* global params.
// - Subsequent calls to initialize(...) DO NOT affect threads that have
// already materialized their store.
// - TTL (time-to-live) is enforced on read and write; 0 means "no expiry".
// - O(1) get/add using unordered_map + intrusive LRU order via std::list.
// -----------------------------------------------------------------------------
// Usage example (mirrors the README from the Rust crate):
//
// // Define a cache for std::string values
// auto cache = LocalCache<std::string>::initialize(2, 60);
// // Change global defaults for threads that haven't touched the cache yet
// cache = LocalCache<std::string>::initialize(2, 0);
// // Add and get
// cache.add_item("key1", std::string("value1"));
// auto v = cache.get_item("key1");
// if (v && *v == "value1") { /* ok */ }
//
// // Store a struct by creating a typed cache
// struct TestStruct { std::string field1; int field2; };
// auto struct_cache = LocalCache<TestStruct>::initialize(128, 120);
// TestStruct ts{ "Hello", 42 };
// struct_cache.add_item("test_key", ts);
// auto ret = struct_cache.get_item("test_key");
// // ret is std::optional<TestStruct>
//
// If you want a single cache that stores raw bytes, use std::string or
// std::vector<unsigned char> as the value type.
// -----------------------------------------------------------------------------

namespace locallru {
    // For timing
    using Clock = std::chrono::steady_clock;
    using Seconds = std::chrono::seconds;
}