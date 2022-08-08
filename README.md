Nakshatra
=========

An XBoard protocol compatible chess and [antichess](https://en.wikipedia.org/wiki/Losing_Chess) variant engine.

# Play Online

To play a game of chess or antichess with Nakshatra, check out these sites where it *may* be online:

* [Nakshatra3](https://lichess.org/@/Nakshatra3) @ [lichess.org](https://lichess.org/)
* [nakshatra](http://ficsgames.org/cgi-bin/search.cgi?player=nakshatra&action=Finger) @ [FICS](https://www.freechess.org/)

# Build and Run from Source (GNU/Linux)

## Pre-requisites

```
# Example for Ubuntu and other Debian based GNU/Linux systems.
sudo apt-get install zip
sudo apt-get install git
sudo apt-get install g++  # or clang++, need C++17+ support
sudo apt-get install scons
```

## Build from Source

```
git clone https://github.com/goutham/nakshatra.git
cd nakshatra/src
./build.sh
```

See comments in `build.sh` for additional build options. The engine executable file, named `nakshatra`, will be generated under `src/` directory upon successful execution of the script.

## Play Locally

Install one of the XBoard protocol compatible interfaces such as [cutechess](https://github.com/cutechess/cutechess), and configure it to run the engine executable file `nakshatra` as the computer player. The executable may be moved to a different directory but it depends on `src/egtb/*` and (may be) other files in your local git repo where `build.sh` was run, so do not delete the repo. Using an opening book (not included) is recommended for variations in gameplay.

# History

Originally developed in 2009 / 2010 as a [suicide-chess](https://www.freechess.org/Help/HelpFiles/suicide_chess.html) engine expressly for playing on [FICS](http://www.freechess.org), where a vibrant community of suicide-chess enthusiasts and engines hung out (sadly, no longer the case). Nakshatra continues, as of this writing, to play there under the handle [nakshatra](http://ficsgames.org/cgi-bin/search.cgi?player=nakshatra&action=Finger). For the over-curious, [this](http://nakshatrachess.blogspot.com) old, long abandoned, [blog](http://nakshatrachess.blogspot.com) also records some historical context.
