#ifndef RISC_V_VECTOR_H
#define RISC_V_VECTOR_H

#include "../types.h"
#include "../io/console/console.h"
#include "../mm/kalloc/kalloc.h"
#include "../mm/mem.h"
#include "../proc/sync/lock.h"

template <typename T>
class Vector {

    static constexpr uint64_t DEFAULT_CAPACITY = 64;
    using REFERENCE_TYPE = T&;
    using CONST_REFERENCE_TYPE = const T&;
    using POINTER_TYPE = T*;
    using CONST_POINTER_TYPE = const T*;
    using VALUE_TYPE = T;

    class Iterator {
    public:
        explicit Iterator(POINTER_TYPE it) : m_it(it) {}

        REFERENCE_TYPE operator*() const {
            return *m_it;
        }

        POINTER_TYPE operator->() const {
            return m_it;
        }

        Iterator& operator++() {
            ++m_it;
            return *this;
        }

        Iterator& operator--() {
            --m_it;
            return *this;
        }

        Iterator operator++(int) {
            Iterator ret = *this;
            ++m_it;
            return ret;
        }

        Iterator operator--(int) {
            Iterator ret = *this;
            --m_it;
            return ret;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.m_it == b.m_it;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.m_it != b.m_it;
        }

    private:
        POINTER_TYPE m_it;
    };

    using ITERATOR_TYPE = Iterator;
    using CONST_ITERATOR_TYPE = const Iterator;

public:
    explicit Vector(int capacity = DEFAULT_CAPACITY) :
        m_data(nullptr), m_capacity(capacity), m_size(0) {
        m_data = (POINTER_TYPE)MemoryAllocator::kmalloc(DEFAULT_CAPACITY * sizeof(VALUE_TYPE));
    }

    ~Vector() {
        MemoryAllocator::kfree(m_data);
    }

    CONST_REFERENCE_TYPE at(uint64_t index) const {
        if (index >= m_size) {
            Console::panic("Vector::at(): index out of bounds");
        }
        return m_data[index];
    }

    REFERENCE_TYPE operator[](uint64_t index) {
        if (index >= m_size) {
            Console::panic("Vector::at(): index out of bounds");
        }
        return m_data[index];
    }

    void pushBack(CONST_REFERENCE_TYPE value) {
        if (m_size == m_capacity) {
            auto newData = (POINTER_TYPE)MemoryAllocator::kmalloc(m_capacity * 2);
            memcpy(newData, m_data, m_size * sizeof(VALUE_TYPE));
            MemoryAllocator::kfree(m_data);
            m_data = newData;
        }
        m_data[m_size++] = value;
    }

    VALUE_TYPE popBack() {
        if (m_size == 0) {
            Console::panic("Vector::popBack(): vector is empty");
        }
        return m_data[--m_size];
    }

    bool empty() const {
        return m_size == 0;
    }

    uint64_t size() const {
        return m_size;
    }

    void reserve(uint64_t capacity) {
        if (m_data) {
            MemoryAllocator::kfree(m_data);
        }
        m_data = (POINTER_TYPE)MemoryAllocator::kmalloc(capacity * sizeof(VALUE_TYPE));
    }

    void clear() {
        if (!m_data) return;
        MemoryAllocator::kfree(m_data);
        m_data = nullptr;
    }

    REFERENCE_TYPE front() const {
        if (m_size == 0) {
            Console::panic("Vector::front(): vector is empty");
        }
        return m_data[0];
    }

    REFERENCE_TYPE back() const {
        if (m_size == 0) {
            Console::panic("Vector::back(): vector is empty");
        }
        return m_data[m_size - 1];
    }

    ITERATOR_TYPE begin() {
        return Iterator(m_data);
    }

    ITERATOR_TYPE end() {
        return Iterator(m_data + m_size);
    }

private:
    POINTER_TYPE m_data;
    uint64_t m_capacity;
    uint64_t m_size;

};

#endif