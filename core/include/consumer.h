#pragma once

#include "core_types.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

namespace vk::statshouse {

struct metrics_consumer_t {
  virtual void consume_counter(
      std::shared_ptr<row_name_t> row_name, double count, statshouse_clock_t::time_point time) = 0;
  virtual void consume_values(
      std::shared_ptr<row_name_t> row_name,
      double count,
      std::span<const double> values,
      statshouse_clock_t::time_point time) = 0;
  virtual void consume_uniques(
      std::shared_ptr<row_name_t> row_name,
      double count,
      std::span<const int64_t> uniques,
      statshouse_clock_t::time_point time) = 0;
};

struct metrics_producer_t {
  virtual void produce(metrics_consumer_t& consumer) = 0;
};

} // namespace vk::statshouse
