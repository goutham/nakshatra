Nakshatra - a variant chess engine
==================================

A variant chess engine that I originally developed to play [antichess](https://en.wikipedia.org/wiki/Losing_Chess) (a.k.a. suicide chess) on [Free Internet Chess Server (FICS)](http://www.freechess.org).

Most of the core code was written around 2009/2010. Back then, plenty of strong antichess playing humans and bots hung out on FICS and it was fun to develop and pitch my own engine against them. For a while, I even (sparingly) [blogged](http://nakshatrachess.blogspot.com) its progress. After 2010, development activity has been sporadic at best; ever so rarely I try something new to scratch an itch or two.

Nakshatra is a pretty strong antichess player. It can also play standard chess but I've expended very little effort towards increasing its standard chess playing strength; there are still lots of low hanging fruits. One of my initial goals was to support many [chess variants](https://en.wikipedia.org/wiki/List_of_chess_variants) so I believe the code is well organized to make that (hopefully) easy. However, I never actually got around to adding any other variants ([atomic chess](https://en.wikipedia.org/wiki/Atomic_chess) was (is?) next on my list).

To play a game of antichess against Nakshatra, check out these sites where it *may* be online (you'll need an account):

* lichess: https://lichess.org/@/Nakshatra3 -- often online (as of Dec. 2020); this is a weakened version running on a low cost cloud VM with minimal resources.
* lichess: https://lichess.org/@/Nakshatra7 -- sometimes online.
* [FICS](freechess.org) handle: [nakshatra](http://ficsgames.org/cgi-bin/search.cgi?player=nakshatra&action=Finger) -- sometimes online.

You can also clone the repository, build and run it locally (tested on a fresh linux VM):

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
[XBoard](https://www.gnu.org/software/xboard/) GUI and use the helper script `play.sh` to invoke the engine with XBoard.
