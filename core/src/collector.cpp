#include "include/collector.h"

namespace vk::statshouse {

bool statshouse_collector_t::register_producer(metrics_producer_t* producer) {
  std::lock_guard l{mutex};
  return this->producers.insert(producer).second;
}

bool statshouse_collector_t::unregister_producer(metrics_producer_t* producer) {
  std::lock_guard l{mutex};
  return this->producers.erase(producer) > 0;
}

void statshouse_collector_t::collect() {
  std::lock_guard l{mutex};
  collector_consumer_t consumer{*this};
  for (const auto& producer : producers) {
    producer->produce(consumer);
  }
}

void statshouse_collector_t::clear() {
  std::lock_guard l{mutex};
  data.clear();
}

void statshouse_collector_t::expose(statshouse_exposer_t& exposer) {
  std::lock_guard l{mutex};
  for (const auto& [k, v] : data) {
    exposer.consume(k, v);
  }
}

void statshouse_collector_t::collector_consumer_t::consume_counter(
    std::shared_ptr<row_name_t> row_name, double count, statshouse_clock_t::time_point time) {
  outer.data[time].emplace_back(
      std::move(row_name),
      row_data_t{
          .count = count,
      });
}

void statshouse_collector_t::collector_consumer_t::consume_values(
    std::shared_ptr<row_name_t> row_name,
    double count,
    std::span<double> values,
    statshouse_clock_t::time_point time) {
  outer.data[time].emplace_back(
      std::move(row_name),
      row_data_t{.count = count, .values = std::vector<double>{values.begin(), values.end()}});
}

void statshouse_collector_t::collector_consumer_t::consume_uniques(
    std::shared_ptr<row_name_t> row_name,
    double count,
    std::span<int64_t> uniques,
    statshouse_clock_t::time_point time) {
  outer.data[time].emplace_back(
      std::move(row_name),
      row_data_t{.count = count, .uniques = std::vector<int64_t>{uniques.begin(), uniques.end()}});
}

} // namespace vk::statshouse
