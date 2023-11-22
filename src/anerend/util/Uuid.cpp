#include "Uuid.h"

#include <uuid.h>

namespace util {

Uuid::Uuid(std::array<std::uint8_t, 16> data)
{
  _data = std::move(data);
}

Uuid::Uuid(const Uuid& rhs)
{
  _data = rhs._data;
}

Uuid::Uuid(Uuid&& rhs)
{
  std::swap(_data, rhs._data);
}

Uuid& Uuid::operator=(const Uuid& rhs)
{
  if (this != &rhs) {
    _data = rhs._data;
  }

  return *this;
}

Uuid& Uuid::operator=(Uuid&& rhs)
{
  if (this != &rhs) {
    if (!rhs._data.empty()) {
      std::swap(rhs._data, _data);
    }
  }

  return *this;
}

Uuid::operator bool() const
{
  for (size_t i = 0; i < _data.size(); ++i) if (_data[i] != 0) return true;
  return false;
}

bool Uuid::operator==(const Uuid& rhs)
{
  return rhs._data == _data;
}

bool Uuid::operator!=(const Uuid& rhs)
{
  return !(*this == rhs);
}

Uuid Uuid::generate()
{
  Uuid id;
  auto& bytes = uuids::uuid_system_generator{}().as_bytes();
  std::memcpy(id._data.data(), bytes.data(), 16);
  return id;
}

std::string Uuid::str() const
{
  return uuids::to_string(uuids::uuid(_data));
}

std::size_t Uuid::hash() const
{
  std::uint64_t l =
    static_cast<std::uint64_t>(_data[0]) << 56 |
    static_cast<std::uint64_t>(_data[1]) << 48 |
    static_cast<std::uint64_t>(_data[2]) << 40 |
    static_cast<std::uint64_t>(_data[3]) << 32 |
    static_cast<std::uint64_t>(_data[4]) << 24 |
    static_cast<std::uint64_t>(_data[5]) << 16 |
    static_cast<std::uint64_t>(_data[6]) << 8 |
    static_cast<std::uint64_t>(_data[7]);
  std::uint64_t h =
    static_cast<std::uint64_t>(_data[8]) << 56 |
    static_cast<std::uint64_t>(_data[9]) << 48 |
    static_cast<std::uint64_t>(_data[10]) << 40 |
    static_cast<std::uint64_t>(_data[11]) << 32 |
    static_cast<std::uint64_t>(_data[12]) << 24 |
    static_cast<std::uint64_t>(_data[13]) << 16 |
    static_cast<std::uint64_t>(_data[14]) << 8 |
    static_cast<std::uint64_t>(_data[15]);

  return l ^ h;
}

std::vector<std::uint8_t> Uuid::bytes() const
{
  std::vector<std::uint8_t> out(_data.begin(), _data.end());
  return out;
}

}