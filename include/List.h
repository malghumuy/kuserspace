// Malghumuy - Library: kuserspace
#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <functional>
#include <iterator>

namespace kuserspace {

template<typename T>
class List {
public:
    // Node structure
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;  // For doubly linked list
        std::atomic<bool> marked{false};  // For safe deletion
        
        explicit Node(const T& value) : data(value) {}
        explicit Node(T&& value) : data(std::move(value)) {}
    };

    // Iterator class
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() = default;
        explicit Iterator(std::shared_ptr<Node> node) : current(node) {}

        reference operator*() { return current->data; }
        pointer operator->() { return &current->data; }
        
        Iterator& operator++() {
            current = current->next;
            return *this;
        }
        
        Iterator operator++(int) {
            Iterator tmp = *this;
            current = current->next;
            return tmp;
        }
        
        bool operator==(const Iterator& other) const {
            return current == other.current;
        }
        
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }

    private:
        std::shared_ptr<Node> current;
        friend class List;
    };

    // Const Iterator class
    class ConstIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        ConstIterator() = default;
        explicit ConstIterator(std::shared_ptr<Node> node) : current(node) {}

        reference operator*() const { return current->data; }
        pointer operator->() const { return &current->data; }
        
        ConstIterator& operator++() {
            current = current->next;
            return *this;
        }
        
        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            current = current->next;
            return tmp;
        }
        
        bool operator==(const ConstIterator& other) const {
            return current == other.current;
        }
        
        bool operator!=(const ConstIterator& other) const {
            return current != other.current;
        }

    private:
        std::shared_ptr<Node> current;
        friend class List;
    };

    // Constructors
    List() = default;
    List(const List& other);
    List(List&& other) noexcept;
    List(std::initializer_list<T> init);

    // Assignment operators
    List& operator=(const List& other);
    List& operator=(List&& other) noexcept;

    // Destructor
    ~List() = default;

    // Iterators
    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(head); }
    ConstIterator end() const { return ConstIterator(nullptr); }
    ConstIterator cbegin() const { return ConstIterator(head); }
    ConstIterator cend() const { return ConstIterator(nullptr); }

    // Capacity
    bool empty() const;
    size_t size() const;

    // Element access
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;

    // Modifiers
    void push_front(const T& value);
    void push_front(T&& value);
    void push_back(const T& value);
    void push_back(T&& value);
    void pop_front();
    void pop_back();
    
    // Insert operations
    Iterator insert(Iterator pos, const T& value);
    Iterator insert(Iterator pos, T&& value);
    Iterator insert(Iterator pos, size_t count, const T& value);
    
    // Erase operations
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);
    
    // Clear
    void clear();

    // Operations
    void reverse();
    void sort();
    void unique();
    void merge(List& other);
    void splice(Iterator pos, List& other);
    void remove(const T& value);
    template<typename Pred>
    void remove_if(Pred pred);

    // Search operations
    Iterator find(const T& value);
    ConstIterator find(const T& value) const;
    bool contains(const T& value) const;

    // Thread-safe operations
    bool try_push_front(const T& value);
    bool try_push_back(const T& value);
    bool try_pop_front(T& value);
    bool try_pop_back(T& value);
    bool try_insert(Iterator pos, const T& value);
    bool try_erase(Iterator pos);

private:
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
    std::atomic<size_t> count{0};
    
    mutable std::shared_mutex mutex;  // For read-write locking
    
    // Helper functions
    void initialize_empty();
    void cleanup();
    std::shared_ptr<Node> create_node(const T& value);
    std::shared_ptr<Node> create_node(T&& value);
    void link_nodes(std::shared_ptr<Node> prev, std::shared_ptr<Node> next);
    void unlink_node(std::shared_ptr<Node> node);
    bool validate_node(std::shared_ptr<Node> node) const;
};

} // namespace kuserspace
