
                                M A M E

                    Multiple Arcade Machine Emulator

                  by Nicola Salmoria (MC6489@mclink.it)

please note that many people helped with this project, either directly or
by making source code available which I examined to write the drivers. I am
not trying to appropriate merit which isn't mine. See the acknowledgemnts
section for a list of contributors.


Here is a quick list of ther currently supported games; read on for details.
The list doesn't include variants of the same game.


Game                         Playable?   Accurate colors?    Sound?

Pac Man                        Yes            Yes             Yes
Ms Pac Man (bootleg)           Yes            Yes             Yes
Crush Roller                   Yes            Yes             Yes
Pengo                          Yes            Yes             Yes
Lady Bug                       Yes            Yes           No noise
Mr. Do!                        Yes            Yes             Yes
Crazy Climber                  Yes            Yes             Yes
Crazy Kong                     Yes             No             Yes
Donkey Kong                    Yes             No              No
Donkey Kong Jr.                Yes             No              No
Bagman                    Yes (slowdowns)      No          Music only
Wizard of Wor               Partially        Maybe             No
The Adventures of Robby Roto    No             No              No
Gorf                            No             No              No
Galaxian                       Yes            Yes           Limited
Pisces                         Yes            Yes           Limited
"Japanese Irem game"           Yes            Yes           Limited
War of the Bugs                Yes             No           Limited
Moon Cresta                    Yes            Yes?          Limited
Moon Quasar                    Yes            Yes?          Limited
The End                        Yes             No              No
Scramble                       Yes            Yes              No
Super Cobra                    Yes             No              No
Frogger                        Yes           Close             No
Amidar                         Yes           Close             No
Turtles                        Yes             No              No
Rally X                   Yes (slowdowns)      No              No
Pooyan                         Yes             No              No
Phoenix                        Yes           Close             No
Pleiades                       Yes             No              No
Space Invaders                 Yes            Yes              No
Carnival                        No             No              No
Mario Bros.                    Yes             No              No
Zaxxon                         Yes             No              No
Congo Bongo                    Yes             No              No
Bomb Jack                      Yes            Yes              No
Centipede                      Yes             No              No
Nibbler                        Yes             No              No



Acknowledgements
----------------

First of all, thanks to Allard van der Bas (avdbas@wi.leidenuniv.nl) for
starting the Arcade Emulation Programming Repository at
http://valhalla.ph.tn.tudelft.nl/emul8
Without the Repository, I would never have even tried to write an emulator.

If you find out something useful, submit it to avdbas@wi.leidenuniv.nl,
so it will be made available to everybody on the Repository page.

Z80Em Portable Zilog Z80 Emulator Copyright (C) Marcel de Kogel 1996,1997
   Note: the version used in MAME is slightly modified. You can find the
   original version at http://www.komkon.org/~dekogel/misc.html.
M6502 Emulator Copyright (C) Marat Fayzullin, Alex Krasivsky 1996
   Note: the version used in MAME is slightly modified. You can find the
   original version at http://freeflight.com/fms/.
Allegro library by Shawn Hargreaves, 1994/96
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c. Thanks to Chuck Cochems for the help in making them more
   compatible.
224x288 noscanlines video mode provided by Valerio Verrando
  (v.verrando@mclink.it)
AY-3-8910 emulation by Ville Hallik (ville@physic.ut.ee) and Michael Cuddy
  (mcuddy@FensEnde.com).
UNIX port by Allard van der Bas (avdbas@wi.leidenuniv.nl) and Dick de Ridder
  (dick@ph.tn.tudelft.nl).

Phoenix driver provided by Brad Oliver (bradman@primenet.com) and Mirko
   Buffoni (mix@lim.dsi.unimi.it)
Mario Bros., Zaxxon and Bomb Jack drivers provided by Mirko Buffoni
   (mix@lim.dsi.unimi.it)
Congo Bongo driver provided by Ville Laitinen (ville@sms.fi).

Very special thanks to Sergio Munoz for the precious information about the
   Pengo sound hardware and colors.
Thanks to Paul Swan for the information on the Lady Bug sound hardware and
   Mr.Do! colors.
Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention them all.
Thanks to Simon Walls (wallss@ecid.cig.mot.com) for the color information
   on many games.
Information about the Crazy Climber machine hardware (including palette)
   and ROM encryption scheme provided by Lionel Theunissen
   (lionelth@ozemail.com.au).
Thanks to Andy Milne (andy@canetics.com) for the information on the Crazy
   Climber sound roms.
Crazy Kong emulation set up by Ville Laitinen (ville@sms.fi).
Special thanks to Brad Thomas (bradt@nol.net) and Gary Shepherdson for the
   extensive information on Donkey Kong and Donkey Kong Jr.
Info on Bagman, Galaxian, Moon Cresta and many other games taken from Arcade
   Emulator by Robert Anschuetz.
Pooyan information provided by Michael Cuddy and Allard van der Bas
Thanks to Mirko Buffoni for the Amidar and Frogger colors.
Thanks to Richard Davies for info on Phoenix.
Thanks to Brad Thomas, Jakob Frendsen and Conny Melin for the info on Bomb
   Jack.
Thanks to Mike@Dissfulfils.co.uk for the information on the Moon Quasar
   encryption scheme.
Space Invaders information gathered from the Space Invaders Emulator by
   Michael Strutt (mstrutt@pixie.co.za)
Many thanks to Jim Hernandez for the information on Wizard of Wor hardware.
Thanks to Mike Coates (mike@dissfulfils.co.uk) for Carnival ROM placement
   indications.
Colors for Donkey Kong, Donkey Kong Jr. and Mario Bros. derived from Kong
   emulator by Gary Shepherdson.
Colors for Amidar and Frogger derived from SPARCADE by Dave Spicer.
Thanks to Brad Oliver, Marc Vergoossen (marc.vergoossen@pi.net) and Richard
   Davies (R.Davies@dcs.hull.ac.uk) for help with Donky Kong Jr. colors.
Thanks to Dave W. for all his help.



Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -nosound   will run Ms Pac Man without sound

options:
-noscanlines  use alternate video mode (not availble in all games). Use this
              if the default mode doesn't work with your monitor/video card.
-vesa         use standard 640x480x256 VESA mode instead of custom video mode.
              Use this as a last resort if -noscanlines doesn't solve your
			  video problems.
-soundcard n  select sound card (if this is not specified, you will be asked
              interactively)
-nojoy        don't poll joystick
-log          create a log of illegal memory accesses in ERROR.LOG
-frameskip n  skip frames to speed up the emulation. For example, if the game
              normally runs at 60 fps, "-skipframe 1" will make it run at 30
			  fps, and "-skipframe 2" at 20 fps. Use F11 to check the fps your
			  computer is actually displaying. If the game is too slow,
			  increase the frameskip value. Note that this setting can also
			  affect audio quality (some games sound better, others sound
			  worse).


The following keys work in all emulators:

3       Insert coin
1       Start 1 player game
2       Start 2 players game
Tab     Change dip switch settings
P       Pause
F3      Reset
F4      Show the game graphics. Use cursor keys to change set/color, F4 to exit.
F11     Toggle fps counter
F10     Toggle speed throttling
F12     Save a screen snapshot
ESC     Exit emulator



Pac Man ("pacman")
------------------

Arrows  Move around
F1      Skip level
F2      Test mode
CTRL    Speed up cheat

Clones supported:
  Pac Man modification ("pacmod")
  Namco Pac Man ("namcopac")
  Hangly Man ("hangly")
  Puck Man ("puckman")
  Piranha ("piranha")

Known issues:
- Blinky and Pinky seem to be shifted one pixel to the right. This is really
  annoying, but I can't seem to be able to understand why. Maybe there is an
  additional "sprite offset" register somewhere? Or did the original just
  behave this way?
  Note that we can't fix it by just moving sprites 0 and 1 one pixel to the
  left, because when Pac Man eats a power pill the sprites order is changed
  so that Pac Man is drawn over the ghosts. It becomes sprite 0, and Blinky
  becomes sprite 4.



Ms Pac Man ("mspacman")
-----------------------

Arrows  Move around
F1      Skip level
F2      Test mode
CTRL    Speed up cheat

Known issues:
- Blinky and Pinky seem to be shifted one pixel to the right. This is really
  annoying, but I can't seem to be able to understand why. Maybe there is an
  additional "sprite offset" register somewhere? Or did the original just
  behave this way?
  Note that we can't fix it by just moving sprites 0 and 1 one pixel to the
  left, because when Pac Man eats a power pill the sprites order is changed
  so that Pac Man is drawn over the ghosts. It becomes sprite 0, and Blinky
  becomes sprite 4.



Crush Roller ("crush")
----------------------

Crush Roller is a hacked version of Make Trax, modified to run on a
Pac Man board.

Arrows  Move around
F1      Skip level

Known issues:
- There's the same problem with sprites as in Pac Man, but here it could be
  fixed without apparent side effects.



Pengo ("pengo")
---------------

Arrows  Move around
CTRL    Push
F1      Skip level
F2      Test mode

Clones supported:
  Penta ("penta")



Lady Bug ("ladybug")
--------------------

Arrows  Move around
F1      Skip level

Known issues:
- The noise generator is not emulated yet.



Mr. Do! ("mrdo")
----------------

Arrows  Move around
CTRL    Fire
F1      Skip level
CTRL+F3 Test mode

Clones supported:
  Mr. Lo! ("mrlo")

Known issues:
- The noise generator is not emulated yet, but I think Mr. Do! doesn't
  use it anyway.



Crazy Climber ("cclimber")
--------------------------

E,S,D,F Left joystick
I,J,K,L Right joystick
F1      Skip level

Clones supported:
  Japanese version ("ccjap")
  bootleg version ("ccboot")



Crazy Kong ("ckong")
--------------------

This Donkey Kong clone runs on the same hardware as Crazy Climber, most
notable differencies being a larger character set and the display rotated
90 degrees.

Arrows  Move around
CTRL    Jump

Known issues:
- Some problems with sound



Donkey Kong ("dkong")
--------------------

Arrows  Move around
CTRL    Jump



Donkey Kong Jr. ("dkongjr")
---------------------------

Arrows  Move around
CTRL    Jump



Bagman ("bagman")
-----------------

Arrows  Move around
CTRL    Action

Known issues:
- Frequent slowdowns, I don't know what's causing them (the fps counter stays
  at 60, so this is not caused by slow emulation).



Wizard of Wor ("wow")
---------------------

Arrows  Move around
CTRL    Fire
F2      Test mode (keep it pressed)
The original machine had a special joystick which could be moved either
partially or fully in a direction. Pushing it slightly would turn around the
player without moking it move. The emulator assumes that you are always
pushing the joystick fully, to simulate the "half press" you can press Alt.


Known issues:
- This game is completely different from anything else I have emulated before.
  The video memory is bitmapped. There are no character generator ROMs,
  graphics data is contained in the code ROMs and the program sends commands
  to some custom circuitry on the board to do copies, fills, and so on.
  Understanding how the thing works without the schematics is tricky. There
  are no memory mapped ports, everything is done via IN and OUT instructions.
  As of now, it is somewhat playable but there are several faults.
- No background stars, no fade in/fade out.


The Adventures of Robby Roto ("robby")
--------------------------------------

This game runs on the same hardware as Wizard of Wor, but doesn't work yet. I
still haven to check the loading address of the ROMs.



Gorf ("gorf")
--------------------------------------

This game runs on the same hardware as Wizard of Wor, but doesn't work yet.
It boots, shows some text on the screen and that's all.



Galaxian ("galaxian")
---------------------

Arrows  Move around
CTRL    Fire
F2      Test mode

There are so many clones here that I'm not even sure which is the "original"
one. The dip switch menu might display wrong settings.
  Namco Galaxian ("galnamco")
  Super Galaxian ("superg")
  Galaxian Part X ("galapx")
  Galaxian Part 1 ("galap1")
  Galaxian Part 4 ("galap4")
  Galaxian Turbo ("galturbo")

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct



Pisces ("pisces")
-----------------

This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct
- What do the dip switches do?



"Japanese Irem game" ("japirem")
--------------------------------

This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Clones supported:
  Uniwars ("uniwars")

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct
- What does dip switch 6 do?



War of the Bugs ("warofbug")
----------------------------

This runs on the same hardware as Galaxian.

Arrows  Move around
CTRL    Fire

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct
- What do the dip switches do?



Moon Cresta ("mooncrst")
------------------------

This runs on a hardware very similar to Galaxian.
The ROMs are encrypted. Nichibutsu copyright.

Arrows  Move around
CTRL    Fire

Clones supported:
  Unencrypted version ("mooncrsb")

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct
- What do the dip switches do?



Moon Quasar ("moonqsr")
-----------------------

This runs on a modified Moon Cresta board.

Arrows  Move around
CTRL    Fire

Known issues:
- Only one sound channel is emulated, and I'm not sure it's correct



The End ("theend")
------------------

This runs on a hardware very similar to Galaxian.

Arrows  Move around
CTRL    Fire

Known issues:
- I haven't yet had time to check what the various input bits and dip switches
  do. Two players are not supported.
- Is this game supposed to have background stars?



Scramble ("scramble")
---------------------

The video hardware is very similar to Galaxian, main differences being that
bullets are not vertical lines and the star background doesn't scroll.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Clones supported:
  Battle of Atlantis ("atlantis") [I don't know what most of the dip switches
                                   do, and you get 9 credits per coin.]

Known issues:
- The background stars don't blink. Maybe they should also be clipped to the top
  and bottom of the screen?
- Two players mode doesn't work



Super Cobra ("scobra")
----------------------

Runs on the same hardware as Scramble.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Clones supported:
  bootleg version ("scobrab")

Known issues:
- The background stars don't blink. Maybe they should also be clipped to the
  top and bottom of the screen?
- Two players mode doesn't work



Frogger ("frogger")
-------------------

Arrows  Move around

Clones supported:
  bootleg version, which runs on a modified Scramble board ("froggers")



Amidar ("amidar")
-----------------

Arrows  Move around
CTRL    Jump

Clones supported:
  US version ("amidarus"). This version is quite different, it has a better
    attract mode and displayes the number of jumps left.

Known issues:
- What do the dip switches do?



Turtles ("turtles")
-------------------

This runs on the same hardware as Amidar

Arrows  Move around
CTRL    Bomb

Known issues:
- What do the dip switches do? I'm obviously missing something, becasue the
  game plays in unlimited lives mode.



Rally X ("rallyx")
------------------

Arrows  Move around
CTRL    Smoke
F2      Test

Known issues:
- Graphic glitches, slowdowns, small screen



Pooyan ("pooyan")
-----------------

Arrows  Move around
CTRL    Fire



Phoenix ("phoenix")
-------------------

Arrows  Move around
CTRL    Fire
ALT     Barrier



Pleiades ("pleiades")
---------------------

This runs on the same hardware as Phoenix.

Arrows  Move around
CTRL    Fire
ALT     Teleport



Space Invaders ("invaders")
---------------------------

Arrows  Move around
CTRL    Fire

Clones supported (some of these have wrong colors, and the dip switch menu
      doesn't work):
  Super Earth Invasion ("earthinv")
  Space Attack II ("spaceatt")
  Space Invaders Deluxe ("invdelux") (doesn't work yet)
  Galaxy Wars ("galxwars")
  Lunar Rescur ("lrescue")
  Destination Earth ("desterth")

Known issues:
- The color stripes are not placed correctly



Carnival ("carnival")
---------------------

Doesn't work yet!



Mario Bros. ("mario")
---------------------

Arrows  Move around player 1
CTRL    Jump player 1
Z,X     Move around player 2
SPACE   Jump player 2
F1      Test (keep it pressed - very nice, try it!)



Zaxxon ("zaxxon")
---------------------

Arrows  Move around
CTRL    Fire



Congo Bongo ("congo")
---------------------

Runs on the same hardware as Zaxxon.

Arrows  Move around
CTRL    Jump



Bomb Jack ("bombjack")
----------------------

Arrows  Move around
CTRL    Jump

Press fire to skip the ROM/RAM test at the beginning.

In the dip switch menu, DIFFICULTY 1 refers to the speed of the mechanical
bird, while DIFFICULTY 2 to the number and speed of enemies.

Known issues:
- Colors are accurate, but not entirely: the original machine uses 12 bits
  (4 bits per pen), while I currently use only 8 bits (3 bits for red and
  green, 2 bits for blue).
- Is the sprite priority correct? For example, during the demo, should GAME
  OVER be in front or behind other sprites?
  There is another bit in the sprite attributes which I don't know what means:
  it seems to be set only when the (B) materializes.
- For the DIFFICULTY 2 setting, EASY and MEDIUM might be swapped. Anyone can
  confirm?
- The INITIAL HIGH SCORE setting doesn't only set that, it does something
  else as well - but I don't know what.



Centipede ("centiped")
----------------------

Arrows  Move around
CTRL    Fire

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.
- The game awards you 18 credits on startup



Nibbler ("nibbler")
-------------------

Arrows  Move around
F1      Skip level

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz
- Some input bits seem to be used as debug controls - quite interesting, but
  I haven't investigated yet.
- One ROM fails the power on test, but the game seems to work well - maybe
  it's a bootleg?
