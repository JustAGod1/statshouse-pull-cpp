#pragma once

#include "include/exposer.h"

namespace vk::statshouse::exposer {

struct flush_error_t {
  std::string description;
  int error_num;
};
struct flush_statistics_t {
  size_t bytes_written;
  size_t overflow;
  size_t datagrams_sent; // including errored ones
  std::vector<flush_error_t> errors;
};

struct creation_error_t {
  std::string msg;
  int err_code; // errno
};

struct network_constants_t {
  static constexpr int DEFAULT_PORT = 13337;
  // https://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
  static constexpr int SAFE_DATAGRAM_SIZE = 508;
  static constexpr int DEFAULT_DATAGRAM_SIZE = 1232;
  // https://stackoverflow.com/questions/42609561/udp-maximum-packet-size/42610200
  static constexpr int MAX_DATAGRAM_SIZE = 65507;
  static constexpr int MAX_KEYS = 16;
  // roughly metric plus all tags
  static constexpr int MAX_FULL_KEY_SIZE = 1024;
};

class udp_exposer_t : public statshouse_exposer_t<flush_statistics_t> {
public:
  static std::variant<udp_exposer_t, creation_error_t> create(std::string& ip, int port);

  flush_statistics_t consume(statshouse_clock_t::time_point time, std::span<const row_t> row) override;
  void set_datagram_size(size_t size) { this->datagram_size = size; }

private:
  udp_exposer_t(int socket, size_t datagram_size) : socket(socket), datagram_size(datagram_size) {}

  int socket;
  size_t datagram_size;
};

} // namespace vk::statshouse::exposer
