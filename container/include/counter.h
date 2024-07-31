#pragma once

#include "include/consumer.h"
#include "include/time_framed_container.h"
#include <unordered_map>

namespace vk::statshouse::container {

template<typename Clock = statshouse_clock_t>
class counter_t : public metrics_producer_t {
public:
  void produce(metrics_consumer_t& consumer) override {
    for (const auto& [k, v] : data.get_content()) {
      consumer.consume_counter(row_name, v, k);
    }
    data.clear();
  }

  void inc(double value = 1.0) { data.find_content(Clock::now()) += value; }

  void dec(double value = 1.0) { inc(-value); }

private:
  std::shared_ptr<row_name_t> row_name;
  details::time_framed_container_t<double, typename Clock::time_point> data;
};

} // namespace vk::statshouse::container
