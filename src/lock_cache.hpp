#pragma once
#include <chrono>
#include <mutex>
#include <list>
#include <unordered_map>
#include <optional>

namespace lockedlru {
    
    using Clock = std::chrono::steady_clock;
    
    template<typename K, typename V>
    class LockCache {
        public:
            using key_type = K;
            using value_type = V;
        
            explicit LockCache(std::size_t capacity) : capacity_(capacity) {}
            
            void put(const key_type& key, value_type value){
                std::lock_guard<std::mutex> lock(mutex_);
                if(capacity_ == 0) return;
                auto it = map_.find(key);
                if(it != map_.end()) {
                    it->second.value = std::move(value);
                    lru_.splice(lru_.begin(), lru_, it->second.lru_it);
                    it->second.lru_it = lru_.begin();
                    return;
                }
                if(map_.size() >= capacity_) {
                    auto last = std::prev(lru_.end());
                    map_.erase(*last);
                    lru_.pop_back();
                }
                lru_.push_front(key);
                Node node{std::move(value), lru_.begin()};
                map_.emplace(key, std::move(node));
            }
            
            std::optional<value_type> get(const key_type& key){
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = map_.find(key);
                if(it == map_.end()) return std::nullopt;
                lru_.splice(lru_.begin(), lru_, it->second.lru_it);
                it->second.lru_it = lru_.begin();
                return it->second.value;
            }
            
        private:
            struct Node {
                value_type value;
                typename std::list<key_type>::iterator lru_it;
            };
            
            using Map = std::unordered_map<key_type, Node>;
            
            std::size_t capacity_;
            Map map_;
            std::list<key_type> lru_;
            std::mutex mutex_;
    };
}