#pragma once

#include <set>
#include <unordered_map>

template <typename K, typename V>
class MinValueMap
{
public:
    MinValueMap(size_t n) : n(n) {}

    bool find(const K &key){
        return map.count(key);
    }

    K insert(const K &key, const V &value)
    {
        auto it = map.find(key);
        if (it != map.end())
        {
            // Key already exists, update its value
            auto setIt = set.find({it->second, it->first});
            set.erase(setIt);
            set.insert({value, key});
            it->second = value;
        }
        else
        {
            // New key
            map[key] = value;
            if (set.size() < n)
            {
                set.insert({value, key});
            }
            else if (value < set.rbegin()->first)
            {
                auto last = *set.rbegin();
                set.erase(last);
                set.insert({value, key});
                map.erase(last.second);
                return last.second;
            }
        }
        return -1;
    }

    bool full() const
    {
        return set.size() == n;
    }

    bool empty() const
    {
        return set.empty();
    }

    V get_max_value() const
    {
        return set.rbegin()->first;
    }

    size_t n;
    std::set<std::pair<V, K>> set;
    std::unordered_map<K, V> map;
private:
};