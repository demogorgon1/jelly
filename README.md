# jelly
_Jelly_ is a simple database system designed to function as blob-storage for online games. It's a C++ library that can either be embedded 
in a game server, similarily to sqlite, or as the inner-most moving parts of a larger distributed database system.

* Efficient and cheap to host for small-scale online games, but scales reasonably well into large systems.
* Multiple options regarding how to design a network topography that match the requirements of your particular project.
* Robust handling of common failure scenarios to prevent loss of player progress.

