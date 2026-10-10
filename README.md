# l1nfo

A tool for L1 CPU cache charactersitcs inference.

## Task

Написать программу, которая вычисляет и печатает характеристики кэша данных L1:

- Объем
- Длина линейки
- Ассоциативность

Рекомендуемый язык: C++

- Функция для чтения таймера высокого разрешения `std::chrono::steady_clock::now()`

Подсказки:

- Паттерн обращения к памяти проще всего задать при помощи хранимого в массиве индекса или адреса следующего обращения к массиву

- Массив лучше выровнять на границу страницы

- Google "Robust Method to Determine Cache and TLB Characteristics"

## Build & Run

```bash
cmake -B build && cmake --build build && ./build/l1nfo
```

## Declared Characteristics

```bash
sysctl -n hw.l1dcachesize
# 65536

sysctl -n hw.cachelinesize
# 128
```

## Measured Characteristics

TODO
