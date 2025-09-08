# Nakshatra

An XBoard protocol compatible chess and [antichess](https://en.wikipedia.org/wiki/Losing_Chess) variant engine created for fun.

## Play Online

To play a game of chess or antichess against Nakshatra, check out [Nakshatra3](https://lichess.org/@/Nakshatra3) @ lichess where it *may* be online.

## Build and Run from Source (GNU/Linux, x86-64)

### Pre-requisites

Example for Ubuntu and other Debian based distros:

```
sudo apt install zip git cmake ninja-build g++  # or clang++, need C++20+ support
```

### Build from Source

```
git clone https://github.com/goutham/nakshatra.git
cd nakshatra
./install.sh
```

The engine executable file `nakshatra` will be generated under the `build/` directory if installation succeeds.

### Play Locally

Install one of the XBoard protocol compatible interfaces such as [cutechess](https://github.com/cutechess/cutechess), and configure it to run the engine executable file `nakshatra` as the computer player. Using an opening book (not included) is recommended for variations in gameplay.

## History

Originally developed in 2009 / 2010 as a [suicide-chess](https://www.freechess.org/Help/HelpFiles/suicide_chess.html) engine expressly for playing on [FICS](http://www.freechess.org), where a vibrant community of suicide-chess enthusiasts and engines hung out back in those days. Nakshatra was active on FICS for quite a few years under the handle [nakshatra](http://ficsgames.org/cgi-bin/search.cgi?player=nakshatra&action=Finger). [This](http://nakshatrachess.blogspot.com) old, long abandoned, [blog](http://nakshatrachess.blogspot.com) also records a bit of history.

## Credits

* [Chessprogramming wiki](https://www.chessprogramming.org/) served as a great guide for building most parts of the program, especially as I bolted on support for standard chess.
* Various papers on [proof number search](https://en.wikipedia.org/wiki/Proof-number_search) and its variants by Victor Allis and others.
* [Nilatac](https://github.com/CatalinFrancu/nilatac) (or a previous version of it from 2009-ish) that served as my initial inspiration to get started.
* [Talkchess](https://talkchess.com/) community.
