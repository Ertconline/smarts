#pragma once

#include <utility>
#include <vector>

typedef uint64_t id_type;

struct point {
  int64_t latitude;
  int64_t longitude;
};

typedef std::pair<point,point>     points_pair;
typedef std::pair<id_type,id_type> id_pair;
typedef std::vector<id_pair>       interval_set;

size_t points_range_length(const points_pair& range);
bool merge_sets(interval_set& set1, interval_set::const_iterator begin, interval_set::const_iterator end);
interval_set::iterator insert_interval(interval_set& id_set, const id_pair& range);
interval_set substract_amount(interval_set& id_set, int64_t amount);
