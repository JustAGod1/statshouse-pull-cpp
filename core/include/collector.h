#pragma once

#include "consumer.h"
#include "exposer.h"
#include <functional>
#include <mutex>
#include <unordered_set>

namespace vk::statshouse {

class statshouse_collector_t {
public:
  bool register_producer(metrics_producer_t* producer);
  bool unregister_producer(metrics_producer_t* producer);
  void collect();
  void clear();

  template<typename T>
  std::vector<std::pair<statshouse_clock_t::time_point, T>> expose(statshouse_exposer_t<T>& exposer) {
    std::lock_guard l{mutex};
    std::vector<std::pair<statshouse_clock_t::time_point, T>> result{};

    for (const auto& [k, v] : data) {
      exposer.consume(k, v);
    }
  }

private:
  std::mutex mutex;

  std::unordered_set<metrics_producer_t*> producers;
  std::unordered_map<statshouse_clock_t::time_point, std::vector<row_t>, statshouse_time_point_hash_t>
      data;

  class collector_consumer_t : public metrics_consumer_t {
  public:
    explicit collector_consumer_t(statshouse_collector_t& outer) : outer(outer) {}

  private:
    statshouse_collector_t& outer;

    void consume_counter(
        std::shared_ptr<row_name_t> row_name, double count, statshouse_clock_t::time_point time) override;
    void consume_values(
        std::shared_ptr<row_name_t> row_name,
        double count,
        std::span<const double> values,
        statshouse_clock_t::time_point time) override;
    void consume_uniques(
        std::shared_ptr<row_name_t> row_name,
        double count,
        std::span<const int64_t> uniques,
        statshouse_clock_t::time_point time) override;
  };

  friend collector_consumer_t;
};

} // namespace vk::statshouse
