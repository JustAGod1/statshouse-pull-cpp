#include "include/core_types.h"

vk::statshouse::statshouse_clock_t::time_point vk::statshouse::statshouse_clock_t::now() noexcept {
  return vk::statshouse::statshouse_clock_t::time_point(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()));
}
