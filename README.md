# jelly
[![CMake](https://github.com/demogorgon1/jelly/actions/workflows/cmake.yml/badge.svg)](https://github.com/demogorgon1/jelly/actions/workflows/cmake.yml)

_Jelly_ is a database system designed to function as blob-storage for online games. It's a C++ library that can either be embedded directly
in a game server or as the inner-most moving parts of a larger distributed database system.

* [Documentation and C++ examples](https://demogorgon1.github.io/jelly/index.html)
* [Comparative benchmarks (SQLite/RocksDB)](https://github.com/demogorgon1/jelly-bench)

## In a nutshell

* Optimized for online games where high throughput and write performance are favored over low latency and read performance.
* Efficient and cheap to host for small-scale online games, but scales reasonably well into larger systems.
* Multiple options regarding how to design a network topography that match the requirements of your particular project.
* Persistent blob-locking mechanism to prevent game servers fighting over data. 
* Improve write performance using request priorities: some player progress data updates will be less important than others.
* Implemented as a Log Structured database engine.

## Usage

First of all you'll need git and cmake to acquire and build the project. Then you can run:

```
git clone https://github.com/demogorgon1/jelly.git
cd jelly
mkdir build
cd build
cmake ..
cmake --build .
```

If succesfull, this will build a static library, tests, and the examples. Run ```ctest``` immediately to verify that everything works as intended.

Optionally you can include _jelly_ directly in your cmake build system using FetchContent:

```
FetchContent_Declare(jelly
  GIT_REPOSITORY https://github.com/demogorgon1/jelly.git
)
FetchContent_MakeAvailable(jelly)
```

Use ```jelly::jelly``` to link your target with _jelly_.

If you're not using CMake, you can just add the ```jelly``` and ```include``` subdirectories to your project. 

See [here](https://demogorgon1.github.io/jelly/index.html) for documentation and examples.

## Limitations and things you should know

* Your blobs should be small (preferably less than 10 KB). Don't put GUIDs or strings in your blobs and you'll be fine.
* All metadata (not the blobs themselves) is expected to fit in memory. You need to have a _lot_ of
users before this becomes a problem, though. This makes _jelly_ very simple.
* _Jelly_ runs a pass on all data on disk (both stores and write-ahead logs) on startup. This means it can take some time if restart is required, but it's really not that bad even with a lot of users. Game servers will generate a lot of data,
but the vast majority of it will be overwrites. And your blobs are small, right?
* This is mainly a low-level storage engine. No networking comes in the box, you'll have to glue everything together yourself.
