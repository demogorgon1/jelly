# jelly
_Jelly_ is a simple database system designed to function as blob-storage for online games. It's a C++ library that can either be embedded 
in a game server, similarily to sqlite, or as the inner-most moving parts of a larger distributed database system.

* Efficient and cheap to host for small-scale online games, but scales reasonably well into large systems.
* Multiple options regarding how to design a network topography that match the requirements of your particular project.
* Robust handling of common failure scenarios to prevent loss of player progress.
* Optimized for online games where high throughput and write performance are favored over low latency and read performance.

## In a nutshell

A typical client-server online game works something like this:

1. Client connects and authenticates with a game server.
2. Game server creates a session for the client and loads player progress.
3. During the session player progress is saved periodically and/or when something important happens.
4. When the client terminates its session, the game server performs a final save of the player progress.

One obvious implication of this is that saves (writes) are much more frequent than loads (reads). There will
only ever be a single read per session, while there will most likely be a large number of writes (depending on the duration of the session). Because of this, writes need to be very efficient, while reads aren't as important.

Secondly it's important to consider what happens if clients need to be able to switch between multiple game server: what if two game server end up fighting over the same player progress? In other words, what happens when there are multiple sessions for the same player across multiple game servers. This can happen in various edge cases and failure modes. A common example could
be that a client crashes or disconnects during a session and then reconnects to another server. The original server session might not have saved progress yet at that point and the new session will get old data. Loss of progress makes players very unhappy and should be avoided.

To solve this problem the game server should acquire an exclusive lock on player progress data before it's loaded. This lock should last as long as the session and not be released before the latest player progress has been saved.
_Jelly_ implements such a locking mechanism in addition to writing and reading player progress.

## Background
