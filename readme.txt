
              M.A.M.E.  -  Multiple Arcade Machine Emulator
         Copyright (C) 1997  by Nicola Salmoria and Mirko Buffoni

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    You can contact us via e-mail to the following addresses:

    Nicola Salmoria:   MC6489@mclink.it
    Mirko Buffoni:     mix@lim.dsi.unimi.it

    Note to the GPL from Nicola and Mirko :

    This General Public License does not limit the use of MAME specific
    source code (drivers, machine emulation etc.). Use of the code is even
    encouraged. But do note that if you make a 'derived' work as stated in
    the full GPL, it is required that full source code must be made available.

    Making available full source code will benefit both the MAME project
    developers and MAME users and is the sole reason for bringing MAME
    under the GPL.


Important note:  from now on I, Mirko, am the coordinator of project MAME.
I created a new account address for those of you who intend to contribute to
MAME with drivers, or bug fixes.  The address is  "mame@lim.dsi.unimi.it".
If you have personal mails (requests or other) send them to my standard
e-mail address.  Thank you for collaboration.

Please note that many people helped with this project, either directly or
by making source code available which I examined to write the drivers. I am
not trying to appropriate merit which isn't mine. See the acknowledgemnts
section for a list of contributors.


Here is a quick list of ther currently supported games; read on for details.
The list doesn't include variants of the same game.


==============================================================================
|                             |        |Accurate|        |Hi score| Directory|
| Game Name                   |Playable| colors | Sound  |  save  |   Name   |
==============================================================================
| 1942                        |  Yes   |  Yes   |  Yes   |  Yes   | 1942     |
|-----------------------------|--------|--------|--------|--------|----------|
| Amidar                      |  Yes   |  Yes   |  Yes   |  Yes   | amidar   |
|-----------------------------|--------|--------|--------|--------|----------|
| Arabian                     |  Yes   | Close  |  Yes   |   No   | arabian  |
|-----------------------------|--------|--------|--------|--------|----------|
| Bagman                      |  Yes   |   No   | Music  |   No   | bagman   |
|-----------------------------|--------|--------|--------|--------|----------|
| Bomb Jack                   |  Yes   |  Yes   |  Yes   |  Yes   | bombjack |
|-----------------------------|--------|--------|--------|--------|----------|
| Burger Time                 |  Yes   |  Yes   |  Yes   |  Yes   | btime    |
|-----------------------------|--------|--------|--------|--------|----------|
| Carnival                    |  Yes   | Maybe  |   No   |   No   | carnival |
|-----------------------------|--------|--------|--------|--------|----------|
| Centipede                   |  Yes   |  Yes   |  Yes   |  Yes   | centiped |
|-----------------------------|--------|--------|--------|--------|----------|
| Commando                    |  Yes   |  Yes   |Partial.|  Yes   | commando |
|-----------------------------|--------|--------|--------|--------|----------|
| Congo Bongo                 |  Yes   |   No   |   No   |  Yes   | congo    |
|-----------------------------|--------|--------|--------|--------|----------|
| Crazy Climber               |  Yes   |  Yes   |  Yes   |  Yes   | cclimber |
|-----------------------------|--------|--------|--------|--------|----------|
| Crazy Kong                  |  Yes   |  Yes   |  Yes   |  Yes   | ckong    |
|-----------------------------|--------|--------|--------|--------|----------|
| Crush Roller                |  Yes   |  Yes   |  Yes   |  Yes   | crush    |
|-----------------------------|--------|--------|--------|--------|----------|
| Diamond Run                 |  Yes   |  Yes   |  Yes   |   No   | diamond  |
|-----------------------------|--------|--------|--------|--------|----------|
| Donkey Kong                 |  Yes   | Close  | Yes(1) |  Yes   | dkong    |
|-----------------------------|--------|--------|--------|--------|----------|
| Donkey Kong Jr.             |  Yes   | Close  |   No   |  Yes   | dkongjr  |
|-----------------------------|--------|--------|--------|--------|----------|
| Donkey Kong 3               |  Yes   |   No   |   No   |  Yes   | dkong3   |
|-----------------------------|--------|--------|--------|--------|----------|
| Elevator Action - Bootleg   |  Yes   |  Yes   |   No   |  n/a   | elevatob |
|-----------------------------|--------|--------|--------|--------|----------|
| Elevator Action - Original  |   No   |   No   |   No   |   No   | elevator |
|-----------------------------|--------|--------|--------|--------|----------|
| Fantasy                     |   No   |   No   |   No   |   No   | fantasy  |
|-----------------------------|--------|--------|--------|--------|----------|
| Frogger                     |  Yes   | Close  |  Yes   |  Yes   | frogger  |
|-----------------------------|--------|--------|--------|--------|----------|
| Galaga                      |  Yes   |  Yes   | Yes(1) |   No   | galaga   |
|-----------------------------|--------|--------|--------|--------|----------|
| Galaga - Bootleg            |  Yes   |  Yes   | Yes(1) |   No   | galagabl |
|-----------------------------|--------|--------|--------|--------|----------|
| Galaxian                    |  Yes   |  Yes   | Limited|  Yes   | galaxian |
|-----------------------------|--------|--------|--------|--------|----------|
| Ghost 'n Goblin             |  Yes   |  Yes   | Partial|   No   | gng      |
|-----------------------------|--------|--------|--------|--------|----------|
| Gorf                        |   No   |   No   |   No   |  n/a   | gorf     |
|-----------------------------|--------|--------|--------|--------|----------|
| Green Beret                 |  Yes   |  Yes   |  Yes   |  Yes   | gberet   |
|-----------------------------|--------|--------|--------|--------|----------|
| Gyruss                      |  Yes   | Close  | Yes(1) |  Yes   | gyruss   |
|-----------------------------|--------|--------|--------|--------|----------|
| Gingateikoku No Gyakushu    |  Yes   |  Yes   | Limited|   No   | japirem  |
|-----------------------------|--------|--------|--------|--------|----------|
| Jump Bug                    |  Yes   |   No   |  Yes   |   No   | jumpbug  |
|-----------------------------|--------|--------|--------|--------|----------|
| Jungle King                 |  Yes   | Close  |   No   |   No   | junglek  |
|-----------------------------|--------|--------|--------|--------|----------|
| Kangaroo                    |  Yes   |   No   |  Yes   |  n/a   | kangaroo |
|-----------------------------|--------|--------|--------|--------|----------|
| Krull                       |  Yes   |  Yes   |   No   |  n/a   | krull    |
|-----------------------------|--------|--------|--------|--------|----------|
| Lady Bug                    |  Yes   |  Yes   |  Yes   |  Yes   | ladybug  |
|-----------------------------|--------|--------|--------|--------|----------|
| Lost Tomb                   |   No   |   No   |  Yes   |  n/a   | losttomb |
|-----------------------------|--------|--------|--------|--------|----------|
| Mario Bros.                 |  Yes   | Close  |   No   |  Yes   | mario    |
|-----------------------------|--------|--------|--------|--------|----------|
| Mad Planets                 |  Yes   |  Yes?  | Yes(1) |  n/a   | mplanets |
|-----------------------------|--------|--------|--------|--------|----------|
| Millipede                   |  Yes   |   No   |   No   |  Yes   | milliped |
|-----------------------------|--------|--------|--------|--------|----------|
| Moon Cresta                 |  Yes   |  Yes   | Limited|  Yes   | mooncrst |
|-----------------------------|--------|--------|--------|--------|----------|
| Moon Patrol                 |  Yes   |   No   |   No   |   No   | mpatrol  |
|-----------------------------|--------|--------|--------|--------|----------|
| Moon Quasar                 |  Yes   |  Yes   | Limited|  Yes   | moonqsr  |
|-----------------------------|--------|--------|--------|--------|----------|
| Mouse Trap                  |Partial.|   No   |   No   |   No   | mtrap    |
|-----------------------------|--------|--------|--------|--------|----------|
| Mr. Do!                     |  Yes   |  Yes   |  Yes   |  Yes   | mrdo     |
|-----------------------------|--------|--------|--------|--------|----------|
| Mr. Do's Castle             |   No   |   No   |   No   |  n/a   | docastle |
|-----------------------------|--------|--------|--------|--------|----------|
| Ms Pac Man (bootleg)        |  Yes   |  Yes   |  Yes   |  Yes   | mspacman |
|-----------------------------|--------|--------|--------|--------|----------|
| Nibbler                     |  Yes   |   No   |   No   |  Yes   | nibbler  |
|-----------------------------|--------|--------|--------|--------|----------|
| Pac Man                     |  Yes   |  Yes   |  Yes   |  Yes   | pacman   |
|-----------------------------|--------|--------|--------|--------|----------|
| Pengo                       |  Yes   |  Yes   |  Yes   |  Yes   | pengo    |
|-----------------------------|--------|--------|--------|--------|----------|
| Pepper II                   |Partial.|   No   |   No   |   No   | pepper2  |
|-----------------------------|--------|--------|--------|--------|----------|
| Phoenix                     |  Yes   | Close  | Yes(1) |   No   | phoenix  |
|-----------------------------|--------|--------|--------|--------|----------|
| Pisces                      |  Yes   |  Yes   | Limited|   No   | pisces   |
|-----------------------------|--------|--------|--------|--------|----------|
| Pleiads                     |  Yes   |   No   | Limited|   No   | pleiads  |
|-----------------------------|--------|--------|--------|--------|----------|
| Pooyan                      |  Yes   | Close  |  Yes   |  Yes   | pooyan   |
|-----------------------------|--------|--------|--------|--------|----------|
| Popeye - Bootleg            |  Yes   |  Yes   |  Yes   |  Yes   | popeyebl |
|-----------------------------|--------|--------|--------|--------|----------|
| Q*Bert                      |  Yes   |  Yes   | Yes(1) |   No   | qbert    |
|-----------------------------|--------|--------|--------|--------|----------|
| Rally X                     |  Yes   |   No   |Partial.|   No   | rallyx   |
|-----------------------------|--------|--------|--------|--------|----------|
| Scramble                    |  Yes   |  Yes   |  Yes   |   No   | scramble |
|-----------------------------|--------|--------|--------|--------|----------|
| Seicross                    |   No   |   No   |  Yes   |  n/a   | seicross |
|-----------------------------|--------|--------|--------|--------|----------|
| Space Invaders              |  Yes   |  Yes   | Yes(1) |  Yes   | invaders |
|-----------------------------|--------|--------|--------|--------|----------|
| Space Panic                 |  Yes   | Close  |   No   |  Yes   | panic    |
|-----------------------------|--------|--------|--------|--------|----------|
| Super Cobra                 |  Yes   |   No   |  Yes   |   No   | scobra   |
|-----------------------------|--------|--------|--------|--------|----------|
| Super PacMan                |  Yes   | Close  |   No   |   No   | superpac |
|-----------------------------|--------|--------|--------|--------|----------|
| The Adventures of Robby Roto|   No   |   No   |   No   |  n/a   | robby    |
|-----------------------------|--------|--------|--------|--------|----------|
| The End                     |  Yes   |   No   |  Yes   |   No   | theend   |
|-----------------------------|--------|--------|--------|--------|----------|
| Time Pilot                  |  Yes   |  Yes   |  Yes   |  Yes   | timeplt  |
|-----------------------------|--------|--------|--------|--------|----------|
| Turtles                     |  Yes   |  Yes?  |  Yes   |   No   | turtles  |
|-----------------------------|--------|--------|--------|--------|----------|
| Vanguard                    |  Yes   |  Yes   |   No   |   No   | vanguard |
|-----------------------------|--------|--------|--------|--------|----------|
| Venture                     |Partial.|   No   |   No   |   No   | venture  |
|-----------------------------|--------|--------|--------|--------|----------|
| Vulgus                      |  Yes   |   No   |  Yes   |  Yes   | vulgus   |
|-----------------------------|--------|--------|--------|--------|----------|
| War of the Bugs             |  Yes   |   No   | Limited|   No   | warofbug |
|-----------------------------|--------|--------|--------|--------|----------|
| Warp Warp                   |  Yes   |   No   |   No   |  n/a   | warpwarp |
|-----------------------------|--------|--------|--------|--------|----------|
| Wizard of Wor               |  Yes   | Maybe  |   No   |   No   | wow      |
|-----------------------------|--------|--------|--------|--------|----------|
| Zaxxon                      |  Yes   | Close  |   No   |  Yes   | zaxxon   |
|-----------------------------|--------|--------|--------|--------|----------|

(1) Need samples provided separately



1942 ("1942")
=============
Arrows  Move around
CTRL    Fire
ALT     Air Routing



Amidar ("amidar")
=================
Arrows  Move around
CTRL    Jump

Clones supported:
  Japanese version ("amidarjp"). This version has a worse attract mode and
                                 does not display the number of jumps left.
Known issues:
- What do the dip switches do?



Arabian ("arabian")
===================
Arrows  Move around
CTRL    Kick
F1      Enter test mode



Bagman ("bagman")
=================
Arrows  Move around
CTRL    Action



Bomb Jack ("bombjack")
======================
Arrows  Move around
CTRL    Jump

Press fire to skip the ROM/RAM test at the beginning.

In the dip switch menu, DIFFICULTY 1 refers to the speed of the mechanical
bird, while DIFFICULTY 2 to the number and speed of enemies.
SPECIAL refers to how often the (E) and (S) coins appear.

Known issues:
- There is a bit in the sprite attributes which I don't know what means:
  it seems to be set only when the (B) materializes.
- The INITIAL HIGH SCORE setting doesn't only set that, it does something
  else as well - but I don't know what.



Burger Time ("btime")
=====================
Arrows  Move around
CTRL    Pepper
F1      \  Various tests.
F2      |  Use F1 to cycle through tests while in test mode.
F1+F2   /

Clones supported:
  different ROM set, without Midway copyright and different demo ("btimea")

Known issues:
- The way I turn sprites off is not correct.



Carnival ("carnival")
=====================
Arrows  Move around
CTRL    Fire



Centipede ("centiped")
======================
Arrows  Move around
CTRL    Fire

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.
- The game awards you 18 credits on startup



Congo Bongo ("congo")
=====================
Runs on the same hardware as Zaxxon.

Arrows  Move around
CTRL    Jump



Commando ("commando")
=====================
Arrows  Move around
CTRL    Fire
Press CTRL during the startup message to enter the test screen

known issues:
- The YM2203 sound chip is emulated as a 8910. Music is missing.



Crazy Climber ("cclimber")
==========================
E,S,D,F Left joystick
I,J,K,L Right joystick
F1      Skip level

Clones supported:
  Japanese version ("ccjap")
  bootleg version ("ccboot")



Crazy Kong ("ckong")
====================
This Donkey Kong clone runs on the same hardware as Crazy Climber, most
notable differencies being a larger character set and the display rotated
90 degrees.

Arrows  Move around
CTRL    Jump

Clones supported:
  version running on Scramble hardware ("ckongs")

Known issues:
- Some problems with sound



Crush Roller ("crush")
======================
Crush Roller is a hacked version of Make Trax, modified to run on a
Pac Man board.

Arrows  Move around
F1      Skip level



Diamond Run ("diamond")
=======================
This game is based on the same hardware as Ghost 'n Goblin

Arrows  Move around

Known issues:
- Don't know if banks mode switch is correct



Donkey Kong ("dkong")
=====================
Arrows  Move around
CTRL    Jump

Clone supported:
  japanese nintendo version ("dkongjp").  This version has the bug that
  barrels do not come down when at the top of a ladder, but the levels
  play in the order barrels-pies-elevators-girders instead of
  barrels-girders-barrels-elevators-girders...



Donkey Kong Jr. ("dkongjr")
===========================
Runs on hardware similar to Donkey Kong

Arrows  Move around
CTRL    Jump



Donkey Kong 3 ("dkong3")
========================
Runs on hardware similar to Donkey Kong

Arrows  Move around
CTRL    Fire
F1      Test (keep it pressed - very nice, try it!)



Elevator Action ("elevator")
============================
Should run on hardware similar to Jungle King.



Elevator Action - Bootleg ("elevatob")
======================================
Arrows  Move around
CTRL    Fire1
ALT     Fire2



Fantasy ("fantasy")
===================
Runs on the same hardware as Nibbler.

Not playable yet!



Frogger ("frogger")
===================
Arrows  Move around

Clones supported:
  alternate version, smaller, with different help, but still (C) Sega 1981
     ("frogsega")
  bootleg version, which runs on a modified Scramble board ("froggers")



Galaga ("galaga")
=====================
Original version with Namco copyright

Arrows  Move around
CTRL    Fire
F1      Skip Level

Clone supported:
  a version with a Z80 that emulates custom I/O chips.  However we don't
  care this because we emulate the chips instead ("galagabl")

Known issues:
- Explosions are implemented in a tricky way (reset game and you'll see!)
- Hiscore support not ready yet



Galaxian ("galaxian")
=====================
Original version with Namco copyright

Arrows  Move around
CTRL    Fire
F2      Test mode

  original with Midway copyright ("galmidw")
  and several bootlegs:
  one with Namco copyright ("galnamco")
  Super Galaxian ("superg")
  Galaxian Part X ("galapx")
  Galaxian Part 1 ("galap1")
  Galaxian Part 4 ("galap4")
  Galaxian Turbo ("galturbo")

Known issues:
- The star background is probably not entirely accurate.



Ghost 'n Goblin ("gng")
=======================
Arrows  Move around
CTRL    Fire
ALT     Jump
F2      Test mode

Known issues:
- To continue a game, insert coin and keep pressed CTRL+1 (or CTRL+2)
- Music is missing.  Maybe the original arcade uses YM2203 sound chips.
- Original machine has 4096 colors.  We now simulate them with a palette of
  256.  We need a skilled player, with a powerful machine (Pentium 133 or
  better, so he doesn't have to skip frames), that can finish the game with
  the -log option and send us the log file so we can implement 100% correct
  colors.



Gorf ("gorf")
=============
This game runs on the same hardware as Wizard of Wor, but doesn't work yet.
It boots, shows some text on the screen and that's all.



Green Beret ("gberet")
======================
Arrows  Move around
CTRL    Knife
ALT     Fire

Clones supported:
  US version, called Rush'n Attack ("rushatck")

Known issues:
- The music starts with what seems a correct pitch, but changes after you die
  for the first time or finishe the first level. Weird.



Gyruss ("gyruss")
===================
Arrows  Move around
CTRL    Fire

Known issues:
- Some of the components of spaceship do wraparound the top of the screen
  for a while.  Dunno if the original machine does.



Gingateikoku No Gyakushu ("japirem")
====================================
This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Clones supported:
  Uniwars ("uniwars")

Known issues:
- The star background is probably not entirely accurate.
- What does dip switch 6 do?



Jump Bug ("jumpbug")
====================
Arrows  Move around
CTRL    Fire
ALT     Jump

Clones supported:
  "Sega" version ("jbugsega")

Known issues:
- The original version has now a decode_opcode, but controls appear to be
  reversed compared to the booleg version.  Haven't checked this yet.
- Graphics are wrong - the game has multiple character banks, not emulated yet.



Jungle King ("junglek")
=======================
Arrows  Move around
CTRL    Jump

Clones supported:
  bootleg version called Jungle Hunt ("jhunt")

Known issues:
- Sound support implemented.  Tarzan yell sample missing.  Is the pitch of
  sounds correct?




Kangaroo ("kangaroo")
=====================
Arrows  Move around
CTRL    Kick



Krull ("krull")
========================
Runs on the same hardware as Q*Bert

Arrows  Move around
E,S,D,F Firing joystick
F1      Test mode
F2      Select



Lady Bug ("ladybug")
====================
Arrows  Move around
F1      Skip level



Lost Tomb ("losttomb")
======================
This runs on a Super Cobra hardware.

Known issues:
- Not playable. Crashes during demo. Graphics are garbled. I think the ROMs
  are corrupted.



Mad Planets ("mplanets")
========================
Runs on the same hardware as Q*Bert

Arrows  Move around
CTRL    Fire

Known issues:
- The dialer (used to rotate the ship) is not supported.



Mario Bros. ("mario")
=====================
Runs on hardware similar to Donkey Kong

Arrows  Move around player 1
CTRL    Jump player 1
Z,X     Move around player 2
SPACE   Jump player 2
F1      Test (keep it pressed - very nice, try it!)



Millipede ("milliped")
======================
As you can imagine, this runs on the same hardware as Centipede.

Arrows  Move around
CTRL    Fire

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.
- High scores don't seem to work.
- Palette is not supported



Moon Cresta ("mooncrst")
========================
This runs on a hardware very similar to Galaxian.
The ROMs are encrypted. Nichibutsu copyright.

Arrows  Move around
CTRL    Fire

Clones supported:
  Unencrypted version ("mooncrsb")
  bootleg version called Fantazia ("fantazia")

Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Moon Patrol ("mpatrol")
=======================
Arrows  Move around
CTRL    Fire
ALT     Jump
F2+F3   Test mode (press and release, then be patient. After the RAM/ROM
                   tests, press 2 for an additional menu of options, then
                   left/right to choose the option, and 1 to select it)

Clones supported:
  bootleg version, called Moon Ranger ("mranger")

Known issues:
- No background graphics. I don't know where to place them... can anyone
  provide a screen snapshot?



Moon Quasar ("moonqsr")
=======================
This runs on a modified Moon Cresta board.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.



Mouse Trap ("mtrap")
====================
Runs on the same hardware as Venture.

Arrows  Move around
CTRL    Fire
3+F3    Test mode

Known issues:
- Collision detection doesn't work.



Mr. Do! ("mrdo")
================
Arrows  Move around
CTRL    Fire
F1      Skip level
CTRL+F3 Test mode

Clones supported:
  Version with additional Taito copyright ("mrdot")
  Mr. Lo! ("mrlo")



Mr. Do's Castle ("docastle")
============================
Not working yet!



Ms Pac Man ("mspacman")
=======================
Arrows  Move around
F1      Skip level
F2      Test mode
CTRL    Speed up cheat



Nibbler ("nibbler")
===================
Arrows  Move around
F1      Skip level

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz
- Some input bits seem to be used as debug controls - quite interesting, but
  I haven't investigated yet.



Pac Man ("pacman")
==================
Arrows  Move around
F1      Skip level
F2      Test mode
CTRL    Speed up cheat

Clones supported:
  Pac Man modification ("pacmod")
  Pac Man Plus ("pacplus")
  Namco Pac Man ("namcopac")
  Hangly Man ("hangly")
  Puck Man ("puckman")
  Piranha ("piranha")



Pengo ("pengo")
===============
Arrows  Move around
CTRL    Push
F1      Skip level
F2      Test mode

Clones supported:
  Penta ("penta")



Pepper II ("pepper2")
=====================
Runs on hardware similar to Venture.

Arrows  Move around
CTRL    Dog button
Z       Yellow button
X       Red button
C       Blue button
3+F3    Test mode

Known issues:
- Collision detection doesn't work - use 7 and 8 to simulate it.



Phoenix ("phoenix")
===================
Arrows  Move around
CTRL    Fire
ALT     Barrier

Clones supported:
  Phoenix Amstar ("phoenixa")



Pisces ("pisces")
=================
This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Pleiads ("pleiads")
===================
This runs on the same hardware as Phoenix.

Arrows  Move around
CTRL    Fire
ALT     Teleport



Pooyan ("pooyan")
=================
Runs on hardware similar to Time Pilot.

Arrows  Move around
CTRL    Fire

Known issues:
- The characters seem to use 16 color codes, however the color code for many
  characters has bit 4 set. I don't know what it's for.



Popeye - bootleg ("popeyebl")
=============================

Arrows  Move around
CTRL    Fire
Q,W,E   Fire



Q*Bert ("qbert")
================
Arrows  Move around
To enter your name in the high score list, use 1 or 2.

Clones supported:
  Japanese version ("qbertjp")



Rally X ("rallyx")
==================
Arrows  Move around
CTRL    Smoke
F2      Test

Known issues:
- Sprites are not turned off appropriately.
- Cars are now displayed on the radar screen, but right color mapping
  must be done.
- I don't know if I reproduced the layout of the screen correctly.



Scramble ("scramble")
=====================
The video hardware is very similar to Galaxian, main differences being that
bullets are not vertical lines and the star background doesn't scroll.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Clones supported:
  Battle of Atlantis ("atlantis") [I don't know what most of the dip switches
                                   do, and you get a massive 14 credits per
                                   coin - now that's what I call good value
                                   for money! ;-)]

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Seicross ("seicross")
=====================
Runs on almost the same hardware as Crazy Climber, but not exactly the same.

Not playable.



Space Invaders ("invaders")
===========================
Arrows  Move around
CTRL    Fire

Clones supported (some of these have wrong colors, and the dip switch menu
      doesn't work):
  Super Earth Invasion ("earthinv")
  Space Attack II ("spaceatt")
  Space Invaders Deluxe ("invdelux")
  Invaders Revenge ("invrvnge")
  Galaxy Wars ("galxwars")
  Lunar Rescur ("lrescue")
  Destination Earth ("desterth")



Space Panic ("panic")
=====================
Arrows  Move around
CTRL    Fire1
ALT     Fire2



Super Cobra ("scobra")
======================
Runs on the same hardware as Scramble.
This is the version with Stern copyright.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Clones supported:
  Konami copyright ("scobrak")
  bootleg version ("scobrab")

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Super Pacman ("superpac")
=========================
Arrows  Move around
CTRL    Speed

Known issues:
- Large sprites in main sprite RAM (0x0810) do not work right. This
  affects the intermissions.
- Adding credits does not work right.
- Some colors are almost right but still off (i.e., orange is too light
  in the logo).
- Bonus star is blue (should be yellow?). GAME OVER is white (should be
  red?).
- Can't enter initials at high score screen.
- Sound CPU is not emulated.
- High score saving not implemented.



The Adventures of Robby Roto ("robby")
======================================
This game runs on the same hardware as Wizard of Wor, but doesn't work yet. I
still haven to check the loading address of the ROMs.



The End ("theend")
==================
This runs on a Scramble hardware.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Time Pilot ("timeplt")
======================
Arrows  Move around
CTRL    Fire

Clones supported:
  bootleg version ("spaceplt")

Known issues:
- This game uses double-width sprites for the clouds, but I haven't yet figured
  out they are selected. The code is currently a hack - just double the sprites
  which I know are used for clouds...
- The memory mapped read port at c000 puzzles me...



Turtles ("turtles")
===================
This runs on the same hardware as Amidar

Arrows  Move around
CTRL    Bomb



Vanguard ("vanguard")
=====================
Runs on hardware similar to Nibbler.

Arrows  Move around
S,D,E,F Fire



Venture ("venture")
===================
Arrows  Move around
CTRL    Fire
3+F3    Test mode
On startup, keep 1 or 2 pressed to proceed

Known issues:
- Collision detection doesn't work - use 7 and 8 to simulate it.



Vulgus ("vulgus")
===================
Arrows  Move around
CTRL    Fire
ALT     Big Missile

Known issues:
- Colors are wrong.  Manual correction is a foolish!
- Sound speed should be accurate.  It seems not to use FM.  Anyone confirm?



Wizard of Wor ("wow")
=====================
Arrows  Move around
CTRL    Fire
F2      Test mode (keep it pressed)
The original machine had a special joystick which could be moved either
partially or fully in a direction. Pushing it slightly would turn around the
player without moking it move. The emulator assumes that you are always
pushing the joystick fully, to simulate the "half press" you can press Alt.

Known issues:
- No background stars, no fade in/fade out.



War of the Bugs ("warofbug")
============================
This runs on the same hardware as Galaxian.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Zaxxon ("zaxxon")
===================
Arrows  Move around
CTRL    Fire




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
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator is based on L.C. Benschop's 6809 Simulator V09.
  Copyright 1994,1995 L.C. Benschop, Eidnhoven The Netherlands.
  This version of the program is distributed under the terms and conditions
  of the GNU General Public License version 2. See the file COPYING.
  THERE IS NO WARRANTY ON THIS PROGRAM!!!
Allegro library by Shawn Hargreaves, 1994/96
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c. Thanks to Chuck Cochems for the help in making them more
   compatible.
224x288 noscanlines and both 288x224 video modes provided by Valerio Verrando
  (v.verrando@mclink.it)
AY-3-8910 emulation by Ville Hallik (ville@physic.ut.ee) and Michael Cuddy
  (mcuddy@FensEnde.com).
POKEY emulator by Ron Fries (rfries@tcmail.frco.com).
UNIX port by Allard van der Bas (avdbas@wi.leidenuniv.nl) and Dick de Ridder
  (dick@ph.tn.tudelft.nl).

Phoenix driver provided by Brad Oliver (bradman@primenet.com), Mirko
   Buffoni (mix@lim.dsi.unimi.it) and Richard Davies (R.Davies@dcs.hull.ac.uk)
1942, Commando, Elevator Action and Galaga drivers by Nicola Salmoria.
Gyruss, Ghost 'n Goblin, Mario Bros., Vulgus, Zaxxon, Bomb Jack, Burger Time
   and Donkey Kong 3 drivers provided by Mirko Buffoni (mix@lim.dsi.unimi.it)
Bomb Jack sound driver by Jarek Burczynski (pbk01@ikp.atm.com.pl).
Arabian driver provided by Jarek Burczynski (pbk01@ikp.atm.com.pl).
Congo Bongo and Kangaroo drivers provided by Ville Laitinen (ville@sms.fi).
Millipede driver provided by Ivan Mackintosh (ivan@rcp.co.uk).
Donkey Kong sound emulation by Ron Fries (rfries@tcmail.frco.com).
Vanguard driver by Brad Oliver and Mirko Buffoni, based on code by Brian
   Levine.
Carnival driver completed by Mike Coates and Richard Davies.
Warp warp driver completed by Chris Hardy (chrish@kcbbs.gen.nz).
Popeye driver provided by Marc LaFontaine and Nicola Salmoria.
Jump Bug driver by Richard Davies (R.Davies@dcs.hull.ac.uk) and Brad Oliver
   (bradman@primenet.com).
Venture, Mouse Trap and Pepper II drivers by Marc Lafontaine
   (marclaf@sympatico.ca).
Q*Bert, Mad Planets, Reactor and Krull drivers by Fabrice Frances
   (frances@ensica.fr)
Space Panic driver by Mike Coates (mike@dissfulfils.co.uk)


Very special thanks to Sergio Munoz for the precious information about the
   Pengo sound hardware and colors.
Thanks to Paul Swan for the information on the Lady Bug sound hardware and
   Mr.Do! and Green Beret colors.
Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention them all.
Thanks to Paul Johnson (mayhem@cix.compulink.co.uk) for the ROM encryption
   scheme and colors of Commando.
Thanks to Simon Walls (wallss@ecid.cig.mot.com) for the color information
   on many games.
Information about the Crazy Climber machine hardware (including palette)
   and ROM encryption scheme provided by Lionel Theunissen
   (lionelth@ozemail.com.au).
Thanks to Andy Milne (andy@canetics.com) for the information on the Crazy
   Climber sound roms.
Thanks to Toninho (fastwork@bios.com.br) and Paul Leaman for the information
   on Vulgus.
Crazy Kong emulation set up by Ville Laitinen (ville@sms.fi).
Kevin Brisley's SUPERPAC.KEG file (for his excellent REPLAY emulator)
  provided the crucial info on Super Pacman.
Very special thanks to Michael Cuddy for the extensive information on
   Gyruss hardware (You'r right, it's a bear of a game!).
Special thanks to Brad Thomas (bradt@nol.net) and Gary Shepherdson for the
   extensive information on Donkey Kong and Donkey Kong Jr.
Info on Bagman, Galaxian, Moon Cresta and many other games taken from Arcade
   Emulator by Robert Anschuetz.
Pooyan information provided by Michael Cuddy and Allard van der Bas
Thanks to Mirko Buffoni for the Amidar and Frogger colors.
Thanks to Brad Thomas, Jakob Frendsen and Conny Melin for the info on Bomb
   Jack.
Thanks to Mike@Dissfulfils.co.uk for the information on the Moon Quasar
   encryption scheme.
Space Invaders information gathered from the Space Invaders Emulator by
   Michael Strutt (mstrutt@pixie.co.za)
Thanks to Paul Leaman (paull@phonelink.com) for exaustive documentation on
   1942 arcade board.
Many thanks to Jim Hernandez for the information on Wizard of Wor hardware.
Thanks to Mike Coates (mike@dissfulfils.co.uk) for Carnival ROM placement
   indications and gfx info.
Colors for Donkey Kong, Donkey Kong Jr. and Mario Bros. derived from Kong
   emulator by Gary Shepherdson.
Colors for Amidar, Frogger and Zaxxon derived from SPARCADE by Dave Spicer.
Thanks to Brad Oliver, Marc Vergoossen (marc.vergoossen@pi.net) and Richard
   Davies (R.Davies@dcs.hull.ac.uk) for help with Donky Kong Jr. colors.
Thanks to Marc Vergoossen and Marc Lafontaine (marclaf@sympatico.ca) for
   Zaxxon colors.
Thanks to Marc Lafontaine for Congo Bongo colors and Popeye bootleg.
Centipede information taken from Centipede emulator by Ivan Mackintosh, MageX
   0.3 by Edward Massey and memory map by Pete Rittwage.
Info on Burger Time taken from Replay 0.01a by Kevin Brisley (kevin@isgtec.com)
Thanks to Chris Hardy (chrish@kcbbs.gen.nz) for info on Moon Patrol.
Thanks to Dave W. (hbbuse08@csun.edu) for all his help.
Thanks to Doug Jefferys (djeffery@multipath.com) for Crazy Kong color
   information.
Thanks to Philip Chapman (Philip_Chapman@qsp.co.uk) for useful feedback on
   Bomb Jack.
Thanks to Mike Cuddy for Pooyan and Time pilot colors.
Thanks to Thomas Meyer for Moon Patrol screenshots.
Many thanks to Steve Scavone (krunch@intac.com) for his invaluable help with
   Wizard of Wor and related games.
Vesa 2.0 linear and banked extensions, -vesascan and -vesaskip implemented
   by Bernd Wiebelt (bernardo@studi.mathematik.hu-berlin.de)
Thanks to Stefano Mozzi (piu1608@cdc8g5.cdc.polimi.it) for Mario Bros. colors.
Thanks to Matthew Hillmer (mhillmer@pop.isd.net) for Donkey Kong 3 colors.
Thanks to Tormod Tjaberg (tormod@sn.no) and Michael Strutts for Space Invaders
   sound.
Thanks to Shaun Stephenson (shaun@marino13.demon.co.uk) for Phoenix samples.


Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -nosound   will run Ms Pac Man without sound

options:
-noscanlines  use alternate video mode (not availble in all games). Use this
              if the default mode doesn't work with your monitor/video card.
-vesa         use standard 640x480x256 VESA 1.2 mode instead of custom video
              mode. Use this as a last resort if -noscanlines doesn't solve
              your video problems.
-vesa2b       use VESA2.0 banked mode extension
-vesa2l       use VESA2.0 linear mode extension
-640          tell MAME to use a vesa 640x480 video mode
-800          same as above, video mode is 800x600
-1024         same as above, video mode is 1024x768

-vesascan     use a VESA 800x600 screen to simulate scanlines. This is much
              slower than the other video modes. Use this if you want
              scanlines and the default video mode doesn't work.
              For frontend authors: same as '-vesa -800'.
-vesaskip n   similar to -vesascan, but use a 640x480 screen instead of
              800x600. Since most games use a screen taller than 240 lines,
              it won't fit in the screen - n sets the initial number of lines
              to skip at the top of the screen. You can adjust the position
              while the game is running using the PGUP and PGDOWN keys.
-vgafreq n    where n can be 0 (default) 1, 2 or 3.
              use different frequencies for the custom video modes. This
              could reduce flicker, especially in the 224x288noscanlines
              mode. WARNING: THE FREQUENCIES USED MIGHT BE WAY OUTSIDE OF
              YOUR MONITOR RANGE, AND COULD EVEN DAMAGE IT. USE THESE OPTIONS
              AT YOUR OWN RISK.
-vsync        syncronize video display with the video beam instead of using
              the timer. This works best with -noscanlines and the -vesaxxx
              modes. Use F11 to check your actual frame rate - it should be
              around 60. If it is lower, try to increase it with -vgafreq (if
              you are using a tqeked video mode) or use your video board
              utilities to set the VESA refresh rate to 60 Hz.
              Note that when this option is turned on, speed will NOT
              downgrade nicely if your system is not fast enough.
-soundcard n  select sound card (if this is not specified, you will be asked
              interactively)
-nojoy        don't poll joystick
-log          create a log of illegal memory accesses in ERROR.LOG
-list         display a list of currently supported games
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
F4      Show the game graphics. Use cursor keys to change set/color, F4 to exit
F8      To select keys assignment.
F9      To change volume percentage thru 100,75,50,25,0 values
F10     Toggle speed throttling
F11     Toggle fps counter
F12     Save a screen snapshot
ESC     Exit emulator
