#include <ctime>
#include <iomanip>
#include <ios>
#include <orderbook/book.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

enum class OpType { limit = 0, cancel = 1, market = 2 };

struct Stats {
  int limits{};
  int cancels{};
  int markets{};
  int cancelSuccess{};
  std::vector<std::int64_t> times{};
};

struct Op {
  OpType type{};
  std::int64_t id{};
  orderbook::Side side{};
  orderbook::Price price;
  orderbook::Quantity quantity;
  std::int64_t targetId{};
};

int main() {
  constexpr int seed{42};
  constexpr int nOrders{1'000'000};
  constexpr int nWarmup{10'000};
  constexpr bool timeIndividial{true};

  std::mt19937_64 rng{seed};

  std::normal_distribution<> priceDist{10000, 20};
  std::uniform_int_distribution<> quantityDist{1, 100};
  std::discrete_distribution<> opDist{60, 30, 10};
  std::bernoulli_distribution sideDist{0.5};
  std::uniform_real_distribution<> cancelDist{0, 1};

  std::vector<Op> ops{};
  ops.reserve(nOrders);
  std::vector<std::int64_t> limitOrders{};
  limitOrders.reserve(static_cast<size_t>(nOrders * 0.6));

  using enum orderbook::Side;
  using enum OpType;

  for (std::size_t i{0}; i < nOrders; ++i) {
    OpType type{static_cast<OpType>(opDist(rng))};
    std::int64_t id{static_cast<std::int64_t>(i)};
    orderbook::Side side{sideDist(rng) ? buy : sell};
    orderbook::Price price{
        std::max(static_cast<int>(std::llround(priceDist(rng))), 1) +
        (side == buy ? 0 : 20)};
    orderbook::Quantity quantity{quantityDist(rng)};
    std::int64_t targetId{
        (type == cancel && !limitOrders.empty())
            ? limitOrders[std::clamp(
                  static_cast<std::size_t>(limitOrders.size() -
                                           cancelDist(rng) * 1000),
                  static_cast<std::size_t>(0), limitOrders.size() - 1)]
            : -1};

    if (type == limit)
      limitOrders.push_back(i);

    ops.push_back(Op{type, id, side, price, quantity, targetId});
  }

  const auto testBook = [](orderbook::Book &book, const Op &o, Stats &s) {
    switch (o.type) {
    case limit:
      ++s.limits;
      book.limitOrder(orderbook::Order{o.id, o.side, o.price, o.quantity});
      break;
    case cancel:
      ++s.cancels;
      if (book.cancel(o.targetId))
        ++s.cancelSuccess;
      break;
    case market:
      ++s.markets;
      book.marketOrder(o.id, o.side, o.quantity);
      break;
    }
  };

  orderbook::Book warmupBook{};
  Stats warmupStats{};

  for (std::size_t i{0}; i < nWarmup; ++i) {
    testBook(warmupBook, ops[i], warmupStats);
  }

  orderbook::Book book{};
  Stats stats{};
  if (timeIndividial)
    stats.times.reserve(nOrders);

  auto start = Clock::now();
  auto previous = Clock::now();

  for (std::size_t i{0}; i < nOrders; ++i) {
    testBook(book, ops[i], stats);
    if (timeIndividial) {
      auto now = Clock::now();
      stats.times.push_back(
          std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous)
              .count());
      previous = now;
    }
  }

  auto end = Clock::now();

  std::ranges::sort(stats.times);

  const auto getPercentileIndex = [](double p) {
    return static_cast<std::size_t>(
        std::clamp(static_cast<int>(nOrders * p), 0, nOrders - 1));
  };
  auto totalTime{
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
          .count()};

  std::streamsize p{std::cout.precision()};

  std::cout << "Limits: " << stats.limits << "\nCancels: " << stats.cancels
            << "\nMarkets: " << stats.markets
            << "\nCancel Successes: " << stats.cancelSuccess << '\n';

  std::cout << "Total time: " << totalTime << "\nTime per order: " << std::fixed
            << std::setprecision(1) << static_cast<double>(totalTime) / nOrders
            << std::defaultfloat << std::setprecision(p) << '\n';
  if (timeIndividial)
    std::cout << "Median: " << stats.times[getPercentileIndex(0.5)]
              << "\np99: " << stats.times[getPercentileIndex(0.99)]
              << "\np99.9: " << stats.times[getPercentileIndex(0.999)] << '\n';

  return 0;
}