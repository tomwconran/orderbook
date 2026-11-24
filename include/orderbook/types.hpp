#ifndef ORDERBOOK_TYPES_HPP
#define ORDERBOOK_TYPES_HPP

#include <algorithm>
#include <compare> // IWYU pragma: keep
#include <cstdint>
#include <stdexcept>

namespace orderbook {

enum class Side {
  buy,
  sell,
};

class Price {
private:
  std::int64_t m_value{};

public:
  explicit Price(std::int64_t val) : m_value{val} {
    if (!(val > 0))
      throw std::invalid_argument("Price: value must be positive");
  }
  std::int64_t value() const { return m_value; }

  auto operator<=>(const Price &other) const = default;
};

class Quantity {
private:
  std::int64_t m_value{};

public:
  explicit Quantity(std::int64_t val) : m_value{val} {
    if (val < 0)
      throw std::invalid_argument("Quantity: value must be non negative");
  }
  std::int64_t value() const { return m_value; }

  auto operator<=>(const Quantity &other) const = default;
  Quantity &operator-=(const Quantity &other) {
    if (other.value() > m_value)
      throw std::invalid_argument("Quantity: quantity must be non negative");

    m_value -= other.value();
    return *this;
  }
  Quantity &operator+=(const Quantity &other) {
    m_value += other.value();
    return *this;
  }
};

class Order {
private:
  std::int64_t m_id{};
  Side m_side{};
  Price m_price;
  Quantity m_quantity;

public:
  explicit Order(std::int64_t id, Side side, Price price, Quantity quantity)
      : m_id{id}, m_side{side}, m_price{price}, m_quantity{quantity} {
    if (!(m_quantity.value() > 0))
      throw std::invalid_argument("Order: quantity must be positive");
  }

  Quantity fill(const Quantity &other) {
    Quantity amount = std::min(m_quantity, other);
    m_quantity -= amount;
    return amount;
  }

  bool isFilled() const { return m_quantity.value() == 0; }
  std::int64_t id() const { return m_id; }
  Side side() const { return m_side; }
  Price price() const { return m_price; }
  Quantity quantity() const { return m_quantity; }
};

struct Trade {
  std::int64_t buyId{};
  std::int64_t sellId{};
  Price price;
  Quantity quantity;
};

} // namespace orderbook

#endif