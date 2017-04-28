#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "cache_policy.hpp"

namespace caches
{

// Base class for caching algorithms
template <typename Key, typename Value, typename Policy = NoCachePolicy<Key>>
class fixed_sized_cache
{
 public:

  using iterator = typename std::unordered_map<Key, Value>::iterator;

  using const_iterator =
      typename std::unordered_map<Key, Value>::const_iterator;

  using operation_guard = typename std::lock_guard<std::mutex>;

  fixed_sized_cache(
          size_t max_size,
          const Policy& policy = Policy())
      : max_cache_size{max_size},
        cache_policy(policy)
  {
    if (max_cache_size == 0)
    {
      max_cache_size = std::numeric_limits<size_t>::max();
    }
  }

  void Put(const Key& key, const Value& value)
  {
    operation_guard{safe_op};
    auto elem_it = FindElem(key);

    if (elem_it == cache_items_map.end())
    {
      // add new element to the cache
      if (Size() + 1 > max_cache_size)
      {
        auto disp_candidate_key = cache_policy.ReplCandidate();

        Erase(disp_candidate_key);
      }

      Insert(key, value);
    }
    else
    {
      // update previous value
      Update(key, value);
    }
  }

  bool Contains(const Key& key)
  {
    operation_guard{safe_op};
    auto elem_it = FindElem(key);
    return elem_it != cache_items_map.end();
  }

  const Value& Get(const Key& key) const
  {
    operation_guard{safe_op};
    auto elem_it = FindElem(key);

    if (elem_it == cache_items_map.end())
    {
      throw std::range_error{"No such element in the cache"};
    }
    cache_policy.Touch(key);

    return elem_it->second;
  }

  const size_t Size() const
  {
    operation_guard{safe_op};

    return cache_items_map.size();
  }

  // return a key of a displacement candidate
  void Clear()
  {
    operation_guard{safe_op};

    cache_policy.Clear();
    cache_items_map.clear();
  }

 protected:

  void Insert(const Key& key, const Value& value)
  {
    cache_policy.Insert(key);
    cache_items_map.emplace(std::make_pair(key, value));
  }

  void Erase(const Key& key)
  {
    cache_policy.Erase(key);
    cache_items_map.erase(key);
  }

  void Update(const Key& key, const Value& value)
  {
    cache_policy.Touch(key);
    cache_items_map[key] = value;
  }

  const_iterator FindElem(const Key& key) const
  {
    return cache_items_map.find(key);
  }


private:

  std::unordered_map<Key, Value> cache_items_map;

  mutable Policy cache_policy;
  mutable std::mutex safe_op;

  size_t max_cache_size;

};
}

#endif  // CACHE_HPP
