
              M.A.M.E.  -  Multiple Arcade Machine Emulator
         Copyright (C) 1997  by Nicola Salmoria and the MAME team

Please note that many people helped with this project, either directly or
by releasing source code which was used to write the drivers. We are not
trying to appropriate merit which isn't ours. See the acknowledgemnts
section for a list of contributors, however please note that the list is
largely incomplete. See also the CREDITS section in the emulator to see
the people who contributed to a specific driver. Again, that list might
be incomplete. We apologize in advance for any omission.


Legal Issues
------------

1) The main purpose of MAME is to be a reference to the inner workings of
   the emulated Arcade Machines. This is done for educational purposes and
   to preserve many historical games from the oblivion they would sink
   into when the hardware they run on will stop working.
   Of course to preserve the games you must also be able to actually play
   them; you can see that as a nice side effect.
   It is not our intention to infringe any copyrights or patents pending on
   the original games. All of the source code is either our own or freely
   available. No portion of the code of the original ROMs is included in
   the executable.

2) Some of the following rules might not apply to specific portions of MAME
   (e.g. CPU emulators) which bear different copyright notices.

3) MAME is free. The source code is free. If you paid for it, you've been
   ripped off. If you sell it, you are a thief.

4) The MAME executables must be distributed only in their original .zip
   archives. You are not allowed to package them together with the original
   ROMs. You are not allowed to distribute them on the same physical medium.
   You ARE allowed to make them available for download on the same site, but
   only if you warn users about the copyright status of the ROMs and the
   legal issues involved.
   You are NOT allowed to make MAME available for download together with one
   giant big file containing all of the supported ROMs, or any files
   containing more than one ROM set each. If you want to distribute such
   files, you are not allowed to distribute MAME.
   You are not allowed to distribute a modified version, nor to remove
   and/or add files to the .zip archive. Adding one text file to advertise
   your site is tolerated only if your site contributes original material
   to the emulation scene.
   If you distribute the executable, you should also distribute the source
   code. If you can't do that, you must provide a pointer to a place where
   the source can be obtained.

5) The source code cannot be used in a commercial product without a written
   authorization of the authors. Use in non commercial products is allowed
   and indeed encouraged; however if you use portions of the MAME source
   code in your program, you must make the full source code freely available
   as well.
   Derivative works are allowed (provided source code is available), but
   discouraged: MAME is a project continuously evolving, and you should, in
   your best interest, submit your contributions to the development team, so
   that they are integrated in the main distribution.
   Usage of the _information_ contained in the source code is free for any
   use. However, given the amount of time and energy it took to collect this
   information, we would appreciate if you made the additional information
   you might have freely available as well.

6) All trademarks cited in this document are property of their respective
   owners.



How to Contribute
-----------------

You can contact us via e-mail to the following addresses:

Nicola Salmoria:   MC6489@mclink.it
Mirko Buffoni:     mix@lim.dsi.unimi.it


Important note:  from now on I, Mirko, am the coordinator of project MAME.
I created a new account address for those of you who intend to contribute to
MAME with drivers, or bug fixes.  The address is  "mame@lim.dsi.unimi.it".
Do not use previous address for non-source code bug reports.
If you have problems using this software, please read all the docs before
mailing us. Questions already answered in the docs will be ignored. Thanks
for the collaboration.



Supported Games
---------------

Here is a quick list of the currently supported games; read on for details.
The list doesn't include variants of the same game.


================================================================================
|                      |        |Accurate|        |Hi score|Cocktail|Directory |
| Game Name            |Playable| colors | Sound  |  save  |  mode  |   Name   |
================================================================================
| 10 yard Fight        |  Yes   |   No   |   No   |   No   |   No   | yard     |
|----------------------|--------|--------|--------|--------|--------|----------|
| 1942                 |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | 1942     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Amidar               |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | amidar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Anteater             |  Yes   |   No   |  Yes   |  Yes   |   No   | anteater |
|----------------------|--------|--------|--------|--------|--------|----------|
| Arabian              |  Yes   | Close  |  Yes   |  Yes   |   No   | arabian  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids            |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | asteroid |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids Deluxe     |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | astdelux |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bagman               |  Yes   |   No   | Music  |   No   |  Yes   | bagman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bank Panic           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | bankp    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle of Atlantis   |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | atlantis |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle Zone          |  Yes   | Close  |Partial.|  Yes   |   No   | bzone    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Black Widow          |  Yes   | Close  |  Yes   |   No   |   No   | bwidow   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blaster              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | blaster  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blue Print           |  Yes   |   No   |  Yes   |  Yes   |   No   | blueprnt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bomb Jack            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bombjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bosconian            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bosco    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubble Bobble        |  Yes   |  Yes   |   No   |  Yes   |   No   | bublbobl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubbles              |   No   |  Yes   | Yes(1) |  Yes   |   No   | bubbles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Burger Time          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | btime    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Carnival             |  Yes   |  Yes   |Partl(1)|  Yes   |   No   | carnival |
|----------------------|--------|--------|--------|--------|--------|----------|
| Centipede            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | centiped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Commando             |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | commando |
|----------------------|--------|--------|--------|--------|--------|----------|
| Congo Bongo          |  Yes   |   No   |   No   |  Yes   |   No   | congo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cosmic Avenger       |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | cavenger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Climber        |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | cclimber |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Kong           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ckong    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crush Roller         |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | crush    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crystal Castles      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | ccastles |
|----------------------|--------|--------|--------|--------|--------|----------|
| Defender             |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | defender |
|----------------------|--------|--------|--------|--------|--------|----------|
| Demolition Derby     |  Yes   |  Yes   |   No   |   No   |   No   | destderb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Diamond Run          |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | diamond  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Dig Dug              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | digdugnm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Dig Dug 2            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | digdug2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Discs of Tron        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | dotron   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Domino Man           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | domino   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong          |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | dkong    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong Jr.      |  Yes   | Close  |   No   |  Yes   |   No   | dkongjr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong 3        |  Yes   |   No   |   No   |  Yes   |   No   | dkong3   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eggs                 |  Yes   |   No   |   No   |  Yes   |   No   | eggs     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Elevator Action      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | elevator |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eliminator (2 Player)|  Yes   | Close  |   No   |   No   |   No   | elim2    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Espial               |  Yes   |   No   |   No   |   No   |   No   | espial   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exed Exes            |  Yes   |  Yes   |Partial |  Yes   |   No   | exedexes |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fantasy              |   No   |   No   |  Yes   |   No   |   No   | fantasy  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frogger              |  Yes   | Close  |  Yes   |  Yes   |   No   | frogger  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Front Line           |   No   |  n/a   |  Yes   |  n/a   |   No   | frontlin |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaga               |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | galaga   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaxian             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | galaxian |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ghosts 'n Goblins    |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | gng      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gorf                 |  Yes   |   No   |   No   |  Yes   |   No   | gorf     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gravitar             |  Yes   | Close  |  Yes   |  Yes   |   No   | gravitar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Green Beret          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | gberet   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gyruss               |  Yes   | Close  | Yes(1) |  Yes   |   No   | gyruss   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jr. Pacman           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | jrpacman |
|----------------------|--------|--------|--------|--------|--------|----------|
| Journey              |  Yes   |  Yes   |Partial.|  Yes   |   No   | journey  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Joust                |  Yes   |  Yes?  | Yes(1) |  Yes   |   No   | joust    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jump Bug             |  Yes   |   No   |  Yes   |   No   |   No   | jumpbug  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jungle King          |  Yes   |  Yes?  |Partial |  Yes   |   No   | junglek  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kangaroo             |  Yes   |   No   |  Yes   |   No   |   No   | kangaroo |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kick     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick Rider           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kickridr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kosmik Krooz'r       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kroozr   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Krull                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | krull    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kung Fu Master       |  Yes   |  Yes   |Partial.|  Yes   |   No   | kungfum  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lady Bug             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ladybug  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Loco-Motion          |  Yes   |   No   |  Yes   |  Yes   |   No   | locomotn |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lost Tomb            |  Yes   |   No   |  Yes   |   No   |   No   | losttomb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lunar Lander         |  Yes   |  Yes   |   No   |  Yes   |   No   | llander  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lunar Rescue         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | lrescue  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mad Planets          |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | mplanets |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mappy                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mappy    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mario Bros.          |  Yes   | Close  |   No   |  Yes   |   No   | mario    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Millipede            |  Yes   |   No   |   No   |  Yes   |   No   | milliped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Missile Command      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | missile  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Cresta          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mooncrst |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Patrol          |  Yes   |   No   |   No   |   No   |   No   | mpatrol  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Quasar          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | moonqsr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mouse Trap           |  Yes   |   No   |   No   |   No   |   No   | mtrap    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mr. Do!              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mrdo     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mr. Do's Castle      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | docastle |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mr. Do! Run Run      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | dorunrun |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mr. Do! Wild Ride    |  Yes   |  Yes   |  Yes   |  Yes   |   No   | dowild   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ms Pac Man           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | mspacman |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mysterious Stones    |  Yes   | Close  |   No   |  Yes   |   No   | mystston |
|----------------------|--------|--------|--------|--------|--------|----------|
| Nibbler              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | nibbler  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Naughty Boy          |  Yes   | Close  |   No   |  Yes   |   No   | naughtyb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Omega Race           |  Yes   |  Yes   |   No   |  Yes   |   No   | omegrace |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac Man              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pacman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac & Pal            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | pacnpal  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pengo                |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pengo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pepper II            |  Yes   |   No   |   No   |   No   |   No   | pepper2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Phoenix              |  Yes   | Close  | Yes(1) |  Yes   |   No   | phoenix  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pisces               |  Yes   |  Yes   |   Yes  |  Yes   |   No   | pisces   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pleiads              |  Yes   |   No   | Limited|  Yes   |   No   | pleiads  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pooyan               |  Yes   | Close  |  Yes   |  Yes   |   No   | pooyan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Popeye               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | popeyebl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pop Flamer           |  Yes   |   No   |   No   |   No   |   No   | popflame |
|----------------------|--------|--------|--------|--------|--------|----------|
| Qix                  |  Yes   |  Yes   |   No   |  Yes   |   No   | qix      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert               |  Yes   |  Yes   |Partial.|  Yes   |   No   | qbert    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert Qubes         |  Yes   |  Yes   |   No   |  Yes   |   No   | qbertqub |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rally X              |  Yes   |   No   | Yes(1) |   No   |   No   | rallyx   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rampage              |  Yes   |  Yes   |   No   |   No   |   No   | rampage  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Red Baron            |  Yes   | Close  |Partial.|  Yes   |   No   | redbaron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Reactor              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | reactor  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rescue               |  Yes   |   No   |  Yes   |   No   |   No   | rescue   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robotron             |  Yes   |  Yes?  | Yes(1) |  Yes   |   No   | robotron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Satan's Hollow       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | shollow  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Scramble             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | scramble |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sea Wolf ][          |  Yes   |   No   |   No   |  Yes   |   No   | seawolf2 |
|----------------------|--------|--------|--------|--------|--------|----------|
| Seicross             |  Yes   |   No   |  Yes   |   No   |   No   | seicross |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sinistar             |   No   |  Yes   | Yes(1) |  Yes   |   No   | sinistar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Snap Jack            |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | snapjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Solar Fox            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | solarfox |
|----------------------|--------|--------|--------|--------|--------|----------|
| Son Son              |  Yes   |  Yes   |  Yes   |  Yes   |  n/a   | sonson   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Duel           |  Yes   | Close  |  Yes   |  Yes   |   No   | spacduel |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Firebird       |  Yes   |  Yes   |   No   |   No   |   No   | spacefb  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Fury           |  Yes   | Close  |Part(1).|   No   |   No   | spacfury |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Invaders       |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | invaders |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Panic          |  Yes   | Close  |   No   |  Yes   |   No   | panic    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Zap            |  Yes   | Maybe  |   No   |  Yes   |   No   | spacezap |
|----------------------|--------|--------|--------|--------|--------|----------|
| Splat                |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | splat    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Spy Hunter           |  Yes   | Maybe  |Partial.|  Yes   |   No   | spyhunt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Starforce            |  Yes   |  Yes   |  Yes   |   No   |   No   | starforc |
|----------------------|--------|--------|--------|--------|--------|----------|
| Stargate             |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | stargate |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Trek            |  Yes   | Close  |Part(1).|   No   |   No   | startrek |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Wars            |  Yes   | Close  |Partial.|   No   |   No   | starwars |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Bagman         |  Yes   |   No   | Music  |   No   |  Yes   | sbagman  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Basketball     |  Yes   |   No   |   No   |   No   |   No   | sbasketb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Cobra          |  Yes   |   No   |  Yes   |   No   |   No   | scobra   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super PacMan         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | superpac |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tac/Scan             |  Yes   | Close  |   No   |   No   |   No   | tacscan  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tapper               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tapper   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tempest              |  Yes   | Close  |  Yes   |  Yes   |   No   | tempest  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robby Roto           |  Yes   |   No   |   No   |  Yes   |   No   | robby    |
|----------------------|--------|--------|--------|--------|--------|----------|
| The End              |  Yes   |   No   |  Yes   |  Yes   |   No   | theend   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Three Stooges        |  Yes   |  Yes   |   No   |  Yes   |   No   | 3stooges |
|----------------------|--------|--------|--------|--------|--------|----------|
| Timber               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | timber   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | timeplt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot 84        |  Yes   |  Yes   |  Yes   |   No   |   No   | tp84     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tron                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tron     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Turtles              |  Yes   |  Yes   |  Yes   |   No   |  Yes   | turtles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tutankham            |  Yes   |  Yes   |  Yes   |   No   |   No   | tutankhm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Two Tigers           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | twotiger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Uniwars              |  Yes   |  Yes   |  Yes   |   No   |   No   | japirem  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vanguard             |  Yes   |  Yes   | Yes(1) |   No   |   No   | vanguard |
|----------------------|--------|--------|--------|--------|--------|----------|
| Venture              |  Yes   |   No   |   No   |   No   |   No   | venture  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vulgus               |  Yes   |   No   |  Yes   |  Yes   |   No   | vulgus   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wacko                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | wacko    |
|----------------------|--------|--------|--------|--------|--------|----------|
| War of the Bugs      |  Yes   |   No   |  Yes   |  Yes   |   No   | warofbug |
|----------------------|--------|--------|--------|--------|--------|----------|
| Warlords             |  Yes   |   No   |  Yes   |   No   |   No   | warlord  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Warp Warp            |  Yes   |   No   |   No   |   No   |   No   | warpwarp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wild Western         |   No   |  Yes   |  Yes   |  n/a   |   No   | wwestern |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wizard of Wor        |  Yes   | Maybe  |   No   |  Yes   |   No   | wow      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Yie Ar Kung Fu       |  Yes   |   No   |   No   |  Yes   |   No   | yiear    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Xevious              |  Yes   |  Yes   | Yes(1) |   No   |   No   | xevious  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zaxxon               |  Yes   | Close  |   No   |  Yes   |   No   | zaxxon   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zektor               |  Yes   | Close  |Part(1).|   No   |   No   | zektor   |
|----------------------|--------|--------|--------|--------|--------|----------|

(1) Needs samples provided separately
(2) Requires OPL chip for FM Music



10 yard Fight ("yard")
======================
Arrows  Move around
CTRL    Throw
ALT     Pass



1942 ("1942")
=============



Amidar ("amidar")
=================
Clones supported:
  Japanese version ("amidarjp"). This version has a worse attract mode and
                                 does not display the number of jumps left.
  bootleg version called Amigo ("amigo")

Known issues:
- What do the dip switches do?



Anteater ("anteater")
=====================
This runs on the Super Cobra board.  Graphic is garbled with an address
shuffling.

Arrows  Move around
CTRL    Retreat tongue

Known issues:
- Graphics are now perfect
- I don't know where coin counter is located.  The game requires 4 credits
  to start



Arabian ("arabian")
===================
Arrows  Move around
CTRL    Kick
F1      Enter test mode



Asteroids ("asteroid")
======================
Arrows  Left & Right rotate ship, Up trust
CTRL    Fire
ALT     Hyperspace  (Shields in asteroid deluxe)

Clones supported:
  alternate version ("asteroi2")
  Asteroids Deluxe ("astdelux")



Bagman ("bagman")
=================
Known issues:
- Guards easily get stuck in corners and disappear after a while. This
  doesn't happen in the original.



Bank Panic ("bankp")
====================



Battle of Atlantis ("atlantis")
===============================
Scramble hardware.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Known issues:
- You get a massive 14 credits per coin - this is caused by ROM 2H, which
  seems to be bad.



Battlezone ("bzone")
====================
W, I    Left and Right forward
S, K    Left and Right backward
Space   Fire

Clones supported:
  alternate version ("bzone2")

There are two control methods:
  1) W,I,S,K,Space: Close to the original controls (two up/down sticks)
  2) Arrow keys+CTRL: simulates a 4-way joystick (no diagonal directions!)

The "Colors" dip switch setting is not a real dip switch of the machine. It
  is used to select between the "standard" greenish display, and a red/green
  display which was probably how the game was designed to be, before
  choosing to use monochrome monitor to cut costs.

Knwon issues:
- This game used a red overlay in the upper part of the screen.
  For speed reasons, this is not implemented.


Black Widow ("bwidow")
======================
E,S,D,F Move around
Arrows  Fire around



Blaster ("blaster")
===================
Lots of keys ;-)



Blue Print ("blueprnt")
=======================
Arrows  Move around
CTRL    Accelerate

Known issues:
- The sound is probably way wrong.



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



Bosconian ("bosco")
===================
Arrows  Move around
CTRL    Fire



Bubble Bobble ("bublbobl")
==========================
Arrows  Move around
ALT     Jump
CTRL    Fire

Clones supported:
  bootleg version called "Bobble Bobble" ("boblbobl")

Known issues:
- The original version doesn't seem to behave correctly, probably due to the
  copy protection. Use the bootleg instead.



Bubbles ("bubbles")
===================
Doesn't start.  It hangs after boot.



Burger Time ("btime")
=====================
Arrows  Move around
CTRL    Pepper
F1      \  Various tests.
F2      |  Use F1 to cycle through tests while in test mode.
F1+F2   /

Clones supported:
  different ROM set, without Midway copyright and different attract mode
  ("btimea")

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

Also this game supports trackball (-trak option)

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.



Commando ("commando")
=====================
Arrows  Move around
CTRL    Fire
Press CTRL during the startup message to enter the test screen

Clones supported:
  Japanese version ("commandj")


Congo Bongo ("congo")
=====================
Runs on almost the same hardware as Zaxxon.

Use F2 to enter test mode, then 1 to advance from one test to the following.

Known issues:
- What do the dip switches do?



Cosmic Avenger ("cavenger")
===========================
Runs on the same hardware as Lady Bug


Crazy Climber ("cclimber")
==========================
Clones supported:
  Japanese version ("ccjap")
  bootleg version ("ccboot")



Crazy Kong ("ckong")
====================
This Donkey Kong clone runs on the same hardware as Crazy Climber, most
notable differencies being a larger character set and the display rotated
90 degrees.

Clones supported:
  alternate version ("ckonga")
  bootleg version by Jeutel ("ckongjeu")
  version running on Scramble hardware ("ckongs")

Known issues:
- music doesn't work correctly in "ckongs".


Crush Roller ("crush")
======================
Crush Roller is a hacked version of Make Trax, modified to run on a
Pac Man board.



Crystal Castles ("ccastles")
============================
Arrows  Move around
CTRL    Jump

- Trackball support (-trak option)

Known issues:
- Sound.  It's better! The game sounds are slightly slow.....
- The game is always in cocktail mode.  If I try to take it out of
  cocktail mode, the player 1 and player 2 buttons no longer work.
  Needless to say, I have not emulated cocktail mode, nor do I plan to so
  only try 1 player games for now. :-)
- Vertical scrolling is wrong.  Is horizontal scrolling right?



Defender ("defender")
===========================
Up key  Up
Dn key  Down
Lft key Reverse
ALT     Thrust
CTRL    Fire
X       Smart bomb
C       Hyperspace
5       Goes into game setup menu
6       Diagnostic menu (only in attract mode)

Known issues:
- The joystick controls are strange, but Defender had lots of buttons.. ;-)



Demolition Derby ("destderb")
=============================
MCR/III hardware
Press F2 before initial screen to configure

Track   Steer player 1
CTRL    Accelerate forward
ALT     Accelerate backward

At this time, the game requires a trackball (-trak option)

Known issues:
- Players 2-4 aren't fully supported
- No sound (it should work in theory)



Diamond Run ("diamond")
=======================
This game runs on the same hardware as Ghosts 'n Goblins

Arrows  Move around

Known issues:
- Don't know if banks mode switch is correct
- Very slow



Dig Dug ("digdugnm")
====================
Namco copyright

Arrows  Move around
CTRL    Pump

Clones supported:
  Atari copyright ("digdugat")



Dig Dug 2 ("digdug2")
=====================
Runs on the same hardware as Mappy

Arrows  Move around
CTRL    Pump
ALT     Drill



Discs of Tron ("dotron")
========================
MCR/III hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire
Track   Aim
ALT     Deflect
A/Z     Vertical Aim

At this time, the game requires a trackball (-trak option)

Known issues:
- Speech is not emulated



Domino Man ("domino")
=====================
MCR/II hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Remove domino/Swat



Donkey Kong ("dkong")
=====================
Arrows  Move around
CTRL    Jump

Clones supported:
  japanese nintendo version ("dkongjp"). In this version barrels do not
  come down a ladder when you are standing at the top of it, and the
  levels play in the order barrels-pies-elevators-girders instead of
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



Eggs ("eggs")
========================
Arrows  Move around
CTRL    Fire



Elevator Action ("elevator")
============================
Runs on the same hardware as Jungle King.

Arrows  Move around
CTRL    Fire
ALT     Jump

Clones supported:
  bootleg version ("elevatob")

Known issues:
- The original version doesn't work consistently due to the copy
  protection. Use the bootleg version instead.



Eliminator ("elim2")
====================
Arrows  Left & Right rotate ship
CTRL    Fire
ALT     Thrust
A,S,D,F Second player controls



Espial ("espial")
=================



Exed Exes ("exedexes")
======================
Arrows   Move around player 1
CTRL     Fire player 1
ALT      Invulnerability player 1
W,A,D,S  Move around player 2
J        Fire player 2
K        Invulnerability player 2

Known issues:
- No background graphics.



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
Original version with Midway copyright

In the test screen, use movement & fire to change & hear sound effects

Clone supported:
  Namco copyright ("galaganm")
  Bootleg version ("galagabl")
  Another bootleg called Gallag ("gallag")



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



Ghosts 'n Goblins ("gng")
=========================
Arrows  Move around
CTRL    Fire
ALT     Jump
F2      Test mode

Clones supported:
  alternate version with different graphics (the last weapon is a cross
    instead of a shield) ("gngcross")

Known issues:
- To continue a game, insert a coin and keep CTRL pressed while pressing 1 (or 2)



Gorf ("gorf")
=============
Arrows  Move around
CTRL    Fire

Known issues:
- Speed of invaders/galaxians/scrolling text is too fast.
- You can crash into the background (i.e. on the warp screen)



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



Gravitar ("gravitar")
=====================
Arrows  Left & Right rotate ship, Up trust
CTRL    Fire
ALT     Shield



Gyruss ("gyruss")
===================
Arrows  Move around
CTRL    Fire



Journey ("journey")
===================
MCR/II hardware with MCR/III video
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire

Known issues:
- No support for the cassette music



Joust ("joust")
====================
Arrows  Move around
CTRL    Flap
F,G     Player 2 move
S       Player 2 flap
5       Goes into game setup menu
6       Diagnostic menu



Jr. Pac Man ("jrpacman")
========================
Arrows  Move around
CTRL    Enable speed cheat
F1      Skip level
F2      Test mode

Known issues:
- Maybe decryption is not correct yet.  For example in the third intermission
  the background is quite black and a line appear on left.  I'm not sure
  about this.
- I don't know what a dip switch does.



Jump Bug ("jumpbug")
====================
Arrows  Move around
CTRL    Fire
ALT     Jump

Clones supported:
  "Sega" version (probably a bootleg) ("jbugsega")



Jungle King ("junglek")
=======================
Arrows  Move around
CTRL    Jump

Clones supported:
  Another official version called Jungle Hunt ("jhunt")

Known issues:
- Sound support implemented.  Tarzan yell sample missing.  Is the pitch of
  sounds correct?



Kangaroo ("kangaroo")
=====================
Arrows  Move around
CTRL    Punch



Kick ("kick")
=============
MCR/I hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Kick

This game supports the trackball (-trak option)



Kick Rider ("kickridr")
=======================
Arrows  Move around
CTRL    Kick



Kosmik Krooz'r ("kroozr")
==========================
MCR/II hardware
Press F2 before initial screen to configure

Arrows  Move
Track   Aim
X/Z     Aim
CTRL    Fire
ALT     Shields

Known issues:
- Movement should be via an analog joystick



Krull ("krull")
========================
Runs on the same hardware as Q*Bert

Arrows  Move around
E,S,D,F Firing joystick
F1      Test mode
F2      Select



Kung Fu Master ("kungfum")
==========================
Arrows  Move around
Ctrl    Kick
Alt     Punch
F1+F3   Test mode

Clones supported:
  bootleg version copyright by O.K. ("kungfub")



Lady Bug ("ladybug")
====================



Loco-Motion ("locomotn")
====================
Arrows  Move around
CTRL    Accelerate

Known issues:
- What do the dip switches do?



Lost Tomb ("losttomb")
======================
This runs on a Super Cobra hardware.

Arrows   Move around
CTRL     Whip
E,S,D,F  Fire

Known issues:
- I don't know where coin counter is located.  The game requires 4 credits
  to start



Lunar Lander ("llander")
========================
Arrows  Left and Right to rotate
CTRL	Thrust
ALT     Maximum trust
A       Abort



Lunar Rescue ("lrescue")
========================
Arrows  Move around
CTRL    Fire
T       Activate Tilt

Clones supported:
  Destination Earth ("desterth") (uses Space Invaders color scheme)

Known issues:
  A free credit is awarded on the first docking of the first game.



Mad Planets ("mplanets")
========================
Runs on the same hardware as Q*Bert

Arrows  Move around
CTRL    Fire
Z,X     Dialer control
F1      Test mode
F2      Select




Mappy ("mappy")
===============
Arrows  Left and Right to move
CTRL    Open door
F1      Skip level



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



Missile Command ("missile")
===========================
Arrows  Move around
A,S,D   Left, Central, Right shot

- Trackball support (-trak option)



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
Arrows  Move around
CTRL    Fire

Clones supported:
  Different (probably earier) version called Mr. Do vs the Unicorns. It has
     significant differences in the gameplay ("douni")
  Another alternate version, differences are unknown ("docastl2")



Mr. Do! RunRun ("dorunrun")
===========================
Arrows  Move around
CTRL    Fire



Mr. Do! WildRide ("dowild")
===========================
Arrows  Move around
CTRL    Fire




Ms Pac Man ("mspacman")
=======================
Only the bootleg version is supported.

CTRL    Speed up cheat

Clones supported:
  Ms Pac Man Attacks ("mspacatk")

Known issues:
- Speed cheat doesn't work in Ms Pac Man Attacks
- In Ms Pac Man Attacks the bonus fruits move through the maze walls. This is
  normal, the original does the same.



Mysterious Stones ("mystston")
==============================
Arrows  Move around
CTRL    Fire
ALT     Kick




Naugthy Boy ("naughtyb")
========================
Arrows  Move around
CTRL    Throw the stone



Nibbler ("nibbler")
===================
Arrows  Move around
F1      Skip level

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz
- Some input bits seem to be used as debug controls - quite interesting, but
  I haven't investigated yet.



Omega Race ("omegrace")
=======================
Arrows  Left & Right rotate ship
CTRL    Fire
ALT     Thrust



Pac Man ("pacman")
==================
CTRL    Speed up cheat

Clones supported:
  Pac Man modification ("pacmod")
  Namco Pac Man ("namcopac")
  another Namco version, slightly different from the above ("pacmanjp")
  Hangly Man ("hangly")
  Puck Man ("puckman")
  Piranha ("piranha")
  Pac Man with Pac Man Plus graphics (this is NOT the real Pac Man Plus)
    ("pacplus")



Pac & Pal ("pacnpal")
=====================
Runs on the same hardware as Super Pac Man
Arrows  Move around
CTRL    Fire



Pengo ("pengo")
===============
Original version, encrypted, with "Popcorn" music.

Clones supported:
  alternate version (earlier), not encrypted, with different music and no
    slow screen draw ("pengoa")
  bootleg called Penta, encrypted ("penta")



Pepper II ("pepper2")
=====================
Runs on hardware similar to Venture.

Arrows  Move around
CTRL    Dog button
Z       Yellow button
X       Red button
C       Blue button
3+F3    Test mode



Phoenix ("phoenix")
===================
Amstar copyright

Arrows  Move around
CTRL    Fire
ALT     Barrier

Clones supported:
  Taito copyright ("phoenixt")
  T.P.N. copyright ("phoenix3")

Known issues:
- Sound is now quite similar to original.  We miss the melody yet.



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



Popeye ("popeyebl")
=============================
Bootleg version. The original version is not supported.

Arrows  Move around
CTRL    Fire
Q,W,E   Fire



Pop Flamer ("popflame")
=======================



Qix ("qix")
===========
Arrows  Move around
CTRL    Fast draw
ALT     Slow draw

While in the configuration screens:
F1      Next screen
F2      Next line
F3      Reset/Start game
F5      Up
F6      Down

Press F3 to skip the configuration screens at the beginning.

In the DIP switch menu, "CONFIGURE CMOS" lets you change the game's
settings by using the configuration screens.  Change the value to "YES"
while not in a game.  Change the configuration as desired, then re-enter
the DIP switch menu and change the value back to "NO".  Your configuration
is saved automatically and will be reloaded the next time you play.  To
leave the configuration screens, press F3.

To change the language, delete the QIX.HI file from the Qix directory
and re-run the program.

Known Issues:
 - No sound.
 - Emulation will be slow on anything less than a Pentium 133.
 - Interrupt timing should be exact now (NdMix)
 - Player 2 controls don't work right.



Q*Bert ("qbert")
================
Arrows  Move around
F1      Test mode
F2      Select

To enter your name in the high score list, use 1 or 2.

Clones supported:
  Japanese version ("qbertjp")



Q*Bert Qubes ("qbertqub")
=========================
Arrows  Move around
F1      Test mode
F2      Select

To enter your name in the high score list, use 1 or 2.

Known issues:
- you have to reset (F3) the game at first time when the empty
  supreme noser table appears, then the table will be correctly filled.
  Alternatively, you can insert a coin before this empty table appears...
- Hiscore save not supported yet.



Rally X ("rallyx")
==================
Arrows  Move around
CTRL    Smoke
F2      Test

Known issues:
- Sprites are not turned off appropriately.
- Cars are now displayed on the radar screen, but right color mapping
  must be done.
- The waveforms used by the sound hardware are not the correct ones



Rampage ("rampage")
===================
MCR/III hardware
Press F2 before initial screen to configure

Arrows  Move George around
CTRL    George Punch
ALT		George Jump
R,D,F,G	Move Lizzie around
A   	Lizzie Punch
S     	Lizzie Jump

Known issues:
- No sound (uses a 68000-based board)



Reactor ("reactor")
===================
Arrows  Move around
CTRL    Player 1 Energy
ALT     Player 1 Decoy
1       Player 2 Energy
2       Player 3 Decoy
F1      Test mode
F2      Select

Also this game supports trackball (-trak option)



Red Baron ("redbaron")
======================
Arrows  Bank left and right
CTRL    Fire

- Red Baron tries to calibrate its analog joystick at the start, so you'll
  have to move the "joystick" a bit before you can fly in all four directions.
- Trackball support (-trak option)


Rescue ("rescue")
===================
This game works on the same board of Super Cobra.  Graphic is garbled with
an address shuffling.

Arrows  Move around
CTRL    Fire

Known issues:
- The colors should be correct, however the sly is black instead of blue.
- I don't know where coins per credit selector is.  The game requires 4
  credits to start.



Robby Roto ("robby")
====================
Arrows  Move around
CTRL    Fire1
ALT     Fire2



Robotron ("robotron")
=====================
Arrows  Move around
E,S,D,F Fire
5       Goes into game setup menu
6       Diagnostic menu



Satan's Hollow ("shollow")
==========================
MCR/II hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire
ALT     Shields



Scramble ("scramble")
=====================
The video hardware is very similar to Galaxian, main differences being that
bullets are not vertical lines and the star background doesn't scroll.

Arrows  Move around
CTRL    Fire
ALT     Bomb

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?


Sea Wolf ][ ("seawolf2")
========================
Arrows  Left & Right move cross
CTRL    Fire



Seicross ("seicross")
=====================
Runs on almost the same hardware as Crazy Climber, but not exactly the same.

Arrows  Move around
CTRL    Fire (if SW7B is enabled)

Known issues:
- You must remember to setup "Debug and Demo" mode to OFF
- Setting SW7B to 1 will turn on FIRE facility
- Colors are wrong



Snap Jack ("snapjack")
======================
Runs on the same hardware as Lady Bug

Known issues:
- 2 player controls don't work.
- What do the dip switches do?



Solar Fox ("solarfox")
============================
MCR/I hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire
ALT     Speed



Son Son ("sonson")
==================



Space Duel ("spacduel")
=======================
Arrows  Left & Right rotate ship, Up trust
CTRL    Fire
ALT     Shields

- Press "2" to choose level, the game won't start otherwise.


Space Firebird ("spacefb")
==========================
Arrows  Move around
ALT     Fire
CTRL    Escape

Clones supported:
  None known

Known issues:
- Red screen "flash" when you die is not supported
- Background Starfield not supported. It is NOT a Galaxians type starfield

Bugs which aren't:
- The sprites/characters don't use transperancy in the real game. This is
  not a bug



Space Fury ("spacfury")
=======================
Arrows  Left and Right rotate ship
CTRL    Fire
ALT     Thrust



Space Invaders ("invaders")
===========================
Arrows  Move around
CTRL    Fire
T       Activate Tilt

Clones supported:
  Super Earth Invasion ("earthinv")
  Space Attack II ("spaceatt")
  Invaders Revenge ("invrvnge")
  Galaxy Wars ("galxwars")

Known issues:
  The colors used in Invaders Revenge may be wrong.



Space Invaders Deluxe ("invdelux")
==================================
Arrows  Move around
CTRL    Fire
T       Activate Tilt
N       Reset the name of the person with the highest score to MIDWAY

Preset mode:

Space Invaders Deluxe provided a way the operator could
enter his/her name each time the machine was turned on.

HIGH SCORE AND NAME DISPLAY PROGRAM

1.  Turn preset mode ON in the DIP switch screen
2.  Insert one credit
3.  Depress one player select button 'Preset Mode' will be displayed on screen
4.  Depress one player select button again to increase score until previous
    high-score is beaten.

    Note:
    If you press the two player select button you can skip the starting
    level as well.

5.  Depress fire button to start game. After all bases have been destroyed
    the alphabet will be displayed on screen. A new name may now be entered.
6.  Turn preset mode OFF in the DIP switch screen to re-establish game mode.



Space Panic ("panic")
=====================
Arrows  Move around
CTRL    Fire1
ALT     Fire2

Clones supported:
  alternate version, probably an earlier revision (it doesn't display the
    level number) ("panica")



Space Zap ("spacezap")
======================
Arrows  Move the base
CTRL    Fire1
ALT     Fire2 (Direction arrow must be pressed together)



Splat ("splat")
===============
Too many keys, use the Keyboard setup menu to see them all ;-)



Spy Hunter ("spyhunt")
======================
MCR/III hardware variant
Press F2 before initial screen to configure

UP      Accelerate
DOWN    Decelerate
LEFT    Steer left
RIGHT   Steer right
CTRL    Machine guns
ALT     Smoke screen
SPACE   Weapons van
SHIFT   Oil slick
BUTTON5 Missiles
ENTER   Toggle gearshift (note you can't start in high gear)

Known issues:
- Steering and acceleration should be via analog controls
- No indicators for which special weapons are available
- Palette seems to be slightly off
- No music (needs a 68000 emulator)



Starforce ("starforc")
======================
Arrows  Move around
CTRL    Fire




Stargate ("stargate")
=====================
Up key  Up
Dn key  Down
Lft key Reverse
ALT     Thrust
CTRL    Fire
X       Smart bomb
C       Hyperspace
V       Inviso
5       Goes into game setup menu
6       Diagnostic menu

Known issues:
- The joystick controls are strange, but Stargate had lots of buttons.. ;-)



Star Trek ("startrek")
=====================
Arrows  Turn left and right
A       Impuls
S       Warp 9, Mr. Sulu
D       Phaser
F       Photon torpedo

- Trackball support (-trak option)



Star Wars ("starwars")
======================
Arrows  Flight Yoke
CTRL    L Fire (thumb)
ALT     R Fire (thumb)
7       Self Test (keep it pressed)
5       Aux Coin  (toggles through tests)
1       L Fire (trigger)/Start game
2       R Fire (trigger)/Start game

Known issues:
- Dip switches not functional, but you can change game settings through
  the self test menus.
- Speech is not implemented.



Super Bagman ("sbagman")
========================
Runs on the same hardware as Bagman.



Super Basketball ("sbasketb")
=============================
Arrows  Move around
CTRL    Dribble
ALT     Shoot
SPACE   Pass



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



Tac/Scan ("tacscan")
====================
Arrows  Turn left & right
CTRL    Fire
ALT     Get new ship

- Trackball support (-trak option)



Tapper ("tapper")
=================
MCR/III hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Tapper



Tempest ("tempest")
===================
Arrows  Left & Right move around
CTRL    Zap
ALT     Super Zapper

- Trackball support (option -trak)



The End ("theend")
==================
This runs on a Scramble hardware.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?
- Bullets are wrong



Three Stooges ("3stooges")
==========================
IJKL    Moe Moves around
ALT     Moe Fire
ESDF    Larry Moves around
ENTER   Larry Fire
Arrows  Curly Moves around
CTRL    Curly Fire



Timber ("timber")
=================
MCR/III hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Chop left
ALT     Chop right



Time Pilot ("timeplt")
======================
Arrows  Move around
CTRL    Fire

Clones supported:
  bootleg version ("spaceplt")



Time Pilot 84 ("tp84")
======================
Known issues:
- sound is quite wrong
- some sprites are displayed with wrong colors



Tron ("tron")
=============
MCR/II hardware
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire
Track   Aim
X/Z     Aim



Turtles ("turtles")
===================
Runs on the same hardware as Amidar

Clones supported:
  Another official version called Turpin ("turpin")



Tutankham ("tutankhm")
======================
Arrows  Move around
CTRL    Fire left
ALT     Fire Right
SPACE   Flash Bomb

Known issues:
- David Dahl reported a probable missing ROM ("ra1_9a.snd"???), but the area
  2000-2fff doesn't seem to be used. Maybe different music roms are available
  since there is a place for a 3rd ROM.



Two Tigers ("twotiger")
=======================
MCR/II hardware
Press F2 before initial screen to configure

Track   Adjust pitch
X/Z     Adjust pitch
CTRL    Fire
ALT     Bomb

Known issues:
- Two players not well supported



Uniwars ("uniwars")
===================
This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Clones supported:
  The original Japanese version, Gingateikoku No Gyakushu ("japirem")

Known issues:
- The star background is probably not entirely accurate.
- What does dip switch 6 do?



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



Vulgus ("vulgus")
===================
Arrows  Move around
CTRL    Fire
ALT     Big Missile

Known issues:
- Sound speed should be accurate.  It seems not to use FM.  Anyone confirm?



Wacko ("wacko")
===============
MCR/II hardware
Press F2 before initial screen to configure

Track   Move around
Arrows  Fire

At this time, the game requires a trackball (-trak option)



War of the Bugs ("warofbug")
============================
This runs on the same hardware as Galaxian.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Warlords ("warlord")
====================
Q,W   Move player 1
I,O   Move player 2
1,2   Fire player 1/2

Known issues:
- The movement keys cannot be remapped.



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
- No background stars.



Yie Ar Kung Fu ("yiear")
========================
Arrows  Move around
CTRL    Punch
ALT     Kick



Xevious ("xevious")
===================
Atari/Namco copyright

Clones supported:
  Namco copyright ("xeviousn")
  Super Xevious ("sxevious")



Zaxxon ("zaxxon")
=================
Use F2 to enter test mode, then 1 to advance from one test to the following.

Known issues:
- What do the dip switches do?


Zektor ("zektor")
=================
Arrows  Turn left & right
CTRL    Fire
ALT     Thrust

Known issues:
- Starts with 99 lives.



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
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M68000 emulator taken from the System 16 Arcade Emulator by Thierry Lescot.
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
YM-2203 emulation by Ishmair (ishmair@vnet.es).
POKEY emulator by Ron Fries (rfries@tcmail.frco.com).
UNIX port by Allard van der Bas (avdbas@wi.leidenuniv.nl) and Dick de Ridder
  (dick@ph.tn.tudelft.nl).

Phoenix driver provided by Brad Oliver (bradman@primenet.com), Mirko
   Buffoni (mix@lim.dsi.unimi.it) and Richard Davies (R.Davies@dcs.hull.ac.uk)
1942, Commando, Elevator Action, Jr. Pacman, Turpin and Galaga drivers by
   Nicola Salmoria.
Gyruss, Ghost 'n Goblin, Mario Bros., Vulgus, Zaxxon, Bomb Jack, Burger Time
   and Donkey Kong 3 drivers provided by Mirko Buffoni (mix@lim.dsi.unimi.it)
Tutankhamon driver provided by Mirko Buffoni and David Dahl (techt@juno.com)
Bomb Jack sound driver by Jarek Burczynski (pbk01@ikp.atm.com.pl).
Arabian driver provided by Jarek Burczynski (pbk01@ikp.atm.com.pl).
Congo Bongo and Kangaroo drivers provided by Ville Laitinen (ville@sms.fi).
Millipede driver provided by Ivan Mackintosh (ivan@rcp.co.uk).
Donkey Kong sound emulation by Ron Fries (rfries@tcmail.frco.com).
Thanks to John Butler (jcbutler@worldnet.att.net) and Ed. Mueller
  (emueller@ti.com) for the Qix driver.
Vanguard driver by Brad Oliver and Mirko Buffoni, based on code by Brian
   Levine.
Carnival driver completed by Mike Coates and Richard Davies.
Warp warp driver completed by Chris Hardy (chrish@kcbbs.gen.nz).
Popeye driver provided by Marc LaFontaine and Nicola Salmoria.
Stargate, Joust, Robotron and generic Williams drivers provided by
   Steven Hugg <hugg@pobox.com>
Jump Bug driver by Richard Davies (R.Davies@dcs.hull.ac.uk) and Brad Oliver
   (bradman@primenet.com).
Venture, Mouse Trap and Pepper II drivers by Marc Lafontaine
   (marclaf@sympatico.ca).
Q*Bert, Q*Bert Qubes, Mad Planets, Reactor and Krull drivers by
   Fabrice Frances (frances@ensica.fr)
Robotron, Splat, Defender, Joust, Stargate, Bubbles, Blaster and Sinistar
   drivers by Marc LaFontaine (marclaf@sympatico.ca).  Robotron, Joust and
   Stargate drivers also provided by Steven Hugg (hugg@pobox.com).
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
Colors log file for Ghosts 'n Goblins provided by Gabrio Secco.
Colors for Donkey Kong, Donkey Kong Jr. and Mario Bros. derived from Kong
   emulator by Gary Shepherdson.
Colors for Amidar, Frogger and Zaxxon derived from SPARCADE by Dave Spicer.
Colors for Space Invaders, Lunar Rescue and clones and high score saving
    for Lunar Rescue provided by Valerio Verrando (v.verrando@mclink.it)
Thanks to Brad Oliver, Marc Vergoossen (marc.vergoossen@pi.net) and Richard
   Davies (R.Davies@dcs.hull.ac.uk) for help with Donky Kong Jr. colors.
Thanks to Marc Vergoossen and Marc Lafontaine (marclaf@sympatico.ca) for
   Zaxxon colors.
Thanks to Marc Lafontaine for Congo Bongo colors and Popeye bootleg.
Centipede information taken from Centipede emulator by Ivan Mackintosh, MageX
   0.3 by Edward Massey and memory map by Pete Rittwage.
Info on Burger Time taken from Replay 0.01a by Kevin Brisley (kevin@isgtec.com)
Thanks to Tormod Tjaberg (tormod@sn.no) for Invaders saga modifications.
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
Vesa & rotate modes by Bernd Wiebelt (bernardo@studi.mathematik.hu-berlin.de)
Thanks to Stefano Mozzi (piu1608@cdc8g5.cdc.polimi.it) for Mario Bros. colors.
Thanks to Matthew Hillmer (mhillmer@pop.isd.net) for Donkey Kong 3 colors.
Thanks to Tormod Tjaberg (tormod@sn.no) and Michael Strutts for Space Invaders
   sound.
Thanks to Shaun Stephenson (shaun@marino13.demon.co.uk) for Phoenix samples.
Thanks to Maurizio Zanello for a better interface between MAME and frontends.
Thanks to Al Kossow (aek@spies.com) and Brad Oliver (bradman@primenet.com) for
   the Sega Vector drivers.
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
Star Wars driver by Steve Baines, Frank Palazzolo and Brad Oliver.



Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -soundcard 0    will run Ms Pac Man without sound

options:
-noscanlines  use alternate video mode (not available in all games). Use this
              if the default mode doesn't work with your monitor/video card.
              in conjunction with VESA modes, things get rather SLOW
-vesa         use a standard VESA 1.2 mode (default 800x600) instead of custom
              video mode. By default this simulates scanlines unless the
              above option ('-noscanlines') is given, or the game is too wide
              to fit on the screen.
              You probably want to try this if the custom modes don't work
              correct for you.  Use PGUP/PGDN to scroll the game up/down
              if the game is too tall to fit entirely on the screen.
-vesa2b       same as '-vesa' except this uses VESA2.0 banked mode extension
-vesa2l       same as '-vesa' except this uses VESA2.0 linear mode extension
              (recommended if available since it is faster than VESA 1.2
-XxY          where X and Y are width and height (ex: '-800x600')
              This is the preferred method for selecting the
              VESA resolution. For possible X/Y combinations see below
-320          tell MAME to use a vesa 320x240 video mode
              If you get an error '320x240 not supported', you probably
              need Scitech's Display Doctor, which provides the 'de facto'
              standard VESA implementaion (http://www.scitechsoft.com)
              Note: this is a nice alternative to '-640x480 -noscanlines'
-512          same as above, video mode is 512x384
-640          same as above, video mode is 640x480
-800          same as above, video mode is 800x600.
              If no VESA resolution is given on the commandline, this
              is the default mode. Since rotation modes (below) use VESA,
              this is also the default mode for them.
-1024         same as above, video mode is 1024x768
-vesascan     is obsolte and no longer supported.
              frontend authors: use '-vesa -800'.
-vesaskip N   is obsolete and no longer supported
              use '-640x480 -skiplines N' to get the old behaviour
-skiplines N  since most games use a screen taller than 240 lines,
              they won't fit in the screen. The parameter 'N' sets
              the initial number of lines to skip at the top of
              the screen. You can adjust the position while the game
              is running using the PGUP and PGDOWN keys.
              Note: this works _only_ if a VESA mode is selected,
                    e.g. ('-640x480 -skiplines 10')
-nodouble     prevents pixel doubling, if you like miniaturised
              arcade emulation. It's also faster than the standard
              pixel-doubling VESA modes.
-vgafreq n    where n can be 0 (default) 1, 2 or 3.
              use different frequencies for the custom video modes. This
              could reduce flicker, especially in the 224x288noscanlines
              mode. WARNING: THE FREQUENCIES USED MIGHT BE WAY OUTSIDE OF
              YOUR MONITOR RANGE, AND COULD EVEN DAMAGE IT. USE THESE OPTIONS
              AT YOUR OWN RISK.
-vsync        syncronize video display with the video beam instead of using
              the timer. This works best with -noscanlines and the -vesaxxx
              modes. Use F11 to check your actual speed - it should be 100%.
              If it is lower, try to increase it with -vgafreq (if you are
			  using a tweaked video mode) or use your video board utilities
			  to set the VESA refresh rate to 60 Hz.
			  Note that when this option is turned on, speed will NOT
              downgrade nicely if your system is not fast enough.
-rotate       is obsolete and no longer supported
-ror          rotate the display clockwise by 90 degrees.
-rol          rotate display anticlockwise
-flipx        flip display horizontally
-flipy        flip display vertically
              -ror and -rol provide authentic *vertical* scanlines, given you
			  turn your monitor to its side.
              CAUTION:
              A monitor is a complicated, high voltage electronic device.
              There are some monitors which were designed to be rotated.
              If yours is _not_ one of those, but you absolutely must
              turn it to its side, you do so at your own risk.

              ******************************************************
              PLEASE DO NOT LET YOUR MONITOR WITHOUT ATTENTION IF IT
              IS PLUGGED IN AND TURNED TO ITS SIDE
              ******************************************************

-soundcard n  select sound card (if this is not specified, you will be asked
              interactively)
-nojoy        don't poll joystick
-nofm         suppress FM support.  If you have problems and don't have a
              soundblaster I suggest you use this option
-log          create a log of illegal memory accesses in ERROR.LOG
-help, -?     display current mame version and copyright notice
-list         display a list of currently supported games
-listfull     display a list of game directory names + description
-listroms     display selected game required roms
-listsamples  display selected game required samples
-mouse        enable mouse support
-trak         enable trackball support
-frameskip n  skip frames to speed up the emulation. For example, if the game
              normally runs at 60 fps, "-frameskip 1" will make it run at 30
              fps, and "-frameskip 2" at 20 fps. Use F11 to check the speed
			  your computer is actually reaching. If the game is too slow,
              increase the frameskip value. Note that this setting can also
              affect audio quality (some games sound better, others sound
              worse).  Maximum value for frameskip is 3.
-vg           changes the resolution used by vector games. For example,
              mame tempest -vesa2l -800 -vg
              will use an 800x600 VESA2.0 linear screen mode and stretch the
              vector game to fit the whole screen, instead of using the
              default dimensions.
-cheat        Cheats like the speedup in Pac Man or the level skip in many
              other games are disabled by default. Use this switch to turn
              them on.


The following keys work in all emulators:

3         Insert coin
1         Start 1 player game
2         Start 2 players game
Tab       Enter dip switch, keys and joy settings, and credits display menu
          Pressing TAB again will back you to the emulator, ESC exit from MAME.
P         Pause
F3        Reset
F4        Show the game graphics. Use cursor keys to change set/color, F4 to exit
F8        Change frame skip on the fly (60, 30, 20, or 15)
F9        To change volume percentage thru 100,75,50,25,0 values
          Keypad PLUS and MINUS change the volume with fine granularity
F10       Toggle speed throttling
F11       Toggle speed display
F12       Save a screen snapshot
ESC       Exit emulator
