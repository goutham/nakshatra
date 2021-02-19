Nakshatra
=========

A chess engine that I originally developed to play [antichess](https://en.wikipedia.org/wiki/Losing_Chess) (a.k.a. suicide chess) variant on [Free Internet Chess Server (FICS)](http://www.freechess.org). Back around 2009/2010, many strong antichess playing humans and bots hung out on [FICS](http://www.freechess.org) and it was fun to develop and pitch my own engine against them. For a while, I even (sparingly) [blogged](http://nakshatrachess.blogspot.com) its progress. Although I don't work on the codebase regularly anymore, every now and then I do try something new to scratch an itch or two.

To play a game of chess or antichess against Nakshatra, check out these sites where it *may* be online:

* [Nakshatra3](https://lichess.org/@/Nakshatra3) @ [lichess.org](https://lichess.org/)
* [nakshatra](http://ficsgames.org/cgi-bin/search.cgi?player=nakshatra&action=Finger) @ [FICS](https://www.freechess.org/)

You can also clone the repository, build and run it locally:

Pre-requisites:

```
sudo apt-get install git
sudo apt-get install scons
sudo apt-get install g++  # need C++17+
sudo apt-get install unzip
```

Clone the repo, build and run tests:

```
git clone https://github.com/goutham/nakshatra.git
cd nakshatra/src
./configure.sh
```

If all tests succeed, you've probably built it successfully. Now you can invoke the engine directly (`./nakshatra`) and communicate with it using [XBoard communication protocol](https://www.gnu.org/software/xboard/engine-intf.html) (which can be a bit awkward), or install
[XBoard](https://www.gnu.org/software/xboard/) GUI and use the helper script `play.sh` to invoke the engine with XBoard (be sure to set the correct variant). The engine is currently not [UCI protocol](https://www.chessprogramming.org/UCI) compatible so UCI-only interfaces won't work.
