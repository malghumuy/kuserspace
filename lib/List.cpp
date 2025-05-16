// Malghumuy - Library: kuserspace
#include "../include/List.h"
#include <algorithm>
#include <stdexcept>

namespace kuserspace {

// Helper function implementations
template<typename T>
void List<T>::initialize_empty() {
    head = nullptr;
    tail = nullptr;
    count = 0;
}

template<typename T>
void List<T>::cleanup() {
    while (head) {
        auto next = head->next;
        head->next = nullptr;
        head = next;
    }
    tail = nullptr;
    count = 0;
}

template<typename T>
std::shared_ptr<typename List<T>::Node> List<T>::create_node(const T& value) {
    return std::make_shared<Node>(value);
}

template<typename T>
std::shared_ptr<typename List<T>::Node> List<T>::create_node(T&& value) {
    return std::make_shared<Node>(std::move(value));
}

template<typename T>
void List<T>::link_nodes(std::shared_ptr<Node> prev, std::shared_ptr<Node> next) {
    if (prev) {
        prev->next = next;
        if (next) {
            next->prev = prev;
        }
    }
}

template<typename T>
void List<T>::unlink_node(std::shared_ptr<Node> node) {
    if (!node) return;
    
    auto prev = node->prev.lock();
    auto next = node->next;
    
    if (prev) {
        prev->next = next;
    } else {
        head = next;
    }
    
    if (next) {
        next->prev = prev;
    } else {
        tail = prev;
    }
    
    node->next = nullptr;
    node->prev.reset();
}

template<typename T>
bool List<T>::validate_node(std::shared_ptr<Node> node) const {
    return node && !node->marked;
}

// Constructor implementations
template<typename T>
List<T>::List(const List& other) : allocator(std::make_unique<NodeAllocator<T>>()) {
    std::shared_lock<std::shared_mutex> otherLock(other.mutex);
    for (const auto& item : other) {
        push_back(item);
    }
}

template<typename T>
List<T>::List(List&& other) noexcept 
    : head(std::move(other.head))
    , tail(std::move(other.tail))
    , count(other.count.load())
    , allocator(std::move(other.allocator)) {
    other.count = 0;
}

template<typename T>
List<T>::List(std::initializer_list<T> init) : allocator(std::make_unique<NodeAllocator<T>>()) {
    for (const auto& item : init) {
        push_back(item);
    }
}

// Assignment operator implementations
template<typename T>
List<T>& List<T>::operator=(const List& other) {
    if (this != &other) {
        std::unique_lock<std::shared_mutex> thisLock(mutex);
        std::shared_lock<std::shared_mutex> otherLock(other.mutex);
        
        clear();
        for (const auto& item : other) {
            push_back(item);
        }
    }
    return *this;
}

template<typename T>
List<T>& List<T>::operator=(List&& other) noexcept {
    if (this != &other) {
        std::unique_lock<std::shared_mutex> thisLock(mutex);
        std::unique_lock<std::shared_mutex> otherLock(other.mutex);
        
        clear();
        head = std::move(other.head);
        tail = std::move(other.tail);
        count = other.count.load();
        allocator = std::move(other.allocator);
        other.count = 0;
    }
    return *this;
}

// Capacity implementations
template<typename T>
bool List<T>::empty() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return count == 0;
}

template<typename T>
size_t List<T>::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return count;
}

// Element access implementations
template<typename T>
T& List<T>::front() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (!head) {
        throw std::out_of_range("List is empty");
    }
    return head->data;
}

template<typename T>
const T& List<T>::front() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (!head) {
        throw std::out_of_range("List is empty");
    }
    return head->data;
}

template<typename T>
T& List<T>::back() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (!tail) {
        throw std::out_of_range("List is empty");
    }
    return tail->data;
}

template<typename T>
const T& List<T>::back() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (!tail) {
        throw std::out_of_range("List is empty");
    }
    return tail->data;
}

// Modifier implementations
template<typename T>
void List<T>::push_front(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto node = create_node(value);
    node->next = head;
    if (head) {
        head->prev = node;
    } else {
        tail = node;
    }
    head = node;
    count++;
}

template<typename T>
void List<T>::push_front(T&& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto node = create_node(std::move(value));
    node->next = head;
    if (head) {
        head->prev = node;
    } else {
        tail = node;
    }
    head = node;
    count++;
}

template<typename T>
void List<T>::push_back(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto node = create_node(value);
    node->prev = tail;
    if (tail) {
        tail->next = node;
    } else {
        head = node;
    }
    tail = node;
    count++;
}

template<typename T>
void List<T>::push_back(T&& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto node = create_node(std::move(value));
    node->prev = tail;
    if (tail) {
        tail->next = node;
    } else {
        head = node;
    }
    tail = node;
    count++;
}

template<typename T>
void List<T>::pop_front() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!head) {
        throw std::out_of_range("List is empty");
    }
    auto oldHead = head;
    head = head->next;
    if (head) {
        head->prev.reset();
    } else {
        tail.reset();
    }
    unlink_node(oldHead);
    count--;
}

template<typename T>
void List<T>::pop_back() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!tail) {
        throw std::out_of_range("List is empty");
    }
    auto oldTail = tail;
    tail = tail->prev.lock();
    if (tail) {
        tail->next.reset();
    } else {
        head.reset();
    }
    unlink_node(oldTail);
    count--;
}

// Insert implementations
template<typename T>
typename List<T>::Iterator List<T>::insert(Iterator pos, const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!pos.current) {
        push_back(value);
        return Iterator(tail);
    }
    
    auto node = create_node(value);
    node->next = pos.current;
    node->prev = pos.current->prev;
    
    if (pos.current->prev.lock()) {
        pos.current->prev.lock()->next = node;
    } else {
        head = node;
    }
    
    pos.current->prev = node;
    count++;
    return Iterator(node);
}

template<typename T>
typename List<T>::Iterator List<T>::insert(Iterator pos, T&& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!pos.current) {
        push_back(std::move(value));
        return Iterator(tail);
    }
    
    auto node = create_node(std::move(value));
    node->next = pos.current;
    node->prev = pos.current->prev;
    
    if (pos.current->prev.lock()) {
        pos.current->prev.lock()->next = node;
    } else {
        head = node;
    }
    
    pos.current->prev = node;
    count++;
    return Iterator(node);
}

template<typename T>
typename List<T>::Iterator List<T>::insert(Iterator pos, size_t count, const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    Iterator result = pos;
    for (size_t i = 0; i < count; ++i) {
        result = insert(pos, value);
    }
    return result;
}

// Erase implementations
template<typename T>
typename List<T>::Iterator List<T>::erase(Iterator pos) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!pos.current) {
        return end();
    }
    
    auto next = pos.current->next;
    unlink_node(pos.current);
    count--;
    return Iterator(next);
}

template<typename T>
typename List<T>::Iterator List<T>::erase(Iterator first, Iterator last) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    while (first != last) {
        first = erase(first);
    }
    return last;
}

// Clear implementation
template<typename T>
void List<T>::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    while (head) {
        auto next = head->next;
        unlink_node(head);
        head = next;
    }
    tail.reset();
    count = 0;
}

// Operation implementations
template<typename T>
void List<T>::reverse() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    std::swap(head, tail);
    auto current = head;
    while (current) {
        auto next = current->next;
        current->next = current->prev.lock();
        current->prev = next;
        current = next;
    }
}

template<typename T>
void List<T>::sort() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    std::vector<T> temp;
    temp.reserve(count);
    for (const auto& item : *this) {
        temp.push_back(item);
    }
    std::sort(temp.begin(), temp.end());
    clear();
    for (const auto& item : temp) {
        push_back(item);
    }
}

template<typename T>
void List<T>::unique() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!head) return;
    
    auto current = head;
    while (current->next) {
        if (current->data == current->next->data) {
            auto next = current->next;
            unlink_node(next);
            count--;
        } else {
            current = current->next;
        }
    }
}

template<typename T>
void List<T>::merge(List& other) {
    std::unique_lock<std::shared_mutex> thisLock(mutex);
    std::unique_lock<std::shared_mutex> otherLock(other.mutex);
    
    auto thisIt = begin();
    auto otherIt = other.begin();
    
    while (otherIt != other.end()) {
        if (thisIt == end() || *otherIt < *thisIt) {
            insert(thisIt, *otherIt);
            ++otherIt;
        } else {
            ++thisIt;
        }
    }
    
    other.clear();
}

template<typename T>
void List<T>::splice(Iterator pos, List& other) {
    std::unique_lock<std::shared_mutex> thisLock(mutex);
    std::unique_lock<std::shared_mutex> otherLock(other.mutex);
    
    if (other.empty()) return;
    
    auto first = other.head;
    auto last = other.tail;
    
    // Remove from other list
    other.head = nullptr;
    other.tail = nullptr;
    other.count = 0;
    
    // Insert into this list
    if (!pos.current) {
        if (tail) {
            tail->next = first;
            first->prev = tail;
        } else {
            head = first;
        }
        tail = last;
    } else {
        if (pos.current->prev.lock()) {
            pos.current->prev.lock()->next = first;
            first->prev = pos.current->prev;
        } else {
            head = first;
        }
        pos.current->prev = last;
        last->next = pos.current;
    }
    
    count += other.count;
}

template<typename T>
void List<T>::remove(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto current = head;
    while (current) {
        auto next = current->next;
        if (current->data == value) {
            unlink_node(current);
            count--;
        }
        current = next;
    }
}

template<typename T>
template<typename Pred>
void List<T>::remove_if(Pred pred) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto current = head;
    while (current) {
        auto next = current->next;
        if (pred(current->data)) {
            unlink_node(current);
            count--;
        }
        current = next;
    }
}

// Search implementations
template<typename T>
typename List<T>::Iterator List<T>::find(const T& value) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto current = head;
    while (current) {
        if (current->data == value) {
            return Iterator(current);
        }
        current = current->next;
    }
    return end();
}

template<typename T>
typename List<T>::ConstIterator List<T>::find(const T& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto current = head;
    while (current) {
        if (current->data == value) {
            return ConstIterator(current);
        }
        current = current->next;
    }
    return cend();
}

template<typename T>
bool List<T>::contains(const T& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return find(value) != cend();
}

// Thread-safe operation implementations
template<typename T>
bool List<T>::try_push_front(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;
    
    push_front(value);
    return true;
}

template<typename T>
bool List<T>::try_push_back(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;
    
    push_back(value);
    return true;
}

template<typename T>
bool List<T>::try_pop_front(T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock() || !head) return false;
    
    value = head->data;
    pop_front();
    return true;
}

template<typename T>
bool List<T>::try_pop_back(T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock() || !tail) return false;
    
    value = tail->data;
    pop_back();
    return true;
}

template<typename T>
bool List<T>::try_insert(Iterator pos, const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;
    
    insert(pos, value);
    return true;
}

template<typename T>
bool List<T>::try_erase(Iterator pos) {
    std::unique_lock<std::shared_mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock() || !pos.current) return false;
    
    erase(pos);
    return true;
}

// Explicit template instantiations for common types
template class List<int>;
template class List<long>;
template class List<float>;
template class List<double>;
template class List<std::string>;

} // namespace kuserspace 
