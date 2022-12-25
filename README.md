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

The relational database equivalent would be having table with these columns:

* _Key_ identifying a blob (for example a user account identifier).
* _Binary blob_ of serialized player progress.
* _Lock_ identifying the game server currently holding the exclusive lock.
* (and various metadata).

## Background
I've been working on an online RPG solo-project for a while now and _jelly_ came into existence because I wanted a cheap and efficient way to store player progress data. Obviously one can argue it's silly to implement your own database system, when your actual goal is to make a game... and sure it is. I just can't help myself. Originally I just wanted something extremely simple with a lot of assumptions, but ended up with something quite general purpose. 

I'm under no illusion that _jelly_ is comparable to any of the well-known, mature, and robust database systems people usually use. What it does is that it offers a simple solution to the narrowly scoped problem of storing player progress data for an online game. Nothing more, nothing less. This allows _jelly_ to be small and quite efficient. This was important to me as it's for a solo-project with a very limited budget for hosting servers.

Notice that _jelly_ doesn't offer any networking. If you want to use it as mainly intended, in a larger distributed system, you'll need to provide all the glue sticking everything together yourself.
