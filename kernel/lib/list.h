#ifndef RISC_V_LIST_H
#define RISC_V_LIST_H
#include "../io/console/console.h"
#include "../mm/kalloc/kmem_cache.h"

template <typename T>
class List {
    using REFERENCE_TYPE = T&;
    using CONST_REFERENCE_TYPE = const T&;
    using POINTER_TYPE = T*;
    using CONST_POINTER_TYPE = const T*;
    using VALUE_TYPE = T;

    struct Node {
        VALUE_TYPE value;
        Node* next;
        Node* prev;

        void* operator new(size_t size) {
            if (!s_nodeCache) {
                s_nodeCache = new KMemCache<Node>();
            }
            return s_nodeCache->alloc();
        }

        void operator delete(void* ptr) {
            s_nodeCache->free(ptr);
        }

    private:
        inline static KMemCache<Node>* s_nodeCache = nullptr;
    };

    class Iterator {
    public:
        explicit Iterator(Node* node) : m_node(node) {}

        REFERENCE_TYPE operator*() const {
            return m_node->value;
        }

        REFERENCE_TYPE operator->() const {
            return m_node->value;
        }

        Iterator& operator++() {
            m_node = m_node->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            m_node = m_node->next;
            return tmp;
        }

        Iterator& operator--() {
            m_node = m_node->prev;
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            m_node = m_node->prev;
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.m_node == b.m_node;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.m_node != b.m_node;
        }

    private:
        friend class List;
        Node* m_node;
    };

    using ITERATOR_TYPE = Iterator;
    using CONST_ITERATOR_TYPE = const Iterator;

public:
    List() : m_head(nullptr), m_tail(nullptr), m_size(0) {}

    ~List() {
        while (m_head) {
            Node* tmp = m_head;
            m_head = m_head->next;
            delete tmp;
        }
    }

    void addLast(REFERENCE_TYPE value) {
        Node* newNode = new Node{.value = value, .next = nullptr, .prev = nullptr};
        if (m_tail) {
            m_tail->next = newNode;
            newNode->prev = m_tail;
            m_tail = newNode;
        }
        else {
            m_head = newNode;
            m_tail = newNode;
        }
        m_size++;
    }

    void addFirst(REFERENCE_TYPE value) {
        Node* newNode = new Node{.value = value, .next = nullptr, .prev = nullptr};
        if (m_head) {
            newNode->next = m_head;
            m_head->prev = newNode;
            m_head = newNode;
        }
        else {
            m_tail = newNode;
            m_head = newNode;
        }
        m_size++;
    }

    VALUE_TYPE removeLast() {
        if (m_tail) {
            REFERENCE_TYPE ret = m_tail->value;
            Node* tmp = m_tail;
            m_tail = m_tail->prev;
            delete tmp;
            if (m_tail) {
                m_tail->next = nullptr;
            }
            else {
                m_head = nullptr;
            }
            m_size--;
            return ret;
        }
        Console::panic("List::removeLast(): list is empty");
    }

    VALUE_TYPE removeFirst() {
        if (m_head) {
            REFERENCE_TYPE ret = m_head->value;
            Node* tmp = m_head;
            m_head = m_head->next;
            delete tmp;
            if (m_head) {
                m_head->prev = nullptr;
            }
            else {
                m_tail = nullptr;
            }
            m_size--;
            return ret;
        }
        Console::panic("List::removeFirst(): list is empty");
    }

    bool empty() const { return m_size == 0; }

    uint64_t size() const { return m_size; }

    ITERATOR_TYPE begin() {
        return Iterator(m_head);
    }

    ITERATOR_TYPE end() {
        return Iterator(nullptr);
    }

    VALUE_TYPE remove(ITERATOR_TYPE it) {
        Node* node = it.m_node;
        if (node->prev)
            node->prev->next = node->next;
        else
            m_head = node->next;
        if (node->next)
            node->next->prev = node->prev;
        else
            m_tail = node->prev;
        m_size--;
        VALUE_TYPE ret = node->value;
        delete node;
        return ret;
    }

private:
    Node* m_head;
    Node* m_tail;
    uint64_t m_size;

};

#endif
