#include "prange.hpp"

size_t points_range_length(const points_pair& range) {
  auto first_pair = std::minmax(range.first.latitude, range.second.latitude);
  size_t first = first_pair.second - first_pair.first;
  if (first < 0) first *= -1;

  auto second_pair = std::minmax(range.first.longitude, range.second.longitude);
  size_t second = second_pair.second - second_pair.first;
  if (second < 0) second *= -1;

  return (first + 1) * (second + 1);
}

bool merge_sets(interval_set& set1, interval_set::const_iterator begin, interval_set::const_iterator end) {
    size_t set2_size = end - begin;
    if (set1.empty() || !set2_size)
      return false;

    interval_set newset;
    newset.reserve(set1.size() + set2_size);
    auto out = newset.begin();

    auto it1 = set1.begin();
    auto it2 = begin;
    if (it1->first < it2->first)
        newset.push_back(*it1++);
    else
        newset.push_back(*it2++);

    while(it1 != set1.end() && it2 != end) {
        id_pair val = *it2;
        if (it2->first < it1->first) {
            ++it2;
        } else {
            val = *it1;
            ++it1;
        }
        if (val.first == out->second + 1)
          out->second = val.second;
        else {
          newset.push_back(val);
          ++out;
        }
    }
    if (it1 != end) {
        if (it1->first == out->second + 1)
            out->second = it1++->second;
        newset.insert(newset.end(), it1, set1.end());
    }
    if (it2 != end) {
        if (it2->first == out->second + 1)
            out->second = it2++->second;
        newset.insert(newset.end(), it2, end);
    }

    set1 = std::move(newset);
    return true;
}

interval_set::iterator insert_interval(interval_set& id_set, const id_pair& range) {
  if (range.first <= range.second)
    return id_set.end();

  auto first_less = [](const auto& a, const auto& b){
    return a < b.first;
  };

  auto start = upper_bound( id_set.begin(), id_set.end(), range.first,  first_less);
  auto end   = upper_bound( id_set.begin(), id_set.end(), range.second, first_less);

  if (start != id_set.begin() && prev(start)->second >= range.first) {
    start = prev(start);
  }

  if (start == end) {
    id_set.insert(start, range);
  } else {
    start->first = std::min(start->first, range.first);
    start->second = std::max(prev(end)->second, range.second);
    id_set.erase(next(start), end);
  }

  return start;
}

interval_set substract_amount(interval_set& id_set, int64_t amount) {
  if (id_set.empty()) return {};
  interval_set result;
  auto it = prev(id_set.end()), nend = id_set.end();
  int64_t accumulated = 0;
  int64_t interval_size = 0;
  for(; accumulated < amount; --it) {
    interval_size = it->second - it->first + 1;
    if (accumulated + interval_size <= amount) {
        accumulated += interval_size;
        result.push_back(*it);
        nend = rotate(it, next(it), nend);
    }
    if (it == id_set.begin())
        break;
  }
  if(nend == id_set.begin() && accumulated < amount)
    return {};
  // interval split
  id_set.erase(nend, id_set.end());
  --nend;
  int64_t remainder = amount - accumulated;
  if (remainder > 0) {
    id_type split = nend->second - remainder;
    id_pair tail{split + 1, nend->second};
    nend->second = split;
    auto it = upper_bound(result.begin(), result.end(), tail.first, [](const auto& a, const auto& b){
        return a > b.first;
    });
    result.insert(it, tail);
  }
  reverse(result.begin(), result.end());
  return result;
}
