#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <numeric>
#include <random>
#include <ratio>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t KiB = 1024;

struct Cell final {
  Cell* next = nullptr;
};

class Cycle final {
  static constexpr std::size_t Seed = 1;

public:
  Cycle(std::size_t stride, std::size_t size) {
    // NOLINTNEXTLINE(cert-msc51-cpp, cert-msc32-c)
    std::default_random_engine random(Seed);

    std::vector<std::size_t> index(size);
    std::ranges::iota(index, 0);
    std::ranges::shuffle(index, random);

    area_.resize(AreaSize(stride, size));

    for (std::size_t i = 0; i < size; ++i) {
      Cell& prev = area_[index[i] * stride];
      Cell& next = area_[index[(i + 1) % size] * stride];
      prev.next = &next;
    }

    begin_ = &area_[index.front() * stride];
  }

  [[nodiscard]] const Cell* Begin() const {
    return begin_;
  }

private:
  static std::size_t AreaSize(std::size_t stride, std::size_t size) {
    assert(2 <= size);
    assert(0 < stride);
    return ((size - 1) * stride) + 1;
  }

  std::vector<Cell> area_;
  Cell* begin_ = nullptr;
};

const Cell* Next(const Cell* cell, std::size_t n = 1) {
  for (std::size_t i = 0; i < n; ++i) {
    cell = cell->next;
  }
  return cell;
}

std::chrono::duration<double, std::nano> DurationOf(const auto& action) {
  using Clock = std::chrono::steady_clock;

  const auto begin = Clock::now();
  action();
  const auto end = Clock::now();

  return end - begin;
}

auto Median(auto samples) {
  assert(!samples.empty());
  std::ranges::sort(samples);
  return samples[samples.size() / 2];
}

double ReadLatency(std::size_t stride, std::size_t size) {
  constexpr std::size_t Samples = 8;
  constexpr std::size_t WarmupOps = 1'000;
  constexpr std::size_t TimedOps = 1'000'000;

  static const Cell* volatile hole = nullptr;

  const Cycle cycle(stride, size);
  const Cell* cell = Next(cycle.Begin(), size * WarmupOps);

  std::vector<double> samples;
  samples.reserve(Samples);

  for (std::size_t i = 0; i < Samples; ++i) {
    const auto duration = DurationOf([&] { cell = Next(cell, TimedOps); });
    samples.push_back(duration.count() / TimedOps);
  }

  hole = cell;
  return Median(samples);
}

std::size_t RefineLastFastSize(std::size_t low, std::size_t high, const auto& is_slow) {
  assert(low < high);
  while (high - low > 1) {
    const std::size_t middle = low + ((high - low) / 2);
    if (is_slow(middle)) {
      high = middle;
    } else {
      low = middle;
    }
  }
  return low;
}

std::size_t LastFastSize(std::size_t stride) {
  constexpr double SlowdownFactor = 1.2;
  constexpr std::size_t SizeBegin = 2;
  constexpr std::size_t SizeEnd = 32768;

  const double hit_time = ReadLatency(stride, SizeBegin);
  const auto is_slow = [&](std::size_t size) {
    const auto latency = ReadLatency(stride, size);
    return hit_time * SlowdownFactor < latency;
  };

  std::size_t low = SizeBegin;
  for (std::size_t size = SizeBegin * 2; size <= SizeEnd; size *= 2) {
    if (is_slow(size)) {
      return RefineLastFastSize(low, size, is_slow);
    }

    low = size;
  }

  throw std::runtime_error("slow read latency found not found");
}

struct Plateau final {
  std::size_t stride = 0;
  std::size_t size = 0;
};

Plateau FindCachePlateau() {
  constexpr std::size_t StrideBegin = 16 / sizeof(Cell);
  constexpr std::size_t StrideEnd = (std::size_t{256} * KiB) / sizeof(Cell);

  std::vector<Plateau> plateaus;
  for (std::size_t stride = StrideBegin; stride <= StrideEnd; stride *= 2) {
    plateaus.push_back({.stride = stride, .size = LastFastSize(stride)});
  }

  for (std::size_t i = 1; i + 2 < plateaus.size(); ++i) {
    const auto& a = plateaus[i - 1];  // NOLINT(readability-identifier-length))
    const auto& b = plateaus[i + 0];  // NOLINT(readability-identifier-length))
    const auto& c = plateaus[i + 1];  // NOLINT(readability-identifier-length))
    const auto& d = plateaus[i + 2];  // NOLINT(readability-identifier-length))

    const bool is_trusted =
        ((a.stride * 2 == b.stride) && (b.stride * 2 == c.stride) && (c.stride * 2 == d.stride));
    const bool is_stable = ((a.size > b.size) && (b.size == c.size) && (c.size == d.size));

    if (is_trusted && is_stable) {
      return b;
    }
  }

  throw std::runtime_error("plateau not found");
}

const Plateau& CachePlateau() {
  static const Plateau Plateau = FindCachePlateau();
  return Plateau;
}

std::size_t CacheCapacity() {
  const auto& plateau = CachePlateau();
  return plateau.stride * plateau.size * sizeof(Cell);
}

std::size_t CacheLineSize() {
  constexpr std::size_t LineBegin = 32;
  constexpr std::size_t LineEnd = 1024;
  constexpr double DecreaseFactor = 0.80;
  constexpr double IncreaseFactor = 1.20;

  bool is_decreased = false;
  std::size_t candidate = 0;
  for (std::size_t bytes = LineBegin; bytes <= LineEnd; bytes *= 2) {
    const auto pure = static_cast<double>(LastFastSize(bytes / sizeof(Cell)));
    const auto mixed = static_cast<double>(LastFastSize((bytes + (bytes / 2)) / sizeof(Cell)));

    if (mixed < pure * DecreaseFactor) {
      is_decreased = true;
      candidate = 0;
    } else if (is_decreased && candidate == 0) {
      candidate = bytes;
    } else if (candidate != 0 && bytes == candidate * 2 && mixed > pure * IncreaseFactor) {
      return candidate;
    }
  }

  throw std::runtime_error("candidate not found");
}

std::size_t CacheAssociativity() {
  return CachePlateau().size;
}

}  // namespace

int main() try {
  auto& out = std::cout;

  const auto capacity = CacheCapacity();
  const auto line_size = CacheLineSize();
  const auto associativity = CacheAssociativity();

  out << "l1nfo\n";
  out << "  Capacity is " << capacity / KiB << " KiB\n";
  out << "  Line size is " << line_size << " B\n";
  out << "  Associativity is " << associativity << "\n";

  return 0;
} catch (const std::exception& error) {
  auto& err = std::cerr;

  err << "l1nfo: " << error.what() << '\n';

  return 1;
}
