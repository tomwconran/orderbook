#include <algorithm>
#include <optional>
#include <orderbook/book.hpp>
#include <stdexcept>
#include <vector>

namespace orderbook {

std::vector<Trade> Book::limitOrder(Order order) {
  if (m_orders.contains(order.id()))
    throw std::invalid_argument("Book.limitOrder: id already in m_orders");
  Quantity remaining{order.quantity()};
  auto trades{match(order.id(), order.side(), remaining, order.price())};
  const auto rest = [this, &order](auto &ownSide) {
    auto &level{ownSide[order.price()]};
    m_orders[order.id()] = level.insert(level.end(), order);
  };
  if (remaining.value() != 0) {
    order = Order{order.id(), order.side(), order.price(), remaining};
    if (order.side() == Side::buy)
      rest(m_bids);
    else
      rest(m_asks);
  }
  return trades;
}

std::vector<Trade> Book::marketOrder(std::int64_t id, Side side,
                                     Quantity quantity) {
  if (m_orders.contains(id))
    throw std::invalid_argument("Book.marketOrder: id already in m_orders");
  if (quantity.value() == 0)
    throw std::invalid_argument("Book.marketOrder: quantity cannot be 0");
  return match(id, side, quantity, std::nullopt);
}

bool Book::cancel(std::int64_t id) {
  const auto iter{m_orders.find(id)};
  if (iter == m_orders.end())
    return false;

  const auto cancelSide = [iter](auto &ownSide) {
    const Price price = iter->second->price();
    auto level{ownSide.find(price)};
    if (level == ownSide.end())
      throw std::logic_error("Book.cancel: level not found");
    level->second.erase(iter->second);
    if (level->second.empty())
      ownSide.erase(level);
  };

  const Side side = iter->second->side();
  if (side == Side::buy)
    cancelSide(m_bids);
  else
    cancelSide(m_asks);

  m_orders.erase(iter);
  return true;
}

std::optional<Price> Book::bestAsk() const {
  if (m_asks.empty())
    return std::nullopt;
  return m_asks.begin()->first;
}

std::optional<Price> Book::bestBid() const {
  if (m_bids.empty())
    return std::nullopt;
  return m_bids.begin()->first;
}

Quantity Book::totalQuantity(Side side, Price price) const {
  Quantity total{0};

  const auto sumQuantity = [&total](const Order &o) { total += o.quantity(); };
  const auto sumSide = [price, &sumQuantity](auto &ownSide) {
    const auto iter{ownSide.find(price)};
    if (iter != ownSide.end()) {
      std::ranges::for_each(iter->second, sumQuantity);
    }
  };
  if (side == Side::buy)
    sumSide(m_bids);
  else
    sumSide(m_asks);
  return total;
}

bool Book::hasId(std::int64_t id) const { return m_orders.contains(id); }

std::vector<Trade> Book::match(std::int64_t id, Side side, Quantity &quantity,
                               std::optional<Price> limit) {
  std::vector<Trade> trades;
  const auto matchLoop = [&](auto &opposite) {
    while (quantity.value() > 0 && !opposite.empty()) {
      auto level{opposite.begin()};
      if (limit && opposite.key_comp()(*limit, level->first))
        return;
      auto &resting{level->second.front()};
      Quantity filled{resting.fill(quantity)};
      quantity -= filled;
      trades.push_back(Trade{(side == Side::buy ? id : resting.id()),
                             (side == Side::buy ? resting.id() : id),
                             level->first, filled});
      if (resting.isFilled()) {
        m_orders.erase(resting.id());
        level->second.pop_front();
      }
      if (level->second.empty())
        opposite.erase(level);
    }
  };

  if (side == Side::buy)
    matchLoop(m_asks);
  else
    matchLoop(m_bids);
  return trades;
}
} // namespace orderbook