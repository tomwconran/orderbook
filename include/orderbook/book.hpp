#ifndef ORDERBOOK_BOOK_HPP
#define ORDERBOOK_BOOK_HPP

#include "types.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace orderbook {
class Book {
public:
  std::vector<Trade> limitOrder(Order order);

  std::vector<Trade> marketOrder(std::int64_t id, Side side, Quantity quantity);

  [[nodiscard]] bool cancel(std::int64_t id);

  std::optional<Price> bestAsk() const;

  std::optional<Price> bestBid() const;

  Quantity totalQuantity(Side side, Price price) const;

  bool hasId(std::int64_t id) const;

private:
  std::map<Price, std::list<Order>> m_asks{};
  std::map<Price, std::list<Order>, std::greater<Price>> m_bids{};
  std::unordered_map<std::int64_t, std::list<Order>::iterator> m_orders{};

  std::vector<Trade> match(std::int64_t id, Side side, Quantity &quantity,
                           std::optional<Price> limit);
};
} // namespace orderbook

#endif