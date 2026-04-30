#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>

namespace casio
{

// Small-device-style bounded cache:
// - inserts overwrite existing values
// - when over limit, evict ~1/8 oldest keys (gentle trim like python shared_cache)
template <typename V>
class BoundedCache
{
public:
    explicit BoundedCache(std::size_t limit = 1024) : limit_(limit) {}

    void set_limit(std::size_t n) { limit_ = n; trim(); }
    std::size_t limit() const { return limit_; }
    std::size_t size() const { return map_.size(); }

    void clear()
    {
        map_.clear();
        order_.clear();
    }

    bool get(std::string const &key, V &out) const
    {
        auto it = map_.find(key);
        if(it == map_.end()) return false;
        out = it->second;
        return true;
    }

    void put(std::string key, V value)
    {
        auto it = map_.find(key);
        if(it == map_.end()) {
            order_.push_back(key);
        }
        map_[std::move(key)] = std::move(value);
        trim();
    }

private:
    void trim()
    {
        if(limit_ == 0) {
            clear();
            return;
        }
        if(map_.size() <= limit_) return;
        std::size_t drop = map_.size() / 8;
        if(drop < 1) drop = 1;
        for(std::size_t i = 0; i < drop && !order_.empty(); i++) {
            std::string k = order_.front();
            order_.pop_front();
            map_.erase(k);
        }
        // If still above limit due to duplicates/overwrites, keep dropping.
        while(map_.size() > limit_ && !order_.empty()) {
            std::string k = order_.front();
            order_.pop_front();
            map_.erase(k);
        }
    }

    std::size_t limit_ = 1024;
    std::unordered_map<std::string, V> map_;
    std::deque<std::string> order_;
};

} // namespace casio

