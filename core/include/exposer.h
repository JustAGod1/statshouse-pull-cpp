#pragma once

#include "core_types.h"

namespace vk::statshouse {

template<typename Output>
class statshouse_exposer_t {
public:
  virtual Output consume(statshouse_clock_t::time_point time, std::span<const row_t> row) = 0;
};

} // namespace vk::statshouse
