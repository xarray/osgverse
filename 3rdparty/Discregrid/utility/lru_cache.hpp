#pragma once

#include <cassert> 
#include <list> 
#include <map>
 
// Class providing fixed-size (by number of records) 
// LRU-replacement cache of a function with signature 
// V f(K). 
// MAP should be one of std::map or std::unordered_map. 
// Variadic template args used to deal with the 
// different type argument signatures of those 
// containers; the default comparator/hash/allocator 
// will be used. 
template <typename K, typename V>
class LRUCache
{

public: 
 
    using key_type = K;
    using value_type = V;
 
    // Key access history, most recent at back 
    using key_tracker_type = std::list<key_type>; 
 
    // Key to value and key history iterator 
    //using key_to_value_type = MAP<key_type, std::pair<value_type, 
    //    typename key_tracker_type::iterator>>;
    using key_to_value_type = std::map<key_type, std::pair<value_type, 
        typename key_tracker_type::iterator>>;

 
    using eval_func_type = std::function<value_type(const key_type&)>;

    // Constuctor specifies the cached function and 
    // the maximum number of records to be stored 
    LRUCache(eval_func_type  const& f, std::size_t c) 
    : _fn(f), _capacity(c) 
    { 
        assert(_capacity!=0); 
    } 
 
    // Obtain value of the cached function for k 
    value_type operator()(const key_type& k)
    {
        // Attempt to find existing record 
        auto it =_key_to_value.find(k); 
 
        if (it == _key_to_value.end())
        { 
            // We don't have it: 
 
            // Evaluate function and create new record 
            auto v = _fn(k); 
            insert(k,v); 
 
            // Return the freshly computed value 
            return v; 
 
        }
        else
        {
            // We do have it:  
            // Update access record by moving 
            // accessed key to back of list 
            _key_tracker.splice(_key_tracker.end(), _key_tracker, (*it).second.second);
 
            // Return the retrieved value 
            return (*it).second.first; 
        } 
    } 
 
    // Obtain the cached keys, most recently used element 
    // at head, least recently used at tail. 
    // This method is provided purely to support testing. 
    template <typename IT>
    void getKeys(IT dst) const
    {
        auto src =_key_tracker.rbegin(); 
        while (src!=_key_tracker.rend())
        {
            *dst++ = *src++; 
        }
    } 
 
private: 
 
    // Record a fresh key-value pair in the cache 
    void insert(key_type const& k, value_type const& v)
    { 
 
        // Method is only called on cache misses 
        //assert(_key_to_value.find(k)==_key_to_value.end()); 
 
        // Make space if necessary 
        if (_key_to_value.size() == _capacity) 
            evict(); 
 
        // Record k as most-recently-used key 
        auto it =_key_tracker.insert(_key_tracker.end(),k); 
 
        // Create the key-value entry, 
        // linked to the usage record. 
        _key_to_value.insert(std::make_pair(k, std::make_pair(v,it))); 
        // No need to check return, 
        // given previous assert. 
    } 
 
    // Purge the least-recently-used element in the cache 
    void evict() 
    {
        // Assert method is never called when cache is empty 
        assert(!_key_tracker.empty()); 
 
        // Identify least recently used key 
        auto it =_key_to_value.find(_key_tracker.front()); 
        assert(it!=_key_to_value.end()); 
 
        // Erase both elements to completely purge record 
        _key_to_value.erase(it); 
        _key_tracker.pop_front(); 
    } 
 
    // The function to be cached 
    eval_func_type _fn; 
 
    // Maximum number of key-value pairs to be retained 
    //const size_t _capacity; 
    size_t _capacity; 
 
    // Key access history 
    key_tracker_type _key_tracker; 
 
    // Key-to-value lookup 
    key_to_value_type _key_to_value; 
};
