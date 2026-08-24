#ifndef RISC_V_HASHMAP_H
#define RISC_V_HASHMAP_H
#include "vector.h"
#include "../mm/mem.h"

template<typename K>
struct HashTrait {
    static uint64_t hash(const K& key, uint64_t capacity) {
        auto v = (uint64_t)key;
        constexpr int shift = (sizeof(K) >= 8) ? 3 :
                              (sizeof(K) >= 4) ? 2 :
                              (sizeof(K) >= 2) ? 1 : 0;
        v >>= shift;
        v *= 11400714819323198485ULL;
        return v % capacity;
    }
    static bool eq(const K& a, const K& b) { return a == b; }
};

template<>
struct HashTrait<uint32_t> {
    static uint64_t hash(uint32_t key, uint64_t capacity) {
        return ((uint64_t)key * 2654435761ULL) % capacity;
    }
    static bool eq(uint32_t a, uint32_t b) { return a == b; }
};

template<>
struct HashTrait<uint64_t> {
    static uint64_t hash(uint64_t key, uint64_t capacity) {
        key ^= key >> 30; key *= 0xbf58476d1ce4e5b9ULL;
        key ^= key >> 27; key *= 0x94d049bb133111ebULL;
        key ^= key >> 31;
        return key % capacity;
    }
    static bool eq(uint64_t a, uint64_t b) { return a == b; }
};

template<>
struct HashTrait<const char*> {
    static uint64_t hash(const char* key, uint64_t capacity) {
        uint64_t h = 5381;
        while (*key) h = ((h << 5) + h) ^ (unsigned char)*key++;
        return h % capacity;
    }
    static bool eq(const char* a, const char* b) { return strcmp(a, b) == 0; }
};

template<typename K, typename V, typename Trait = HashTrait<K>>
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

    struct Pair {
        K_REFERENCE_TYPE key;
        V_REFERENCE_TYPE value;
    };

    class Iterator {
    public:
        Iterator(const HashMap* map, uint64_t index, HashMapEntry* entry)
            : m_map(map), m_index(index), m_entry(entry) {}

        Pair operator*() const {
            return Pair{.key = m_entry->key, .value = m_entry->value};
        }

        Iterator& operator++() {
            if (!m_entry) return *this;

            m_entry = m_entry->next;

            if (!m_entry) {
                m_index++;
                while (m_index < m_map->m_capacity) {
                    if (m_map->m_entries.at(m_index)) {
                        m_entry = m_map->m_entries.at(m_index);
                        break;
                    }
                    m_index++;
                }
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return m_entry == other.m_entry && m_index == other.m_index;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        const HashMap* m_map;
        uint64_t m_index;
        HashMapEntry* m_entry;
    };

    using ITERATOR_TYPE = Iterator;
    using CONST_ITERATOR_TYPE = const Iterator;

public:
    explicit HashMap(uint64_t capacity = DEFAULT_CAPACITY) :
        m_entries(capacity), m_capacity(capacity), m_count(0)
    {
        m_entries.resize(capacity);
        m_entries.fill(0);
    }

    ~HashMap() {
        for (uint64_t i = 0; i < m_capacity; i++) {
            HashMapEntry* entry = m_entries[i];
            while (entry) {
                HashMapEntry* next = entry->next;
                delete entry;
                entry = next;
            }
        }
    }

    V_CONST_REFERENCE_TYPE at(K_CONST_REFERENCE_TYPE key) const {
        uint64_t index = hash(key);
        HashMapEntry* entry = m_entries.at(index);
        while (entry != nullptr) {
            if (Trait::eq(entry->key, key))
                return entry->value;
            entry = entry->next;
        }
        Console::panic("HashMap::at(): key not found");
    }

    V_REFERENCE_TYPE operator[](K_CONST_REFERENCE_TYPE key) {
        uint64_t index = hash(key);
        HashMapEntry* entry = m_entries[index];
        while (entry != nullptr) {
            if (Trait::eq(entry->key, key))
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
            if (Trait::eq(it->key, key))
                Console::panic("HashMap::insert(): key already inside");
            it = it->next;
        }

        auto* entry = new HashMapEntry{.key = key, .value = value, .next = nullptr};
        entry->next = m_entries[index];
        m_entries[index] = entry;
        m_count++;
    }

    void erase(K_CONST_REFERENCE_TYPE key) {
        uint64_t index = hash(key);

        HashMapEntry* entry = m_entries[index];
        HashMapEntry* prev = nullptr;
        while (entry != nullptr) {
            if (Trait::eq(entry->key, key))
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
        HashMapEntry* entry = m_entries.at(index);
        while (entry != nullptr) {
            if (Trait::eq(entry->key, key))
                return true;
            entry = entry->next;
        }
        return false;
    }

    ITERATOR_TYPE begin() {
        for (uint64_t i = 0; i < m_capacity; i++) {
            if (m_entries[i]) {
                return Iterator(this, i, m_entries[i]);
            }
        }
        return end();
    }

    ITERATOR_TYPE end() {
        return Iterator(this, m_capacity, nullptr);
    }

private:
    uint64_t hash(K_CONST_REFERENCE_TYPE key) const {
        return hashKey(key, m_capacity);
    }

    static uint64_t hashKey(K_CONST_REFERENCE_TYPE key, uint64_t capacity) {
        return Trait::hash(key, capacity);
    }

    void rehash() {
        uint64_t newCapacity = m_capacity * 2;
        
        auto** newTable = (HashMapEntry**)MemoryAllocator::kmalloc(newCapacity * sizeof(HashMapEntry*));
        memset(newTable, 0, newCapacity * sizeof(HashMapEntry*));

        for (uint64_t i = 0; i < m_capacity; i++) {
            HashMapEntry* entry = m_entries[i];
            while (entry != nullptr) {
                HashMapEntry* next = entry->next;

                uint64_t newIndex = hashKey(entry->key, newCapacity);
                entry->next = newTable[newIndex];
                newTable[newIndex] = entry;

                entry = next;
            }
        }

        m_entries.reset(newTable, newCapacity);
        m_capacity = newCapacity;
    }

    Vector<HashMapEntry*> m_entries;
    uint64_t m_capacity;
    uint64_t m_count;
};

#endif