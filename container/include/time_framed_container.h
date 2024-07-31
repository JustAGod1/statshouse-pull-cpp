#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace vk::statshouse::container::details {

template<typename InnerContent, typename TimePoint>
class time_framed_container_t {
public:
  InnerContent& find_content(TimePoint time) {
    if (last.has_value()) {
      if (last->time_point == time) {
        return last->content;
      }
    }

    InnerContent& v = container[time];

    last.emplace(time, v);

    return v;
  }

  void clear() {
    container.clear();
    last.reset();
  }

private:
  struct time_point_hash_t {
    size_t operator()(const TimePoint& v) const { return v.time_since_epoch().count(); }
  };

  struct last_accessed_info_t {
    TimePoint time_point;
    InnerContent& content;
  };

  std::optional<last_accessed_info_t> last;

  std::unordered_map<TimePoint, InnerContent, time_point_hash_t> container;

public:

  const std::unordered_map<TimePoint, InnerContent, time_point_hash_t>& get_content() const {
    return container;
  }

};

} // namespace vk::statshouse::container::details
