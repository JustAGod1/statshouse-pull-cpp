#include "include/udp_exposer.h"

#include <arpa/inet.h>
#include <bits/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <unistd.h>

namespace vk::statshouse::exposer {

namespace {

enum {
  TL_INT_SIZE = 4,
  TL_LONG_SIZE = 8,
  TL_DOUBLE_SIZE = 8,
  TL_MAX_TINY_STRING_LEN = 253,
  TL_BIG_STRING_LEN = 0xffffff,
  TL_BIG_STRING_MARKER = 0xfe,
  TL_STATSHOUSE_METRICS_BATCH_TAG = 0x56580239,
  TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK = 1 << 0,
  TL_STATSHOUSE_METRIC_TS_FIELDS_MASK = 1 << 4,
  TL_STATSHOUSE_METRIC_VALUE_FIELDS_MASK = 1 << 1,
  TL_STATSHOUSE_METRIC_UNIQUE_FIELDS_MASK = 1 << 2,
  BATCH_HEADER_LEN = TL_INT_SIZE * 3 // TL tag, fields_mask, # of batches
};

size_t string_len(std::string_view str) {
  size_t len = str.size();
  if (len > TL_BIG_STRING_LEN) {
    len = TL_BIG_STRING_LEN;
  }
  // rounding up to 4
  size_t rounded_len = (len | 4) & ~3;
  if (len > TL_MAX_TINY_STRING_LEN) {
    return 1 + rounded_len;
  } else {
    return 4 + rounded_len;
  }
}

class tl_writer_t {
public:
  explicit tl_writer_t(std::span<uint8_t> buffer) : buffer(buffer) {}

  tl_writer_t fork() { return tl_writer_t{std::span{buffer.begin() + pos, buffer.end()}}; }

  template<typename T>
  T* ptr() {
    if (!has_room(sizeof(T))) {
      return nullptr;
    }
    return reinterpret_cast<T*>(&buffer[pos]);
  }

  bool write_int(int32_t v) { return write_primitive_little_endian(v); }

  bool write_double(double v) { return write_primitive_little_endian(v); }

  bool write_long(int64_t v) { return write_primitive_little_endian(v); }

  bool write_byte(uint8_t v) { return write_primitive_little_endian(v); }

  bool write_string(std::string_view str) {
    if (!has_room(string_len(str))) {
      return false;
    }

    uint32_t len = str.size();

    if (len > TL_BIG_STRING_LEN) {
      len = TL_BIG_STRING_LEN;
    }
    // rounding up to 4
    uint32_t rounded_len = (len | 4) & ~3;

    if (len > TL_MAX_TINY_STRING_LEN) {
      write_int((len << 8U) | TL_BIG_STRING_MARKER);
    } else {
      write_byte(len);
    }
    for (int i = 0; i < len; ++i) {
      write_byte_unsafe(str[i]);
    }

    // padding
    size_t remainder = rounded_len - len;
    for (int i = 0; i < remainder; ++i) {
      write_byte_unsafe(0);
    }

    return true;
  }

  [[nodiscard]] std::span<const uint8_t> get_written_data() const {
    return std::span{buffer.begin(), buffer.begin() + pos};
  }

  [[nodiscard]] bool has_room(size_t size) const { return pos + size < buffer.size(); }

private:
  size_t pos{0};
  std::span<uint8_t> buffer;

  void write_byte_unsafe(uint8_t v) { buffer[pos++] = v; }

  template<typename T>
  bool write_primitive_little_endian(T v) {
    int size = sizeof(T);
    if (!has_room(size)) {
      return false;
    }

    std::memcpy(&buffer[pos], &v, sizeof(v));

    pos += sizeof(v);

    return true;
  }
};

struct socket_info_t {
  int socket;
  size_t datagram_size = network_constants_t::DEFAULT_DATAGRAM_SIZE;
};

std::variant<socket_info_t, creation_error_t> create_socket(const std::string& ip, int port) {
  if (port < 0 || port > 0xffff) {
    return creation_error_t{"statshouse::TransportUDP invalid port=" + std::to_string(port)};
  }
  size_t datagram_size = network_constants_t::DEFAULT_DATAGRAM_SIZE;
  ::sockaddr_storage addr = {};
  auto ap = reinterpret_cast<sockaddr*>(&addr);
  auto ap6 = reinterpret_cast<sockaddr_in6*>(ap);
  auto ap4 = reinterpret_cast<sockaddr_in*>(ap);
  int ap_len = 0;
  if (inet_pton(AF_INET6, ip.c_str(), &ap6->sin6_addr) == 1) {
    addr.ss_family = AF_INET6;
    ap6->sin6_port = htons(uint16_t(port));
    ap_len = sizeof(*ap6);
    // TODO - check if localhost and increase default packet size
  } else if (inet_pton(AF_INET, ip.c_str(), &ap4->sin_addr) == 1) {
    addr.ss_family = AF_INET;
    ap4->sin_port = htons(uint16_t(port));
    ap_len = sizeof(*ap4);
    char high_byte = 0;
    std::memcpy(&high_byte, &ap4->sin_addr, 1); // this is correct, sin_addr in network byte order
    if (high_byte == 0x7F) {
      datagram_size = network_constants_t::MAX_DATAGRAM_SIZE;
    }
  } else {
    return creation_error_t{"statshouse::TransportUDP could not parse ip=" + ip};
  }
  auto sock = ::socket(addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    return creation_error_t{"statshouse::TransportUDP socket() failed", errno};
  }
  if (::connect(sock, ap, ap_len) < 0) {
    ::close(sock);
    return creation_error_t{"statshouse::TransportUDP connect() failed", errno};
  }
  return socket_info_t{sock, datagram_size};
}

bool write_row(tl_writer_t& writer, statshouse_clock_t::time_point time, const row_t& row) {
  uint32_t flags = 0;

  auto* flags_ptr = writer.ptr<uint32_t>();

  if (!writer.write_string(row.name->name)) {
    return false;
  }

  if (!writer.write_int(row.name->tags.size())) {
    return false;
  }

  for (const auto& item : row.name->tags) {
    if (!writer.write_string(item.first)) {
      return false;
    }
    if (!writer.write_string(item.second)) {
      return false;
    }
  }

  if (row.data.count != 0) {
    flags |= TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK;
    if (!writer.write_double(row.data.count)) {
      return false;
    }
  }

  flags |= TL_STATSHOUSE_METRIC_TS_FIELDS_MASK;
  if (!writer.write_int(time.time_since_epoch().count())) {
    return false;
  }

  if (!row.data.uniques.empty()) {
    flags |= TL_STATSHOUSE_METRIC_UNIQUE_FIELDS_MASK;
    if (!writer.write_int(row.data.uniques.size())) {
      return false;
    }
    for (const auto item : row.data.uniques) {
      if (!writer.write_long(item)) {
        return false;
      }
    }
  }
  if (!row.data.values.empty()) {
    flags |= TL_STATSHOUSE_METRIC_VALUE_FIELDS_MASK;
    if (!writer.write_int(row.data.values.size())) {
      return false;
    }
    for (const auto item : row.data.values) {
      if (!writer.write_double(item)) {
        return false;
      }
    }
  }

  *flags_ptr = flags;

  return true;
}

int flush_some(
    int socket,
    statshouse_clock_t::time_point time,
    std::span<const row_t> row,
    flush_statistics_t& statistics,
    size_t datagram_size) {

  static std::array<uint8_t, network_constants_t::MAX_DATAGRAM_SIZE> datagram{};

  tl_writer_t root({datagram.begin(), datagram.begin() + datagram_size});

  root.write_int(TL_STATSHOUSE_METRICS_BATCH_TAG);
  root.write_int(0); // fields mask

  auto* size_ptr = root.ptr<uint32_t>();

  tl_writer_t tail = root;
  int i;
  for (i = 0; i < row.size(); ++i) {
    const auto& item = row[i];
    tl_writer_t fork = root.fork();
    if (write_row(fork, time, item)) {
      tail = fork;
    } else {
      break;
    }
  }

  *size_ptr = i;

  std::span<const uint8_t> written = tail.get_written_data();
  ssize_t result = ::sendto(socket, written.data(), written.size(), 0, nullptr, 0);


  if (result >= 0) {
    return i;
  }
  auto err = errno;
  if (err == EAGAIN) { //  || err == EWOULDBLOCK
    statistics.overflow++;
  } else {
    statistics.errors.emplace_back("statshouse::TransportUDP sendto() failed", err);
  }

  return i;
}

} // namespace

flush_statistics_t udp_exposer_t::consume(statshouse_clock_t::time_point time, std::span<const row_t> rows) {
  flush_statistics_t stats{};

  int offset = 0;
  while (offset < rows.size()) {
    std::span<const row_t> slice{rows.begin()+offset,rows.end()};
    offset += flush_some(socket, time, slice, stats, datagram_size);
  }

  return stats;
}

std::variant<udp_exposer_t, creation_error_t> udp_exposer_t::create(std::string& ip, int port) {
  auto socket = create_socket(ip, port);
  if (std::holds_alternative<creation_error_t>(socket)) {
    return get<creation_error_t>(socket);
  }
  socket_info_t sock = std::get<socket_info_t>(socket);

  return udp_exposer_t{sock.socket, sock.datagram_size};
}

} // namespace vk::statshouse::exposer
