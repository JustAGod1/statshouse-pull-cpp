#pragma once

#include <chrono>

namespace vk::statshouse {

struct statshouse_clock_t {
  using duration = std::chrono::seconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<statshouse_clock_t, duration>;

  static time_point
  now() noexcept;
};

struct statshouse_time_point_hash_t {
  size_t operator()(const statshouse_clock_t::time_point v) const { return v.time_since_epoch().count(); }
};

struct row_data_t {
  double count{};
  std::vector<double> values{};
  std::vector<int64_t> uniques{};
};

struct row_name_t {
  const std::vector<std::pair<std::string, std::string>> tags;
  const std::string name;
};

struct row_t {
  std::shared_ptr<row_name_t> name;
  row_data_t data;
};

} // namespace vk::statshouse
