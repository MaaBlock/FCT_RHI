#pragma once

#ifndef FCT_CONTAINERS_H
#define FCT_CONTAINERS_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <queue>
#include <memory>
#include <functional>
#include <algorithm>

namespace FCT {

    template<typename T, typename Alloc = std::allocator<T>>
    using Vector = std::vector<T, Alloc>;

    template<typename Key, 
             typename Value, 
             typename Hash = std::hash<Key>, 
             typename KeyEqual = std::equal_to<Key>, 
             typename Alloc = std::allocator<std::pair<const Key, Value>>>
    using UnorderedMap = std::unordered_map<Key, Value, Hash, KeyEqual, Alloc>;

    template<typename Key, 
             typename Hash = std::hash<Key>, 
             typename KeyEqual = std::equal_to<Key>, 
             typename Alloc = std::allocator<Key>>
    using UnorderedSet = std::unordered_set<Key, Hash, KeyEqual, Alloc>;

    template<typename Key, 
             typename Value, 
             typename Compare = std::less<Key>, 
             typename Alloc = std::allocator<std::pair<const Key, Value>>>
    using Map = std::map<Key, Value, Compare, Alloc>;

    template<typename Key, 
             typename Compare = std::less<Key>, 
             typename Alloc = std::allocator<Key>>
    using Set = std::set<Key, Compare, Alloc>;

    template<typename T, typename Alloc = std::allocator<T>>
    using Deque = std::deque<T, Alloc>;

    template<typename T, typename Container = std::deque<T>>
    using Queue = std::queue<T, Container>;

    template<typename Container, typename T>
    [[nodiscard]] inline bool Contains(const Container& container, const T& value) {
        if constexpr (requires { container.contains(value); }) {
            return container.contains(value);
        } else {
            return std::find(container.begin(), container.end(), value) != container.end();
        }
    }

    template<typename Container, typename T>
    inline bool Erase(Container& container, const T& value) {
        return std::erase(container, value) > 0;
    }

    template<typename MapType, typename KeyType>
    [[nodiscard]] inline auto* FindOrNull(MapType& container, const KeyType& key) {
        auto it = container.find(key);
        if (it == container.end()) {
            return decltype(&it->second)(nullptr);
        }
        return &it->second;
    }

    template<typename MapType, typename KeyType>
    [[nodiscard]] inline const auto* FindOrNull(const MapType& container, const KeyType& key) {
        auto it = container.find(key);
        if (it == container.end()) {
            return decltype(&it->second)(nullptr);
        }
        return &it->second;
    }

} // namespace FCT

#endif // FCT_CONTAINERS_H
