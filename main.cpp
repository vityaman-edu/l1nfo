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
