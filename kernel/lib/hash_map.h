#ifndef RISC_V_HASHMAP_H
#define RISC_V_HASHMAP_H
#include "vector.h"

template<typename K, typename V>
class HashMap {
    static constexpr uint64_t DEFAULT_CAPACITY = 64;
    static constexpr uint64_t LOAD_FACTOR_NUM  = 3;
    static constexpr uint64_t LOAD_FACTOR_DEN  = 4;

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
            s_entryCache->free(ptr);
        }
    };

    inline static KMemCache<HashMapEntry>* s_entryCache = nullptr;

public:
    explicit HashMap(uint64_t capacity = DEFAULT_CAPACITY) :
        m_entries(capacity), m_capacity(capacity), m_count(0)
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
        HashMapEntry* entry = m_entries[index];
        while (entry != nullptr) {
            if (entry->key == key)
                return entry->value;
            entry = entry->next;
        }
        Console::panic("HashMap::operator[](): key not found");
    }

    void insert(K_CONST_REFERENCE_TYPE key, V_CONST_REFERENCE_TYPE value) {
        if ((m_count + 1) * LOAD_FACTOR_DEN > m_capacity * LOAD_FACTOR_NUM)
            rehash();

        uint64_t index = hash(key);
        HashMapEntry* it = m_entries[index];
        while (it != nullptr) {
            if (it->key == key)
                Console::panic("HashMap::insert(): key already inside");
            it = it->next;
        }

        HashMapEntry* entry = new HashMapEntry{.key = key, .value = value, .next = nullptr};
        entry->next = m_entries[index];
        m_entries[index] = entry;
        m_count++;
    }

    void erase(K_CONST_REFERENCE_TYPE key) {
        uint64_t index = hash(key);

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

        if (prev)
            prev->next = entry->next;
        else
            m_entries[index] = entry->next;

        entry->next = nullptr;
        delete entry;
        m_count--;
    }

    void clear() {
        for (uint64_t i = 0; i < m_capacity; i++) {
            delete m_entries[i];
            m_entries[i] = nullptr;
        }
        m_count = 0;
    }

    bool empty() const {
        return m_count == 0;
    }

    uint64_t size() const {
        return m_count;
    }

    bool contains(K_CONST_REFERENCE_TYPE key) const {
        uint64_t index = hash(key);
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
        return hashKey(key, m_capacity);
    }

    static uint64_t hashKey(K_CONST_REFERENCE_TYPE key, uint64_t capacity) {
        uint64_t v = (uint64_t)key;
        constexpr int shift = (sizeof(K) >= 8) ? 3 :
                              (sizeof(K) >= 4) ? 2 :
                              (sizeof(K) >= 2) ? 1 : 0;
        v >>= shift;
        v *= 11400714819323198485ULL;
        return v % capacity;
    }

    void rehash() {
        uint64_t newCapacity = m_capacity * 2;
        Vector<HashMapEntry*> newEntries(newCapacity);
        newEntries.resize(newCapacity);

        for (uint64_t i = 0; i < m_capacity; i++) {
            HashMapEntry* entry = m_entries[i];
            while (entry != nullptr) {
                HashMapEntry* next = entry->next;

                uint64_t newIndex = hashKey(entry->key, newCapacity);
                entry->next = newEntries[newIndex];
                newEntries[newIndex] = entry;

                entry = next;
            }
            m_entries[i] = nullptr;
        }

        m_entries = static_cast<Vector<HashMapEntry*>&&>(newEntries);
        m_capacity = newCapacity;
    }

    Vector<HashMapEntry*> m_entries;
    uint64_t m_capacity;
    uint64_t m_count;
};

#endif