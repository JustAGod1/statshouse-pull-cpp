#include "include/counter.h"
#include "include/values.h"

namespace vk::statshouse::container {

template class counter_t<statshouse_clock_t>;
template class values_t<statshouse_clock_t>;

}
