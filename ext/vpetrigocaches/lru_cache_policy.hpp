#ifndef LRU_CACHE_POLICY_HPP
#define LRU_CACHE_POLICY_HPP

#include <list>
#include <unordered_map>
#include "cache_policy.hpp"

namespace caches
{
template <typename Key>
class LRUCachePolicy : public ICachePolicy<Key>
{

 public:

  using lru_iterator = typename std::list<Key>::const_iterator;
  
  LRUCachePolicy() = default;
  ~LRUCachePolicy() = default;
  
  void Insert(const Key& key) override
  {
    lru_queue.emplace_front(key);
    key_finder[key] = lru_queue.cbegin();
  }
  
  void Touch(const Key& key) override
  {
    // move the touched element at the beginning of the lru_queue 
    lru_queue.splice(lru_queue.cbegin(), lru_queue, key_finder[key]);
  }
  
  void Erase(const Key& key) override
  {
    // remove the least recently used element
    key_finder.erase(lru_queue.back());

    lru_queue.pop_back();
  }

  // return a key of a displacement candidate
  const Key& ReplCandidate() const override
  {
    return lru_queue.back();
  }

  // return a key of a displacement candidate
  void Clear() override
  {
      lru_queue.clear();
      key_finder.clear();
  }
  
 private:

  std::list<Key> lru_queue;

  std::unordered_map<Key, lru_iterator> key_finder; 
};

} // namespace caches

#endif // LRU_CACHE_POLICY_HPP
