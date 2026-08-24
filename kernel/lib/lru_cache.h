#ifndef RISC_V_LRU_CACHE_H
#define RISC_V_LRU_CACHE_H

#include "list.h"
#include "hash_map.h"
#include "../io/console/console.h"

template <typename K, typename V>
class LRUCache {
public:
    using CleanupCallback = void (*)(const K& key, V& value);

private:
    static constexpr uint64_t DEFAULT_CAPACITY = 1024;

    using K_REFERENCE_TYPE = K&;
    using K_CONST_REFERENCE_TYPE = const K&;
    using K_POINTER_TYPE = K*;
    using K_CONST_POINTER_TYPE = const K*;
    using K_VALUE_TYPE = K;

    using V_REFERENCE_TYPE = V&;
    using V_CONST_REFERENCE_TYPE = const V&;
    using V_POINTER_TYPE = V*;
    using V_CONST_POINTER_TYPE = const V*;
    using V_VALUE_TYPE = V;

    struct CacheEntry {
        K key;
        V value;
    };

    using ListIterator = typename List<CacheEntry>::Iterator;

public:
    explicit LRUCache(uint64_t capacity = DEFAULT_CAPACITY, 
                     CleanupCallback onEvict = nullptr, 
                     CleanupCallback onFlush = nullptr) 
        : m_capacity(capacity), 
          m_map(capacity), 
          m_onEvict(onEvict), 
          m_onFlush(onFlush ? onFlush : onEvict) {}

    explicit LRUCache(CleanupCallback onEvict = nullptr, 
                     CleanupCallback onFlush = nullptr) 
        : m_capacity(DEFAULT_CAPACITY), 
          m_map(DEFAULT_CAPACITY), 
          m_onEvict(onEvict), 
          m_onFlush(onFlush ? onFlush : onEvict) {}

    ~LRUCache() = default;

    bool contains(K_CONST_REFERENCE_TYPE key) {
        if (!m_map.contains(key)) {
            return false;
        }

        ListIterator it = m_map[key];
        m_list.moveFront(it);
        return true;
    }

    V_REFERENCE_TYPE at(K_CONST_REFERENCE_TYPE key) {
        ListIterator it = m_map[key];
        return it->value;
    }

    bool get(K_CONST_REFERENCE_TYPE key, V_REFERENCE_TYPE outVal) {
        if (!m_map.contains(key)) {
            return false;
        }
        ListIterator it = m_map[key];
        m_list.moveFront(it);
        outVal = it->value;
        return true;
    }

    void insert(K_CONST_REFERENCE_TYPE key, V_CONST_REFERENCE_TYPE value) {
        if (m_map.contains(key)) {
            ListIterator it = m_map[key];
            m_list.moveFront(it);
            m_map[key] = m_list.begin();
            return;
        }

        if (m_map.size() >= m_capacity) {
            CacheEntry entry = m_list.back();
            if (m_onEvict) {
                m_onEvict(entry.key, entry.value);
            }
            m_map.erase(entry.key);
            m_list.removeLast();
        }

        CacheEntry ce = CacheEntry{key, value};
        m_list.addFirst(ce);
        m_map.insert(key, m_list.begin());
    }

    void erase(K_CONST_REFERENCE_TYPE key) {
        if (!m_map.contains(key)) {
            return;
        }
        ListIterator it = m_map[key];
        m_list.remove(it);
        m_map.erase(key);
    }

    template <typename Predicate>
    void eraseIf(Predicate predicate) {
        auto it = m_list.begin();
        while (it != m_list.end()) {
            auto nextIt = it;
            ++nextIt;

            if (predicate(it->key, it->value)) {
                if (m_onEvict) {
                    m_onEvict(it->key, it->value);
                }
                m_map.erase(it->key);
                m_list.remove(it);
            }
            it = nextIt;
        }
    }

    void flush() {
        if (m_onFlush || m_onEvict) {
            CleanupCallback cb = m_onFlush ? m_onFlush : m_onEvict;
            for (auto& entry : m_list) {
                cb(entry.key, entry.value);
            }
        }
        m_list.clear();
        m_map.clear();
    }

private:
    uint64_t m_capacity;
    List<CacheEntry> m_list;
    HashMap<K, ListIterator> m_map;
    
    CleanupCallback m_onEvict{nullptr};
    CleanupCallback m_onFlush{nullptr};
};

#endif