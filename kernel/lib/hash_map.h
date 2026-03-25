#ifndef RISC_V_HASHMAP_H
#define RISC_V_HASHMAP_H
#include "vector.h"

template<typename K, typename V>
class HashMap {
    static constexpr uint64_t DEFAULT_CAPACITY = 64;
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

    struct HashMapEntry {
        K_VALUE_TYPE key;
        V_VALUE_TYPE value;
        HashMapEntry* next;

        ~HashMapEntry() {
            delete next;
        }

        void* operator new(size_t size) {
            if (!s_entryCache) {
                s_entryCache = new KMemCache<HashMapEntry>();
            }
            return s_entryCache->alloc();
        }

        void operator delete(void* ptr) {
            return s_entryCache->free(ptr);
        }
    };

    inline static KMemCache<HashMapEntry>* s_entryCache = nullptr;

public:
    explicit HashMap(uint64_t capacity = DEFAULT_CAPACITY) :
        m_entries(Vector<HashMapEntry*>(capacity)), m_capacity(capacity), m_count(0)
    {
        memset(m_entries, 0, sizeof(HashMapEntry*) * m_capacity);
    }

    ~HashMap() {
        for (auto& e : m_entries) {
            delete e;
        }
    }

    V_CONST_REFERENCE_TYPE at(K_CONST_REFERENCE_TYPE key) const {
        uint64_t index = hash(key);
        if (m_entries[index] == nullptr)
            Console::panic("HashMap::at(): key not found");

        HashMapEntry* entry = m_entries[index];
        while (entry != nullptr) {
            if (entry->key == key)
                return entry->value;
            entry = entry->next;
        }
        Console::panic("HashMap::at(): key not found");
    }

    V_REFERENCE_TYPE operator[](K_CONST_REFERENCE_TYPE key) {
        uint64_t index = hash(key);
        if (m_entries[index] == nullptr)
            Console::panic("HashMap::at(): key not found");

        HashMapEntry* entry = m_entries[index];
        while (entry != nullptr) {
            if (entry->key == key)
                return entry->value;
            entry = entry->next;
        }
        Console::panic("HashMap::at(): key not found");
    }

    void insert(K_CONST_REFERENCE_TYPE key, V_CONST_REFERENCE_TYPE value) {
        uint64_t index = hash(key);
        HashMapEntry* entry = new HashMapEntry{.key = key, .value = value, .next = nullptr};
        m_count++;
        if (!m_entries[index]) {
            m_entries[index] = entry;
            return;
        }
        HashMapEntry* it = entry;
        while (it != nullptr) {
            if (it->key == key)
                Console::panic("HashMap::insert(): key already inside");
        }
        entry->next = m_entries[index];
        m_entries[index] = entry;
    }

    void erase(K_CONST_REFERENCE_TYPE key) {
        uint64_t index = hash(key);
        if (m_entries[index] == nullptr)
            Console::panic("HashMap::at(): key not found");

        HashMapEntry* entry = m_entries[index];
        HashMapEntry* prev = nullptr;
        while (entry != nullptr) {
            if (entry->key == key)
                break;
            prev = entry;
            entry = entry->next;
        }
        if (!entry)
            Console::panic("HashMap::erase(): key not found");

        m_count--;
        if (prev) {
            prev->next = entry->next;
        }
        else {
            m_entries[index] = entry->next;
        }
        delete entry;
    }

    void clear() {
        for (auto& e : m_entries) {
            delete e;
            e = nullptr;
        }
        m_count = 0;
    }

    bool empty() const {
        return m_count == 0;
    }

    bool contains(K_CONST_REFERENCE_TYPE key) const {
        uint64_t index = hash(key);
        if (m_entries[index] == nullptr) return false;

        HashMapEntry* entry = m_entries[index];
        while (entry != nullptr) {
            if (entry->key == key)
                return true;
            entry = entry->next;
        }
        return false;
    }

private:
    uint64_t hash(K_CONST_REFERENCE_TYPE key) const {
        return (uint64_t)key % m_capacity;
    }

    Vector<HashMapEntry*> m_entries;
    uint64_t m_capacity;
    uint64_t m_count;
};

#endif