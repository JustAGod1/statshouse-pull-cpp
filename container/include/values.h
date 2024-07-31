#pragma once

#include "include/consumer.h"
#include "include/time_framed_container.h"
#include <unordered_map>

namespace vk::statshouse::container {

template<typename Clock = statshouse_clock_t>
class values_t : public metrics_producer_t {
public:
  void produce(metrics_consumer_t& consumer) override {
    for (const auto& [k, v] : data.get_content()) {
      consumer.consume_values(row_name, static_cast<double>(v.size()), v, k);
    }
    data.clear();
  }

  void observe(double value) { data.find_content(Clock::now()).push_back(value); }

private:
  std::shared_ptr<row_name_t> row_name;

  details::time_framed_container_t<std::vector<double>, typename Clock::time_point> data;
};

} // namespace vk::statshouse::container
