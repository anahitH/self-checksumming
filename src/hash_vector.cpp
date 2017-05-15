#include "hash_vector.h"

namespace selfchecksum {

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

}

