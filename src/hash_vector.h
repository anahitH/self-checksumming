#pragma once

#include <vector>
#include <unordered_map>

class BPatch_basicBlock;

namespace selfchecksum {

template <class T>
class hash_vector : private std::vector<T>
{
public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;

public:
    hash_vector() = default;
    hash_vector(unsigned size);

public:
    bool insert(unsigned pos, const T& value);
    bool push_back(const T& value);
    void pop_back();
    iterator erase(unsigned index);
    void erase(const T& value);
    const T& get(unsigned index) const;
    bool contains(const T& value) const;
    void clear();

    template <class InputIterator>
    void push_back(InputIterator first, InputIterator last);
    //unsigned size() const;

public:
    using std::vector<T>::reserve;
    using std::vector<T>::resize;
    using std::vector<T>::size;
    using std::vector<T>::capacity;
    using std::vector<T>::empty;
    using std::vector<T>::back;
    using std::vector<T>::front;
    using std::vector<T>::begin;
    using std::vector<T>::end;
    using std::vector<T>::cbegin;
    using std::vector<T>::cend;


private:
    std::unordered_map<T, unsigned> value_index;
};

template <class T>
hash_vector<T>::hash_vector(unsigned size)
    : std::vector<T>(size)
    , value_index(size)
{
}

template <class T>
bool hash_vector<T>::insert(unsigned pos, const T& value)
{
    if (value_index.insert(std::make_pair(value, pos)).second) {
        std::vector<T>::operator[](pos) = value;
        return true;
    }
    return false;
}

template <class T>
bool hash_vector<T>::push_back(const T& value)
{
    if (value_index.insert(std::make_pair(value, size())).second) {
        std::vector<T>::push_back(value);
        return true;
    }
    return false;
}

template <class T>
void hash_vector<T>::pop_back()
{
   auto val = back(); 
   std::vector<T>::pop_back();
   value_index.erase(val);
}

template <class T>
typename hash_vector<T>::iterator hash_vector<T>::erase(unsigned index)
{
    auto val = back();
    auto old_val = std::vector<T>::operator[](index);
    std::vector<T>::operator[](index) = val;
    value_index[val] = index;
    value_index.erase(old_val);
    std::vector<T>::pop_back();
}

template <class T>
void hash_vector<T>::erase(const T& value)
{
    auto pos = value_index.find(value);
    if (pos == value_index.end()) {
        return;
    }
    erase(pos->second);
}


template <class T>
const T& hash_vector<T>::get(unsigned index) const
{
    return std::vector<T>::operator[](index);
}

template <class T>
bool hash_vector<T>::contains(const T& value) const
{
    return value_index.find(value) != value_index.end();
}

template <class T>
void hash_vector<T>::clear()
{
    std::vector<T>::clear();
    value_index.clear();
}

template <class T>
template <class InputIterator>
void hash_vector<T>::push_back(InputIterator first, InputIterator last)
{
    while (first != last) {
        push_back(*first);
        ++first;
    }
}

//template <class T>
//unsigned hash_vector<T>::size() const
//{
//    return value_index.size();
//}
//
} // namespace selfchecksum

