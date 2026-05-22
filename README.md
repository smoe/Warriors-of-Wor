<div style="text-align: center;">
<h1 style="color: red; background-color: black;">
    <img src="https://raw.githubusercontent.com/vranki/Warriors-of-Wor/master/wow-sprites/spr2E9E.bmp" alt="Player" style="vertical-align: middle;">
    BOMBERWARRIORS OF WOR
    <img src="https://raw.githubusercontent.com/vranki/Warriors-of-Wor/master/wow-sprites/spr221E.bmp" alt="Monster" style="vertical-align: middle;">
</h1>
by Ville Ranki <ville.ranki@iki.fi>
</div>
<p></p>
BWOW is a remake and enhanced version of Commodore 64's 
Wizard of Wor game. It supports 3 game modes and up to 8 
players using Wiimotes.

It has been very successfull as a party game, although
still contains a few bugs. Consider it version 0.9 beta.

## Game modes

Original (co-op):
 - Fight different monsters in the dungeon.

Deathmatch:
 - Kill other players. After a few minutes walls start to disappear to 
   speed up the fight. 
 - Winner of each match gets one "Coin". The first player getting five 
   coins wins.

Bomberman:
 - Much like deathmatch, but with bombs instead of lazorguns.

## Adding players

Pairing wiimotes:

 - Start game
 - In player selection screen press "1" and "2" buttons on a wiimote simultaneously. 
 - Wiimote LEDs flash for a while and if the mote is connected leds light up 
   displaying player number (look at the screen). 
 - Repeat for all players. Press "+" to continue when all players have joined. 
 - Use wiimote in horizontal position. Only d-pad and button "1" are used in game.

You can press "K" on keyboard to add a single keyboard player, controlled by
arrow keys and space to fire. Yes, a second keyboard player is needed.

## Build requirements

Required:

 - A C++17 compiler.
 - Qt 6 development files, including Qt Core, Gui, Widgets, qmake, and moc.

Optional:

 - SDL2 and SDL2_mixer for sound. If SDL2_mixer is not installed, the game
   still builds and runs without audio.
 - libcwiid for Wiimote support. This is mainly available on Linux. If cwiid is
   not installed, the game still builds and runs with keyboard controls only.

On macOS with Homebrew:

```
brew install qt
brew install sdl2_mixer   # optional audio support
export PATH="$(brew --prefix qt)/bin:$PATH"
```

On Debian or Ubuntu:

```
sudo apt install build-essential qt6-base-dev qt6-base-dev-tools
sudo apt install libsdl2-dev libsdl2-mixer-dev   # optional audio support
sudo apt install libcwiid-dev                    # optional Wiimote support, if available
```

Some distributions install Qt 6's qmake as `qmake6`, while others provide it as
`qmake`.

## Building

A separate build directory keeps generated files out of the source files:

```
mkdir -p build
cd build
qmake6 ../wow.pro
make
```

If your system only has `qmake`, use this instead:

```
qmake ../wow.pro
make
```

To force a build without optional audio or Wiimote support:

```
qmake6 ../wow.pro CONFIG+=no_sdl CONFIG+=no_cwiid
make
```

The qmake project also disables an optional feature automatically when its
development package is missing.

## Installing

There is no packaged installer or `make install` target yet. The game expects
its data files to be available through relative paths at runtime:

 - `Adore64.ttf`
 - `maps_wow.txt`
 - `maps_bomberman.txt`
 - `wow-sprites/`
 - `samples/` when audio support is enabled

For a local install, copy the built executable or app bundle together with
those files and directories, then start the executable from that install
directory. On macOS, the generated `wow.app` bundle is not self-contained yet,
so double-clicking it from Finder may not find the game data.

## Running

From the source checkout after building:

```
./build/wow
```

On macOS, qmake builds an app bundle:

```
./build/wow.app/Contents/MacOS/wow
```

Run these commands from the repository root so the relative paths to maps,
sprites, sounds, and the font resolve correctly.

Useful keyboard controls:

 - `K` adds a keyboard player on the player selection screen.
 - `A` adds a cooperative automated player on the player selection screen.
 - Arrow keys or `H`, `J`, `K`, `L` move the first keyboard player.
 - Space fires for the first keyboard player.
 - `W`, `A`, `S`, `D` move the second keyboard player, and Tab fires.
 - `S` confirms menu selections.
 - `R` resets the player selection.
 - `F` toggles fullscreen.
 - `Q` quits.

## Portability

The game was originally written on Ubuntu Linux and now builds with Qt 6 on
macOS too. SDL2_mixer audio and cwiid Wiimote support are optional at build
time; cwiid is Linux-specific, so macOS builds normally run without Wiimotes.

Patches to build the game on other platforms are welcome.

## Known issues & todo

 - Further Qt6 and other modernizations
 - Performance is not as good as it should, probably because
   sprites are not pre-scaled to game resolution. Needs some
   benchmarking.
 - Original game mode still has some bugs. Deathmatch should work
   pretty well.
 - The Worluk death sound is missing. The code expects
   `samples/worluk_died.wav`, but the original effect has not been recovered
   yet and may need a new C64-style replacement.
 - More controller options needed (SDL joystick).
 - CRT rendering - see http://filthypants.blogspot.fi/2011/05/more-emulator-pixel-shaders-crt-updated.html

Patches are welcome. Use GitHub's merge requests if possible!

## See also

Several other implementations of WoW are presented on GitHub that you may want investigate:
 - https://github.com/dabadab/wizardofwor
   The reverse engineering one, also offering a series of technical insights.
   BWOW was developed completely independly, though.
 - https://github.com/Sanguinik/KnightOfWor/
   A Java implementation.

## Credits

Wizard of Wor arcade game by Midway Wizard of Wor C64 version by Commodore Bomberman by Hudson Soft

I consider WOW as abandonware. If you own the copyright 
for this game and want this free open source version 
to be removed, contact me but be prepared for negative 
PR in press.

Adore64 font from:
http://www.dafont.com/adore64.font

Free for personal use.
