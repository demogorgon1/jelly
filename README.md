# jelly
[![CMake](https://github.com/demogorgon1/jelly/actions/workflows/cmake.yml/badge.svg)](https://github.com/demogorgon1/jelly/actions/workflows/cmake.yml)

___WARNING: This is very much work in progress. Here be dragons.___

_Jelly_ is a database system designed to function as blob-storage for online games. It's a C++ library that can either be embedded directly
in a game server or as the inner-most moving parts of a larger distributed database system.

* [Visit the wiki for discussion about the inner workings of _jelly_](https://github.com/demogorgon1/jelly/wiki)
* [Doxygen-generated documentation](https://demogorgon1.github.io/jelly/annotated.html)
* [Examples](https://demogorgon1.github.io/jelly/examples.html)

## In a nutshell

* Efficient and cheap to host for small-scale online games, but scales reasonably well into larger systems.
* Multiple options regarding how to design a network topography that match the requirements of your particular project.
* Optimized for online games where high throughput and write performance are favored over low latency and read performance.
* Persistent blob-locking mechanism to prevent game servers fighting over data. 
* Implemented as a Log Structured database engine.

## Limitations and things you should know

* Your blobs should be small (preferably less than 10 KB). Don't put GUIDs or strings in your blobs and you'll be fine.
* All metadata (not the blobs themselves) is expected to fit in memory. You need to have a _lot_ of
users before this becomes a problem, though. This makes _jelly_ very simple.
* _Jelly_ runs a pass on all data on disk (both stores and write-ahead logs) on startup. This means it can take some time if restart is required, but it's really not that bad even with a lot of users. Game servers will generate a lot of data,
but the vast majority of it will be overwrites. And your blobs are small, right?
* This is mainly a low-level storage engine. No networking comes in the box, you'll have to glue everything together yourself.
* This is largely an expirement and I'm yet to run proper comparative benchmarks against alternative ways
to store data. 
