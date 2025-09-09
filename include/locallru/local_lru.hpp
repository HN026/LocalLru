#pragma once
#include <cstdint>
#include <unordered_map>
#include <list>
#include <chrono>
#include <optional>
#include <utility>

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
    
    // A single-thread store implementing LRU with TTL.
    // Not thread-safe across threads (by design) but safe for single-thread use.
    // Managed behind thread_local in LocalCache<T>.
    
    template<typename K, typename V>
    class LruStore{
      public:
        using key_type = K;
        using value_type = V;
        using time_point = Clock::time_point;
        
        explicit LruStore(std::size_t capacity, std::uint64_t ttl_seconds) 
            : capacity_(capacity), ttl_seconds_(ttl_seconds) {}
        
        std::size_t capacity() const noexcept { return capacity_;}
        std::uint64_t ttl_seconds() const noexcept { return ttl_seconds_ ; }
        std::size_t size() const noexcept { return map_.size(); }
        
        void clear(){
            map_.clear();
            lru_.clear();
        }
        
        bool contains_expired(const key_type &key, time_point now) const {
            auto it = map_.find(key);
            if(it==map_.end()) return false;
            return is_expired(it->second, now);
        }
        
        std::optional<value_type> get(const key_type &key){
            const auto now = Clock::now();
            auto it = map_.find(key);
            if(it == map_.end()) return std::nullopt;
            if (is_expired(it->second, now)) {
                erase_it(it);
                return std::nullopt;
            }
            touch(it);
            return it->second.value;
        }
        
        void put(const key_type& key, value_type value){
            const auto now = Clock::now();
            if(capacity_ == 0) return; // No capacity to store
            
            auto it = map_.find(key);
            if(it != map_.end()){
                it->second.value = value;
                it->second.expiry = expiry_from(now);
                touch(it);
                return;
            }
            
            // Ensure space
            while(map_.size() >= capacity_) {
                evict_one();
            }
            
            lru_.push_front(key);
            Node node{std::move(value), expiry_from(now), lru_.begin()};
            map_.emplace(key, std::move(node));
        }
        
        bool erase(const key_type &key){
            auto it = map_.find(key);
            if(it == map_.end()) return false;
            erase_it(it);
            return true;
        }
        
        
      private:
        struct Node {
            value_type value;
            time_point expiry;
            typename std::list<key_type>::iterator lru_it;
        };
        
        using Map = std::unordered_map<key_type, Node>;
        
        bool is_expired(const Node& n, time_point now) const {
            if(ttl_seconds_ == 0) return false;
            return now > n.expiry;
        }
        
        time_point expiry_from(time_point now) const {
            if (ttl_seconds_ == 0) return time_point::max();
            return now + Seconds(static_cast<long long>(ttl_seconds_));
        }
        
        void touch(typename Map::iterator it){
            // Move key to front (Most recently used)
            lru_.splice(lru_.begin(), lru_, it->second.lru_it);
            it->second.lru_it = lru_.begin();
        }
        
        void evict_one() {
            if(lru_.empty()) return;
            auto last_it = std::prev(lru_.end()); // lru_.end() is a sentinel iterator (points past the last element)
            auto key = *last_it;
            auto it = map_.find(key);
            if (it != map_.end()) {
                lru_.erase(it->second.lru_it);
                map_.erase(it);
            } else {
                // Should not happen; Keep structure consistent
                lru_.erase(last_it);
            }
        }
        
        void erase_it(typename Map::iterator it) {
            lru_.erase(it->second.lru_it);
            map_.erase(it);
        }
      
        std::size_t capacity_ = 0;
        std::uint64_t ttl_seconds_ = 0; // 0 => No expiry
        std::list<key_type> lru_; // front = most-recent, back = least-recent
        Map map_;
    };
