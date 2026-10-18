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
cmake --build build --config Release && cmake -B build && cmake --build build
```

## Declared Characteristics

### MacOS arm64

```bash
sysctl hw.cachelinesize hw.l1dcachesize hw.perflevel1.l1dcachesize hw.perflevel0.l1dcachesize 
# hw.cachelinesize: 128
# hw.l1dcachesize: 65536
# hw.perflevel1.l1dcachesize: 65536
# hw.perflevel0.l1dcachesize: 131072
```

### Linux x64_86

```bash
lscpu --cache
# NAME ONE-SIZE ... WAYS ... COHERENCY-SIZE
# L1d       32K        8                 64
# ...
```

## Measured Characteristics

### MacOS arm64

```bash
./build/l1nfo
# l1nfo
#   Capacity is 128 KiB
#   Line size is 64 B
#   Associativity is 8
# NB. Guess, this is because of the P-core.
```

### Linux x64_86

```bash
./build/l1nfo
# l1nfo
#   Capacity is 32 KiB
#   Line size is 64 B
#   Associativity is 8
```
