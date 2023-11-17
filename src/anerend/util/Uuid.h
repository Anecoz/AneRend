#pragma once 

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace util {

class Uuid
{
public:
  // Default constructed Uuid is invalid, i.e. operator bool will return false.
  Uuid() = default;
  ~Uuid() = default;
  Uuid(std::array<std::uint8_t, 16> data);

  Uuid(const Uuid&);
  Uuid(Uuid&&);
  Uuid& operator=(const Uuid&);
  Uuid& operator=(Uuid&&);

  explicit operator bool() const;

  bool operator==(const Uuid&);
  bool operator!=(const Uuid&);

  static Uuid generate();

  std::string str() const;
  std::size_t hash() const;
  std::vector<std::uint8_t> bytes() const;

  // danger zone
  std::uint8_t* rawPtr() { return _data.data(); }

private:
  std::array<std::uint8_t, 16> _data{ {0} };

  friend bool operator==(const Uuid& lhs, const Uuid& rhs) noexcept;
};

inline bool operator==(const Uuid& lhs, const Uuid& rhs) noexcept
{
  return lhs._data == rhs._data;
}

}

namespace std {

template<>
struct hash<util::Uuid>
{
  inline std::size_t operator()(const util::Uuid& uuid) const { return uuid.hash(); }
};

}
