
              M.A.M.E.  -  Multiple Arcade Machine Emulator
       Copyright (C) 1997, 1998  by Nicola Salmoria and the MAME team

Please note that many people helped with this project, either directly or by
releasing source code which was used to write the drivers. We are not trying to
appropriate merit which isn't ours. See the acknowledgemnts section for a list
of contributors, however please note that the list is largely incomplete. See
also the CREDITS section in the emulator to see the people who contributed to a
specific driver. Again, that list might be incomplete. We apologize in advance
for any omission.

All trademarks cited in this document are property of their respective owners.


Usage and Distribution Licence
------------------------------

I. Purpose
----------
   MAME is strictly a no profit project. Its main purpose is to be a reference
   to the inner workings of the emulated arcade machines. This is done for
   educational purposes and to preserve many historical games from the oblivion
   they would sink into when the hardware they run on will stop working.
   Of course to preserve the games you must also be able to actually play them;
   you can see that as a nice side effect.
   It is not our intention to infringe any copyrights or patents pending on the
   original games. All of the source code is either our own or freely
   available. To work, the emulator requires ROMs of the original arcade
   machines, which must be provided by the user. No portion of the code of the
   original ROMs is included in the executable.

II. Cost
--------
   MAME is free. The source code is free. Selling it is not allowed.

III. ROM Images
---------------
   You are not allowed to distribute MAME and ROM images on the same physical
   medium. You ARE allowed to make them available for download on the same web
   site, but only if you warn users about the copyright status of the ROMs and
   the legal issues involved. You are NOT allowed to make MAME available for
   download together with one giant big file containing all of the supported
   ROMs, or any files containing more than one ROM set each.
   You are not allowed to distribute MAME in any form if you sell, advertise or
   publicize illegal CD-ROMs or other media containing ROM images. Note that
   the restriction applies even if you don't directly make money out of that.
   The restriction of course does not apply if the CD-ROMs are published by the
   ROMs copyrights owners.

IV. Distribution Integrity
--------------------------
   MAME must be distributed only in the original archives. You are not allowed
   to distribute a modified version, nor to remove and/or add files to the
   archive. Adding one text file to advertise your web site is tolerated only
   if your site contributes original material to the emulation scene.

V. Source Code Distribution
---------------------------
   If you distribute the binary, you should also distribute the source code. If
   you can't do that, you must provide a pointer to a place where the source
   can be obtained.

VI. Reuse of Source Code
-------------------------
   This chapter might not apply to specific portions of MAME (e.g. CPU
   emulators) which bear different copyright notices.
   The source code cannot be used in a commercial product without a written
   authorization of the authors. Use in non commercial products is allowed and
   indeed encouraged; however if you use portions of the MAME source code in
   your program, you must make the full source code freely available as well.
   Derivative works are allowed (provided source code is available), but
   discouraged: MAME is a project continuously evolving, and you should, in
   your best interest, submit your contributions to the development team, so
   that they are integrated in the main distribution. Usage of the
   _information_ contained in the source code is free for any use. However,
   given the amount of time and energy it took to collect this information, we
   would appreciate if you made the additional information you might have
   freely available as well.




How to Contact Us
-----------------

Here are some of the people contributing to MAME. If you have comments,
suggestions or bug reports about an existing driver, check the driver's Credits
section to find who has worked on it, and send comments to that person. If you
are not sure who to contact, write to Mirko or Nicola. If you have comments
specific to a system other than DOS (e.g. Mac, Win32, Unix) should be sent to
the respective port maintainer. Don't send them to Mirko or Nicola - they will
be ignored.

You can send source code to mame@lim.dsi.unimi.it. Do not use this address for
non-source code bug reports.

Nicola Salmoria    MC6489@mclink.it
Mirko Buffoni      mix@lim.dsi.unimi.it

Mike Balfour       mab22@po.cwru.edu
Aaron Giles        agiles@sirius.com
Chris Moore        chris.moore@writeme.com
Brad Oliver        bradman@primenet.com
Andrew Scott       ascott@utkux.utcc.utk.edu
Martin Scragg      mnm@onaustralia.com.au
Zsolt Vasvari      shakemejazz@aol.com
Bernd Wiebelt      bernardo@studi.mathematik.hu-berlin.de

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have (e.g. http://208.8.142.105/mame/), or ask on the appropriate Usenet
newsgroups (e.g. comp.emulators.misc) or on the emulation message boards (e.g.
http://www.darkening.com/dave/cgi/MessageBoards/MAME/mameboard.html).



Supported Games
---------------

Here is a quick list of the currently supported games; read on for details.
The list doesn't include variants of the same game.


================================================================================
|                      |        |Accurate|        |Hi score|Cocktail|Directory |
| Game Name            |Playable| colors | Sound  |  save  |  mode  |   Name   |
================================================================================
| 005                  |  Yes   |  Yes   |   No   |  Yes   |  Yes   | 005      |
|----------------------|--------|--------|--------|--------|--------|----------|
| 10 Yard Fight        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | yard     |
|----------------------|--------|--------|--------|--------|--------|----------|
| 1942                 |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | 1942     |
|----------------------|--------|--------|--------|--------|--------|----------|
| 1943                 |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | 1943     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Amidar               |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | amidar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Anteater             |  Yes   |   No   |  Yes   |  Yes   |   No   | anteater |
|----------------------|--------|--------|--------|--------|--------|----------|
| Arabian              |  Yes   | Close  |  Yes   |  Yes   |   No   | arabian  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Armored Car          |  Yes   |   No   |  Yes   |  Yes   |   No   | armorcar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Arkanoid             |   No   |  Yes   |   No   |   No   |   No   | arkanoid |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids            |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | asteroid |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids Deluxe     |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | astdelux |
|----------------------|--------|--------|--------|--------|--------|----------|
| Astro Blaster        |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | astrob   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Astro Fighter        |  Yes   |   No   |   No   |   No   |   No   | astrof   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bagman               |  Yes   |  Yes   | Music  |  Yes   |  Yes   | bagman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bank Panic           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | bankp    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle of Atlantis   |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | atlantis |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle Zone          |  Yes   | Close  | Yes(1) |  Yes   |   No   | bzone    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Berzerk              |  Yes   |  Yes   |   No   |  Yes   |   No   | berzerk  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Black Tiger          |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | blktiger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Black Widow          |  Yes   | Close  |  Yes   |   No   |   No   | bwidow   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blaster              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | blaster  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blue Print           |  Yes   |   No   |  Yes   |  Yes   |  Yes   | blueprnt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bomb Jack            |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | bombjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bosconian            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bosco    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Break Thru           |  Yes   |  Yes   |   No   |   No   |  Yes   | brkthru  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubble Bobble        |  Yes   |  Yes   | Partial|  Yes   |   No   | bublbobl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubbles              |   No   |  Yes   |  Yes   |  Yes   |   No   | bubbles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bump 'n Jump         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bnj      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Burger Time          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | btime    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Carnival             |  Yes   |  Yes   |Partl(1)|  Yes   |   No   | carnival |
|----------------------|--------|--------|--------|--------|--------|----------|
| Centipede            |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | centiped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Champion Baseball    |  Yes   |  Yes?  |  Yes   |   No   |   No   | champbas |
|----------------------|--------|--------|--------|--------|--------|----------|
| Circus               |  Yes   | Close  |Partial |  Yes   |   No   | circus   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Circus Charlie       |  Yes   |   No   |  Yes   |  Yes   |  Yes   | circusc  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cloak & Dagger       |  Yes   |  Yes   |  Yes   |   No   |   No   | cloak    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Colony 7             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | colony7  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Commando             |  Yes   |  Yes   | Yes(2) |  Yes   |  Yes   | commando |
|----------------------|--------|--------|--------|--------|--------|----------|
| Congo Bongo          |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | congo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Coors Light Bowling  |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | capbowl  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cosmic Avenger       |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | cavenger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Climber        |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | cclimber |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Kong           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ckong    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crystal Castles      |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ccastles |
|----------------------|--------|--------|--------|--------|--------|----------|
| Defender             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | defender |
|----------------------|--------|--------|--------|--------|--------|----------|
| Demolition Derby     |  Yes   |  Yes   |   No   |   No   |   No   | destderb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Diamond Run          |  Yes   |  Yes   | Yes(2) |  Yes   |  Yes   | diamond  |
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
| Donkey Kong Jr.      |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | dkongjr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong 3        |  Yes   |  Yes   |   No   |  Yes   |   No   | dkong3   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eggs                 |  Yes   |   No   |  Yes   |  Yes   |   No   | eggs     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exerion              |  Yes   |   No   |  Yes   |  Yes   |   No   | exerion  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Elevator Action      |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | elevator |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eliminator           |  Yes   | Close  | Yes(1) |  Yes   |   No   | elim2    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Espial               |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | espial   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exed Exes            |  Yes   |  Yes   |Partial |  Yes   |   No   | exedexes |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exerion              |  Yes   |   No   |  Yes   |   No   |   No   | exerion  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fantasy              |  Yes   |  Yes   |  Yes   |   No   |  Yes   | fantasy  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fire Trap            |  Yes   |  Yes   |   No   |   No   |  Yes   | firetrap |
|----------------------|--------|--------|--------|--------|--------|----------|
| Food Fight           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | foodf    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frenzy               |  Yes   |  Yes   |   No   |   No   |   No   | frenzy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frogger              |  Yes   | Close  |  Yes   |  Yes   |   No   | frogger  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Front Line           |Partial.|  Yes   |  Yes   |   No   |  Yes   | frontlin |
|----------------------|--------|--------|--------|--------|--------|----------|
| Future Spy           |   No   |   No   |   No   |   No   |   No   | futspy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaga               |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | galaga   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaxian             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | galaxian |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gauntlet             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | gauntlet |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gauntlet 2           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | gaunt2   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ghosts 'n Goblins    |  Yes   |  Yes   | Yes(2) |  Yes   |  Yes   | gng      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gorf                 |  Yes   |   No   |   No   |  Yes   |   No   | gorf     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gravitar             |  Yes   | Close  |  Yes   |  Yes   |   No   | gravitar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Green Beret          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | gberet   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gunsmoke             |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | gunsmoke |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gyruss               |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | gyruss   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Hunchback            |   No   |  n/a   |   No   |  n/a   |   No   | hunchy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Hyper Sports         |  Yes   |   No   |No Speech  Yes   |  Yes   | hyperspt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Invinco              |  Yes   |  Yes?  |   No   |   No   |   No   | invinco  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Invinco / Head On 2  |  Yes   |  Yes?  |   No   |   No   |   No   | invho2   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Iron Horse           |  Yes   |   No   |  Yes   |   No   |   No   | ironhors |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jr. Pacman           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | jrpacman |
|----------------------|--------|--------|--------|--------|--------|----------|
| Journey              |  Yes   |  Yes   |Partial.|  Yes   |   No   | journey  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Joust                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | joust    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jump Bug             |  Yes   |   No   |  Yes   |  Yes   |   No   | jumpbug  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jumping Jack         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | jjack    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jungle King          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | junglek  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kangaroo             |  Yes   |   No   |  Yes   |   No   |   No   | kangaroo |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kick     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick Rider           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kickridr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kosmik Krooz'r       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kroozr   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Krull                |  Yes   |  Yes   |  Yes   |  Yes   |  n/a   | krull    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kung Fu Master       |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | kungfum  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lady Bug             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ladybug  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Legendary Wings      |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | lwings   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Loco-Motion          |  Yes   |   No   |  Yes   |  Yes   |   No   | locomotn |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lost Tomb            |  Yes   |   No   |  Yes   |   No   |   No   | losttomb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lunar Lander         |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | llander  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lunar Rescue         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | lrescue  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mad Planets          |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | mplanets |
|----------------------|--------|--------|--------|--------|--------|----------|
| Major Havoc          |  Yes   |  Yes   |  Yes   |   No   |   No   | mhavoc   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Make Trax            |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | maketrax |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mappy                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mappy    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mario Bros.          |  Yes   |  Yes   |Partl(1)|  Yes   |   No   | mario    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mat Mania            |  Yes   |  Yes?  |No speech  Yes   |   No   | matmania |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mikie                |  Yes   |  Yes?  |  Yes   |   No   |   No   | mikie    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Millipede            |  Yes   |   No   |   No   |  Yes   |   No   | milliped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Minefield            |  Yes   |   No   |  Yes   |   No   |   No   | minefld  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Missile Command      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | missile  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Monster Bash         |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | monsterb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Cresta          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mooncrst |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Patrol          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mpatrol  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Quasar          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | moonqsr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Motos                |  Yes   |   No   |  Yes   |  Yes   |   No   | motos    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mouse Trap           |  Yes   |   No   |   No   |  Yes   |   No   | mtrap    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mr. Do!              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | mrdo     |
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
| Naughty Boy          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | naughtyb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Nibbler              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | nibbler  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Omega Race           |  Yes   |  Yes   |   No   |  Yes   |   No   | omegrace |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac Man              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pacman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac & Pal            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | pacnpal  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pengo                |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pengo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pepper II            |  Yes   |   No   |   No   |  Yes   |   No   | pepper2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Phoenix              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | phoenix  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pinball Action       |  Yes   |  Yes   |   No   |   No   |   No   | pbaction |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pisces               |  Yes   |  Yes   |   Yes  |  Yes   |   No   | pisces   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pleiads              |  Yes   |  Yes?  | Limited|  Yes   |   No   | pleiads  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pooyan               |  Yes   |   No   |  Yes   |  Yes   |  Yes   | pooyan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Popeye               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | popeyebl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pop Flamer           |  Yes   |  Yes   |  Yes   |   No   |   No   | popflame |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pulsar               |  Yes   |  Yes?  |   No   |   No   |   No   | pulsar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Qix                  |  Yes   |  Yes   |  Yes   |  Yes   |   No   | qix      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Quantum              |  Yes   |   No   |  Yes   |  Yes   |   No   | quantum  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert               |  Yes   |  Yes   |Partial |  Yes   |   No   | qbert    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert Qubes         |  Yes   |  Yes   |Partial |  Yes   |   No   | qbertqub |
|----------------------|--------|--------|--------|--------|--------|----------|
| Radar Scope          |  Yes   |   No   |Partial |   No   |   No   | radarscp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rally X              |  Yes   |   No   | Yes(1) |  Yes   |   No   | rallyx   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rampage              |  Yes   |  Yes   |  Yes   |   No   |   No   | rampage  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rastan               |  Yes   | Close  |  Yes   |   No   |   No   | rastan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Red Baron            |  Yes   | Close  | Yes(1) |  Yes   |   No   | redbaron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Reactor              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | reactor  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rescue               |  Yes   |   No   |  Yes   |   No   |   No   | rescue   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robot Bowl           |  Yes   |  Yes   |   No   |   No   |   No   | robotbwl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robotron             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | robotron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Roc'n Rope           |  Yes   | Close  |  Yes   |  Yes   |  Yes   | rocnrope |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sarge                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | sarge    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Satan's Hollow       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | shollow  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Scramble             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | scramble |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sea Wolf ][          |  Yes   |   No   |   No   |  Yes   |   No   | seawolf2 |
|----------------------|--------|--------|--------|--------|--------|----------|
| Section Z            |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | sectionz |
|----------------------|--------|--------|--------|--------|--------|----------|
| Seicross             |  Yes   |  Yes   |  Yes   |   No   |   No   | seicross |
|----------------------|--------|--------|--------|--------|--------|----------|
| Shao-Lin's Road      |  Yes   |   No   |   No   |   No   |   No   | shaolins |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sidearms             |  Yes   |  Yes   |Partial |   No   |   No   | sidearms |
|----------------------|--------|--------|--------|--------|--------|----------|
| Silkworm             |  Yes   |  Yes   |   No   |   No   |   No   | silkworm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sinistar             |   No   |  Yes   |  Yes   |  Yes   |   No   | sinistar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Snap Jack            |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | snapjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Solar Fox            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | solarfox |
|----------------------|--------|--------|--------|--------|--------|----------|
| Son Son              |  Yes   |  Yes   |  Yes   |  Yes   |  n/a   | sonson   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Attack         |  Yes   |   No   |   No   |   No   |   No   | sspaceat |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Duel           |  Yes   | Close  |  Yes   |  Yes   |   No   | spacduel |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Firebird       |  Yes   |  Yes   |   No   |   No   |   No   | spacefb  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Fury           |  Yes   | Close  | Yes(1) |  Yes   |   No   | spacfury |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Invaders       |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | invaders |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Odyssey        |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | spaceod  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Panic          |  Yes   | Close  |   No   |  Yes   |   No   | panic    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Zap            |  Yes   | Maybe  |   No   |  Yes   |   No   | spacezap |
|----------------------|--------|--------|--------|--------|--------|----------|
| Spectar              |  Yes   |  Yes   |   No   |   No   |   No   | spectar  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Splat                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | splat    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sprint 2             |  Yes   | Maybe  |   No   |  n/a   |  n/a   | sprint2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Spy Hunter           |  Yes   | Maybe  |  Yes   |  Yes   |   No   | spyhunt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Starforce            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | starforc |
|----------------------|--------|--------|--------|--------|--------|----------|
| Stargate             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | stargate |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Trek            |  Yes   | Close  | Yes(1) |  Yes   |   No   | startrek |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Wars            |  Yes   | Close  |  Yes   |   No   |   No   | starwars |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Bagman         |  Yes   |  Yes   | Music  |  Yes   |  Yes   | sbagman  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Basketball     |  Yes   |  Yes   |No Speech   No   |   No   | sbasketb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Cobra          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | scobra   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super PacMan         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | superpac |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Zaxxon         |  Yes   |  Yes   |   No   |  Yes   |   No   | szaxxon  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Swimmer              |  Yes   |   No   |  Yes   |   No   |  Yes   | swimmer  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tac/Scan             |  Yes   | Close  | Yes(1) |  Yes   |   No   | tacscan  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tapper               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tapper   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Targ                 |  Yes   |  Yes   |   No   |   No   |   No   | targ     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tempest              |  Yes   | Close  |  Yes   |  Yes   |   No   | tempest  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Track & Field        |  Yes   |  Yes   |No Speech  Yes   |   No   | hyperspt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Traverse USA         |  Yes   |   No   |  Yes   |  Yes   |   No   | travrusa |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robby Roto           |  Yes   |   No   |   No   |  Yes   |   No   | robby    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Targ                 |  Yes   | Close  |   No   |   No   |   No   | targ     |
|----------------------|--------|--------|--------|--------|--------|----------|
| The End              |  Yes   |   No   |  Yes   |  Yes   |   No   | theend   |
|----------------------|--------|--------|--------|--------|--------|----------|
| The Tower of Druaga  |  Yes   |  Yes   |  Yes   |  Yes   |   No   | todruaga |
|----------------------|--------|--------|--------|--------|--------|----------|
| Three Stooges        |  Yes   |  Yes   |Partial |  Yes   |   No   | 3stooges |
|----------------------|--------|--------|--------|--------|--------|----------|
| Timber               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | timber   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | timeplt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot 84        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tp84     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Track & Field        |  Yes   |  Yes   |No Speech  Yes   |  Yes   | trackfld |
|----------------------|--------|--------|--------|--------|--------|----------|
| Traverse USA         |  Yes   | Close  |  Yes   |  Yes   |   No   | travrusa |
|----------------------|--------|--------|--------|--------|--------|----------|
| Triple Punch         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | triplep  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tron                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tron     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Turtles              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | turtles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tutankham            |  Yes   |  Yes   |  Yes   |   No   |   No   | tutankhm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Two Tigers           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | twotiger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Uniwars              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | uniwars  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vanguard             |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | vanguard |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vastar               |  Yes   |   No   |  Yes   |   No   |   No   | vastar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Venture              |  Yes   |   No   |   No   |  Yes   |   No   | venture  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vulgus               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | vulgus   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wacko                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | wacko    |
|----------------------|--------|--------|--------|--------|--------|----------|
| War of the Bugs      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | warofbug |
|----------------------|--------|--------|--------|--------|--------|----------|
| Warlords             |  Yes   |   No   |  Yes   |   No   |   No   | warlord  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Warp Warp            |  Yes   |   No   |   No   |  Yes   |   No   | warpwarp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wild Western         |   No   |  Yes   |  Yes   |  n/a   |  Yes   | wwestern |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wizard of Wor        |  Yes   | Maybe  |   No   |  Yes   |   No   | wow      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Yie Ar Kung Fu       |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | yiear    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Xevious              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | xevious  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zaxxon               |  Yes   |   No   |   No   |  Yes   |   No   | zaxxon   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zektor               |  Yes   | Close  |Part(1).|  Yes   |   No   | zektor   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zoo Keeper           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | zookeep  |
|----------------------|--------|--------|--------|--------|--------|----------|

(1) Needs samples provided separately
(2) Use the -fm option for OPL emulation instead of digital emulation



10 Yard Fight ("yard")
======================
Clones supported:
  Vs. version ("vsyard")

Known issues:
- Vs. mode runs only in cocktail mode (second player controls inverted).
  That's probably how the game is designed.



1943 ("1943")
=============
Clones supported:
  1943 Kai ("1943kai")

Known issues:
- Colors in test mode are not correct, but the board the PROMs were read from
  does the same. The colors in the game seem to be correct.
- To be able to access items 4-7 in the test menu, you must keep 3 pressed
  while pressing F3.



Amidar ("amidar")
=================
Clones supported:
  Japanese version ("amidarjp"). This version has a worse attract mode and
                                 does not display the number of jumps left.
  bootleg version called Amigo ("amigo")
  version running on Scrtamble hardware ("amidars")

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



Arkanoid ("arkanoid")
=====================
Not playable

Clones supported:
  US version ("arknoidu")



Armored Car ("armorcar")
========================
Press the second player button #2 (default: S) + F3 to enter test mode



Asteroids ("asteroid")
======================
Clones supported:
  alternate version ("asteroi2")
  Asteroids Deluxe ("astdelux")



Astro Blaster ("astrob")
========================
Clones supported:
  version 1 ("astrob1")
To quote from Clay Cowgill's page ( http://wwwpro.com/clay/roms_stuff.html )
about the version 1 ROMs:

This ROM set appears to be the "original" Astro Blaster ROMs. There are no
on-screen instructions and the progression of waves is very difficult. I
believe they were replaced by version 2 (below) to make the game a little
easier. The game behaves a little differently than version 2 when starting
up-- the more coins you put in, the more ships you get to start with.



Astro Fighter ("astrof")
========================
Clones supported:
  alternate version ("astrof2")



Bagman ("bagman")
=================
Known issues:
- Guards easily get stuck in corners and disappear after a while. This
  doesn't happen in the original and is probably due to copy protection,
  probably read at location A000.



Battlezone ("bzone")
====================
Clones supported:
  alternate version ("bzone2")

There are two control methods:
  1) E,I,D,K,Space: Close to the original controls (two up/down sticks)
  2) Arrow keys+CTRL: simulates a 4-way joystick

Known issues:
- The hardware itself produces red and green vectors. However (to save
  costs?) the original monitor was a white one and had a red/green
  overlay.
  This is implemented, but only line endpoints are checked, so you'll
  sometimes see incorrectly colored lines.



Berzerk ("berzerk")
===================
Clones supported:
  earlier, very rare version ("berzerk1")



Black Tiger ("blktiger")
========================
Clones supported:
  Black Dragon ("blkdrgon")

Known issues:
- Colors are accurate, but the color space is downgraded from 4x4x4
  to 3x3x2.



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
Midway copyright

Arrows  Move around
CTRL    Fire

Clones supported:
  Namco copyright ("bosconm")

Known issues:
- some of the dip switches for bosconm are wrong.



Break Thru ("brkthru")
======================
Keep 1 & 2 pressed and hit F3 to enter test mode, then hit 3 to proceed
through the tests.



Bubble Bobble ("bublbobl")
==========================
Arrows  Move around
ALT     Jump
CTRL    Fire

Clones supported:
  bootleg version called "Bobble Bobble" ("boblbobl")
  another bootleg, which has a dip switch setting for Super Bobble Bobble
    ("sboblbob")

Known issues:
- Currently, the original version doesn't work at all due to the copy
  protection. Use the bootleg instead.



Bubbles ("bubbles")
===================
Clones supported:
  "red label" version ("bubblesr")


Bump 'n Jump ("bnj")
====================
In test mode, press 3 to advance through the various tests.

Clones supported:
  Burnin' Rubber ("brubber") the service ROM is missing for this one so
    service mode doesn't work.

Known issues:
- The continue timer runs too fast, this is related to the VBlank input bit
  timing.



Burger Time ("btime")
=====================
In test mode, press 3 to advance through the various tests.

Clones supported:
  different ROM set, without Midway copyright and different attract mode
  ("btimea")



Carnival ("carnival")
=====================
Arrows  Move around
CTRL    Fire



Centipede ("centiped")
======================
Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.



Circus Charlie ("circusc")
==========================
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen

Clones supported:
  alternate version which allows to select the level to play ("circusc2")



Colony 7 ("colony7")
====================
Clones supported:
  alternate version ("colony7a")



Commando ("commando")
=====================
Arrows  Move around
CTRL    Fire
Press CTRL during the startup message to enter the test screen

Clones supported:
  Japanese version 'Senjo no ookami' (The wolf of battlefield) ("commandj")



Congo Bongo ("congo")
=====================
Runs on almost the same hardware as Zaxxon.

Use F2 to enter test mode, then 1 to advance from one test to the following.

Clones supported:
  UK version, called Tip Top ("tiptop")

Known issues:
- What do the dip switches do?



Coors Light Bowling ("capbowl")
===============================
F2 enter service mode (press it on the high score screen)

Known issues:
- The colors are accurate, however the color space is reduced from 4x4x4 to 3x3x2.



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
  bootleg version by Alca ("ckongalc")
  version running on Scramble hardware ("ckongs")

Known issues:
- music doesn't work correctly in "ckongs".


Crystal Castles ("ccastles")
============================
To start a game, press the JUMP button. '1' and '2' work only in cocktail
mode.

Known issues:
- Keyboard control isn't up to the task. Use the mouse.
- Sound.  It's better! The game sounds are slightly slow.....
- When the cabinet is set to Cocktail, pressing 2 while in the switch test
  screen of the test mode is supposed to flip the screen. This doesn't work
  yet, since the screen is not refreshed.



Defender ("defender")
===========================
Known issues:
- The joystick controls are strange, but Defender had lots of buttons.. ;-)



Demolition Derby ("destderb")
=============================
MCR/III hardware
Press F2 before initial screen to configure

Track   Steer player 1
CTRL    Accelerate forward
ALT     Accelerate backward

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

Known issues:
- The game stops when you reach the level where you can aim up and down.



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

Clones supported:
  Japanese, maybe earlier version, with levels playing in the order 1-2-3-4
    instead of 1-4-1-3-4-1-2-3-4, a different title screen, 11 characters
	high scores, and no copyright notice ("dkngjrjp")
  Another Japanese (?) version, with levels playing in the order 1-2-3-4
    ("dkjrjp")
  bootleg version, with levels playing in the order 1-2-3-4 ("dkjrbl")



Donkey Kong 3 ("dkong3")
========================
Runs on hardware similar to Donkey Kong

Arrows  Move around
CTRL    Fire
F1      Test (keep it pressed - very nice, try it!)

Known issues:
- The colors come from a bootleg board. They might not be the same as the
  original version.



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
- Some sprite priority problems (people half disappearing behind doos etc.)



Eliminator ("elim2")
====================
Two players version
Clones supported:
  four players version ("elim4")



Exerion ("exerion")
===================
Known issues:
- The background graphics are missing.



Fire Trap ("firetrap")
======================
Clones supported:
  Japanese booleg ("firetpbl")

Known issues:
- Due to copy protection, the original version doesn't work. Use the bootleg instead.



Frogger ("frogger")
===================
Arrows  Move around

Clones supported:
  alternate version, smaller, with different help, but still (C) Sega 1981
     ("frogsega")
  bootleg version, which runs on a modified Scramble board ("froggers")



Front Line ("frontlin")
=======================
Runs on the same hardware as Jungle King.

Known issues:
- The aiming dial is not emulated.
- Can't kill or be killed.



Future Spy ("futspy")
=====================
Doesn't work because the program ROMs are encrypted.



Galaga ("galaga")
=====================
Original version with Midway copyright

In the test screen, use movement & fire to change & hear sound effects

Clone supported:
  Namco copyright ("galaganm")
  Bootleg version ("galagabl")
  Another bootleg called Gallag ("gallag")

Known issues:
- Sometimes explosion sprites appear on the left of the screen.



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
  Defend the Terra Attack on the Red UFO ("redufo")

Known issues:
- The star background is probably not entirely accurate.



Gauntlet ("gauntlet")
=====================
Only the hacked version which doesn't require a slapstic is supported.

Known issues:
- Colors are accurate, however the palette is reduced from a 16-bit IRGB
  (4-4-4-4) to 8-bit RGB (3-3-2) color space.



Gauntlet 2 ("gaunt2")
=====================
Only the hacked version which doesn't require a slapstic is supported.

Known issues:
- Colors are accurate, however the palette is reduced from a 16-bit IRGB
  (4-4-4-4) to 8-bit RGB (3-3-2) color space.



Ghosts 'n Goblins ("gng")
=========================
Arrows  Move around
CTRL    Fire
ALT     Jump
F2      Test mode

Clones supported:
  alternate version with different graphics (the last weapon is a cross
    instead of a shield) ("gngcross")
  Japanese version 'Makai-mura' ("gngjap")

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
Konami copyright

Arrows  Move around
CTRL    Fire

Clones supported:
  licensed to Centuri ("gyrussce")



Gunsmoke ("gunsmoke")
====================
Capcom copyright

Clones supported:
  Romstar license ("gunsmrom")
  Japanese version ("gunsmokj")



Hunchback ("hunchy")
====================
Runs on a modified Donkey Kong Jr. board.

Doesn't work at all due to the custom CPU.



Hyper Sports ("hyperspt")
=========================
To have the high score table properly initialized, the first time you run
the game you should go into the dip switch menu, set World Records to Erase
on Reset, reset the game (F3), and the set the dip switch backto Don't Erase.



Invinco ("invinco")
=================
Press button 2 to select the game.

Known issues:
- dip switches not verified



Invinco / Head On 2 dual game ("invho2")
========================================
Known issues:
- dip switches not verified



Journey ("journey")
===================
MCR/II hardware with MCR/III video
Press F2 before initial screen to configure

Arrows  Move around
CTRL    Fire

Known issues:
- No support for the cassette music



Joust ("joust")
===============
white/green label version

Clones supported:
  white/red label version ("joustwr")
  green label version ("joustg")
  red label version ("joustr")
All of the alternate versions are older, and have the pterodactyl bug. The
order, from older to newer, is: white/red - green - red - white/green



Jump Bug ("jumpbug")
====================
Clones supported:
  "Sega" version (probably a bootleg) ("jbugsega")



Jumping Jack ("jjack")
======================
CTRL+F3 Test mode

Known issues:
- Dip switches not verified



Jungle King ("junglek")
=======================
Clones supported:
  Jungle Hunt ("jhunt")



Kangaroo ("kangaroo")
============================
This runs on hardware very similar to Arabian.

In test mode, to test sound press 1 and 2 player start simultaneously.
Punch + 1 player start moves to the crosshatch pattern.

Known issues:
- F3 doesn't reset the game, it hangs it up.



Kick Rider ("kickridr")
=======================
Arrows  Move around
CTRL    Kick
CTRL+F3 Test mode

Known issues:
- Dip switches are wrong



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
===============
The first time you run the game, you should go into service mode, select
4. Bookkeeping, Reset Krull Elite to have the high score table properly
initialized.



Kung Fu Master ("kungfum")
==========================
Arrows  Move around
Ctrl    Kick
Alt     Punch
F1+F3   Test mode

Clones supported:
  bootleg version copyright by O.K. ("kungfub")



Legendary Wings ("lwings")
==========================
Clones supported:
  Japanese version ("lwingsjp")

Known issues:
- dip switch support is partial.
- The colors should be accurate, but the color space is downgraded from 4x4x4
  to 3x3x2.



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

Press E+F3 to enter test mode

Known issues:
- I don't know where coin counter is located.  The game requires 4 credits
  to start



Lunar Lander ("llander")
========================
Arrows  Left and Right to rotate
Arrows  Up and Down to thrust
A       Abort

Mouse emulates the thrust poti.

Known issues:
- Selftest does not work. It seems page 0 and 1 are mirrored, and
  the cpu emulation can't handle this correctly



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



Major Havoc ("mhavoc")
=======================
Clones supported:
  earlier version ("mhavoc2")
  Return to Vax   ("mhavocrv")



Make Trax ("maketrax")
======================
Clones supported:
  Crush Roller, which is a bootleg version modified to run on a Pac Man
     board. ("crush")

Known issues:
- Colors are accurate for Crush Roller, but they could be wrong for Make Trax.



Mappy ("mappy")
===============
Arrows  Left and Right to move
CTRL    Open door
F1      Skip level

Clones supported:
  Japanese version ("mappyjp")



Mario Bros. ("mario")
=====================
F2      Test (keep it pressed - very nice, try it!)



Mat Mania ("matmania")
======================
Clones supported:
  Exciting Hour ("excthour")

Known issues:
- Most colors seem correct, but the audience graphics use purple instead of
  blue?



Mikie ("mikie")
===============
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen

Known issues:
- Some colors are not very convincing, they might be wrong.



Millipede ("milliped")
======================
As you can imagine, this runs on the same hardware as Centipede.

Arrows  Move around
CTRL    Fire

Known issues:
- What is the clock speed of the original machine? I'm currently using 1Mhz,
  I don't know if the game runs correctly.
- High scores don't seem to work.



Moon Cresta ("mooncrst")
========================
This runs on a hardware very similar to Galaxian.
The ROMs are encrypted. Nichibutsu copyright.

Arrows  Move around
CTRL    Fire

Clones supported:
  Unencrypted Gremlin version ("mooncrsg")
  Unencrypted version, probably bootleg ("mooncrsb")
  bootleg version called Fantazia ("fantazia")

Known issues:
- The graphics ROMs in the "mooncrst" set are wrong.
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

Known issues:
- The background might not be entirely accurate
- Colors in test mode are not correct (black instead of R, G, B). The other
  colors in the game seem to be correct, so I don't know what's going on.
- Some problems with sound, which might be due to imperfect 6808 emulation.



Moon Quasar ("moonqsr")
=======================
This runs on a modified Moon Cresta board.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.



Mouse Trap ("mtrap")
====================
4+F3 to enter service mode



Mr. Do! ("mrdo")
================
Arrows  Move around
CTRL    Fire
F1      Skip level
CTRL+F3 Test mode

Clones supported:
  Version with additional Taito copyright ("mrdot")
  Mr. Lo! ("mrlo")
  Mr. Du! ("mrdu")



Mr. Do's Castle ("docastle")
============================
Arrows  Move around
CTRL    Fire
CTRL+F3 Test mode

Clones supported:
  Different (probably earier) version called Mr. Do vs the Unicorns. It has
     significant differences in the gameplay ("douni")
  Another alternate version, differences are unknown ("docastl2")

Known issues:
- Dip switches don't work



Mr. Do! Run Run ("dorunrun")
============================
CTRL+F3 Test mode

Clones supported:
  Super Piero ("spiero")


Mr. Do! WildRide ("dowild")
===========================
Arrows  Move around
CTRL    Fire
CTRL+F3 Test mode



Ms Pac Man ("mspacman")
=======================
Only the bootleg version is supported.

CTRL    Speed up cheat

Clones supported:
  Miss Pac Plus ("mspacatk")

Known issues:
- Speed cheat doesn't work in Miss Pac Plus
- In Miss Pac Plus the bonus fruits move through the maze walls. This is
  normal, the original does the same.



Naugthy Boy ("naughtyb")
========================
Arrows  Move around
CTRL    Throw the stone

Known issues:
- some colors seem wrong, maybe the PROM is bad or I made some mistake in
  its interpretation.



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
  bootleg version running on Galaxian hardware ("pacmanbl")



Pengo ("pengo")
===============
Original version, encrypted, with "Popcorn" music.

Clones supported:
  alternate version (earlier), not encrypted, with different music and no
    slow screen draw ("pengoa")
  bootleg called Penta, encrypted ("penta")



Pepper II ("pepper2")
=====================
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
- The startup tune is missing.



Pisces ("pisces")
=================
This runs on a modified Galaxian board.

Arrows  Move around
CTRL    Fire

Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Pooyan ("pooyan")
=================
Clones supported:
  bootleg version called Pootan ("pootan")



Popeye ("popeyebl")
=============================
Bootleg version. The original version is not supported.



Pulsar ("pulsar")
=================
Runs on the same hardware as Carnival

Known issues:
- dip switches not verified



Qix ("qix")
===========
Keep F2 pressed to enter test mode, then F1 to advance to the next test.

There are no dip switches, press F1 to enter the service mode, then
  F1      Next screen
  F2      Next line
  F5      Up
  F6      Down

The first time you run the game, it will ask the language. You can use F1 to
proceed through all the configuration screens, or just reset (F3).
To change the language afterwards, delete the QIX.HI file from the HI
directory and re-run the program.

Clones supported:
  Qix II ("qix2")

Known Issues:
- In cocktail mode, player 2 controls don't work right.



Quantum ("quantum")
===================
Clones supported:
  Alternate version ("quantum2")



Q*Bert ("qbert")
================
Arrows  Move around
F1      Test mode
F2      Select

To enter your name in the high score list, use 1 or 2.

The first time you run the game, you should go into service mode, select
4. Options/Parameters, Reset High Score Table to have the high score table
properly initialized.

Clones supported:
  Japanese version ("qbertjp")



Q*Bert Qubes ("qbertqub")
=========================
Arrows  Move around
F1      Test mode
F2      Select

To enter your name in the high score list, use 1 or 2.

Known issues:
- The first time you run the game, you have to reset (F3) when the empty
  supreme noser table appears, then the table will be correctly filled.
  Alternatively, you can insert a coin before this empty table appears...



Rally X ("rallyx")
==================
Arrows  Move around
CTRL    Smoke
F2      Test

Clones supported:
  New Rally X ("nrallyx")

Known issues:
- Sprites are not turned off appropriately.
- Colors are not correct in Rally X. They are in New Rally X.
- Dip switches for New Rally X are not correct



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



Rastan ("rastan")
=================
Clones supported:
  Rastan Saga ("rastsaga")

Known issues:
- Choppy.
- Rastan Saga doesn't always boot, use F3 to make it start.



Reactor ("reactor")
===================
Arrows  Move around
CTRL    Player 1 Energy
ALT     Player 1 Decoy
1       Player 2 Energy
2       Player 3 Decoy
F1      Test mode
F2      Select

Known issues:
- the highscores of the 7-lives games are not saved.  You see - you can
  spend 2 credits and get a game with 7 lives, or spend 1 credit and get
  3 lives, and each has its own highscore table. Currently, only the
  3-lives game table is saved.



Red Baron ("redbaron")
======================
Arrows  Bank left and right
CTRL    Fire

- Red Baron tries to calibrate its analog joystick at the start, so you'll
  have to move the "joystick" a bit before you can fly in all four directions.



Rescue ("rescue")
===================
This game works on the same board of Super Cobra.  Graphic is garbled with
an address shuffling.

Arrows  Move around
CTRL    Fire

Press 1+F3 to enter test mode

Known issues:
- The colors should be correct, however the sky is black instead of blue.
- I don't know where coins per credit selector is.  The game requires 4
  credits to start.



Robotron ("robotron")
===============
Clones supported:
  yellow/orange label version, where quarks are incorrectly called Cubeoids
    during the demo ("robotryo")



Roc'n Rope ("rocnrope")
=======================
Clones supported:
  bootleg called Rope Man ("ropeman")

Knon issues:
- Colors come from the bootleg version, they might be correct for that
  version, but they are wrong for the original.
- The bootleg version crashes when you start a game. This is might be due to
  a slightly different encryption scheme.



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



Section Z ("sectionz")
======================
Same hardware as Legendary Wings.

Known issues:
- The colors should be accurate, but the color space is downgraded from 4x4x4
  to 3x3x2.



Seicross ("seicross")
=====================
Runs on almost the same hardware as Crazy Climber, but not exactly the same.

Arrows  Move around
CTRL    Fire (if SW7B is enabled)

Known issues:
- Setting SW7B to 1 will turn on FIRE ability



Sidearms ("sidearms")
=====================
Clones supported:
  Japanese version ("sidearjp")

Known issues:
- The blinking star background is missing.



Silkworm ("silkworm")
=====================
On startup, keep BOTH start 1 and start 2 pressed to use the test mode.

Known issues:
- The colors are accurate, but the color space is downgraded from 4x4x4 to
  3x3x2.



Snap Jack ("snapjack")
======================
Runs on the same hardware as Lady Bug

Known issues:
- 2 player controls don't work.
- What do the dip switches do?



Space Attack ("sspaceat")
=========================
Runs on same hardware as Carnival

Note that this is NOT the same as "Space Attack II", which runs on Space
Invaders hardware.

Known issues:
- dip switches not verified



Space Duel ("spacduel")
=======================
Arrows  Left & Right rotate ship
CTRL    Fire
ALT     Thrust
SPACE   Shields

- Press "2" to choose level, the game won't start otherwise.



Space Firebird ("spacefb")
==========================
Arrows  Move around
ALT     Fire
CTRL    Escape

Known issues:
- Red screen "flash" when you die is not supported
- Background Starfield not supported. It is NOT a Galaxians type starfield

Bugs which aren't:
- The sprites/characters don't use transperancy in the real game. This is
  not a bug



Space Fury ("spacfury")
=======================
Clones supported:
  revision C ("spacfurc")



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
  Space Invaders Part 2 (Taito) - this one is a color game, unlike the
    others ("invadpt2")
  Space Phantoms ("spaceph")
  Rolling Crash ("rollingc")
  Cosmic Monsters ("cosmicmo")


Known issues:
- The colors used in Invaders Revenge may be wrong.
- Space Invaders Part 2 doesn't turn red when you die.


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
- Palette seems to be slightly off



Star Force ("starforc")
=======================
Clones supported:
  Mega Force ("megaforc")



Stargate ("stargate")
=====================
Known issues:
- The joystick controls are strange, but Stargate had lots of buttons.. ;-)



Star Wars ("starwars")
======================
Known issues:
- Dip switches not functional, but you can change game settings through
  the self test menus.


Super Basketball ("sbasketb")
=============================
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen



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
Midway copyright

Arrows  Move around
CTRL    Speed

Clones supported:
  Namco copyright ("superpcn")



Super Zaxxon ("szaxxon")
========================
Known issues:
- The program ROMs are encrypted. The game seems to work, but there might be
  misbehaviours caused by imperfect decryption.



Tapper ("tapper")
=================
Clones supported:
  Root Beer Tapper ("rbtapper")



Tempest ("tempest")
===================
Arrows  Left & Right move around
CTRL    Zap
ALT     Super Zapper

Mouse emulates spinner.

Clones supported:
  Tempest Tubes ("temptube")

Known issues:
- Several people complained that mouse control is reversed. This is not the
  case. The more abvious place where this can be seen is the level selection
  screen at the beginning: move the mouse right, the block goes right.
  Anyway, if you don't like the key assignments, you can change them.



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



Track & Field ("trackfld")
==========================
To have the high score table properly initialized, the first time you run
the game you should go into the dip switch menu, set World Records to Erase
on Reset, reset the game (F3), and the set the dip switch backto Don't Erase.

Clones supported:
  Hyper Olympic ("hyprolym")



Triple Punch ("triplep")
========================
Known issues:
- sometimes the game resets after you die. This is probably caused by the
  copy protection.



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



Vastar ("vastar")
=================
Known issues:
- Some wrong sprite graphics (e.g. when you die)



Venture ("venture")
===================
3+F3    Test mode



Wacko ("wacko")
===============
MCR/II hardware
Press F2 before initial screen to configure

Track   Move around
Arrows  Fire



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



Xevious ("xevious")
===================
Atari/Namco copyright

Clones supported:
  Namco copyright ("xeviousn")
  Super Xevious ("sxevious")

Known issues:
- Sometimes explosion sprites appear at the top of the screen.



Zaxxon ("zaxxon")
=================
Use F2 to enter test mode, then 1 to advance from one test to the following.



Zoo Keeper ("zookeep")
======================
Keep F2 pressed to enter test mode, then F1 to advance to the next test.

There are no dip switches, press F1 to enter the service mode, then
  F1      Next screen
  F2      Next line
  F5      Up
  F6      Down

The first time you run the game, it will ask the location. You can change the
name using F5/F6 a F2, then F1 to proceed through all the configuration
screens, or just reset (F3).

Known issues:
- Every time you start the emulation, you get a free credit



Acknowledgements
----------------

First of all, thanks to Allard van der Bas (avdbas@wi.leidenuniv.nl) for
starting the Arcade Emulation Programming Repository at
http://valhalla.ph.tn.tudelft.nl/emul8
Without the Repository, I would never have even tried to write an emulator.
Unfortunately, the original Repository is now closed, but its spirit lives
on in MAME.

Z80Em Portable Zilog Z80 Emulator Copyright (C) Marcel de Kogel 1996,1997
   Note: the version used in MAME is slightly modified. You can find the
   original version at http://www.komkon.org/~dekogel/misc.html.
M6502 Emulator Copyright (C) Marat Fayzullin, Alex Krasivsky 1996
   Note: the version used in MAME is slightly modified. You can find the
   original version at http://freeflight.com/fms/.
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M68000 emulator taken from the System 16 Arcade Emulator by Thierry Lescot.
Allegro library by Shawn Hargreaves, 1994/97
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203 and YM-2151 emulation by Tatsuyuki Satoh.
OPL based YM-2203 emulation by Ishmair (ishmair@vnet.es).
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention them all.



Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -soundcard 0    will run Ms Pac Man without sound

options:
-scanlines/-noscanlines (default: -scanlines)
              if the default mode doesn't work with your monitor/video card
              (double image/picture squashed vertically), use -noscanlines.
-vesa         Tries to automatically pick the best VESA mode available.
-vesa1        use a standard VESA 1.2 mode instead of custom
              video mode. By default this simulates scanlines unless the
              above option ('-noscanlines') is given, or the game is too wide
              to fit on the screen.
              You probably want to try this if the custom modes don't work
              correct for you.  Use PGUP/PGDN to scroll the game up/down
              if the game is too tall to fit entirely on the screen.
-vesa2b       same as '-vesa1' except this uses VESA2.0 banked mode extension
-vesa2l       same as '-vesa1' except this uses VESA2.0 linear mode extension
              (recommended if available since it is faster than VESA 1.2
-XxY          where X and Y are width and height (ex: '-800x600')
              This is the preferred method for selecting the
              VESA resolution. For possible X/Y combinations see below
-320          tell MAME to use a vesa 320x240 video mode
              If you get an error '320x240 not supported', you probably
              need Scitech's Display Doctor, which provides the 'de facto'
              standard VESA implementaion (http://www.scitechsoft.com)
              Note: this is a nice alternative to '-640x480 -noscanlines'
-400          same as above, video mode is 400x300
-512          same as above, video mode is 512x384
-640          same as above, video mode is 640x480
-800          same as above, video mode is 800x600.
-1024         same as above, video mode is 1024x768
-1280         same as above, video mode is 1280x1024
-1600         same as above, video mode is 1600x1200
-skiplines N  since most games use a screen taller than 240 lines,
              they won't fit in the screen. The parameter 'N' sets
              the initial number of lines to skip at the top of
              the screen. You can adjust the position while the game
              is running using the PGUP and PGDOWN keys.
              Note: this works _only_ if a VESA mode is selected,
                    e.g. ('-640x480 -skiplines 10')
-double/-nodouble (default: -double)
              use nodouble to disable pixel doubling in VESA modes (faster,
			  but smaller picture)
-vgafreq n    where n can be 0 (default) 1, 2 or 3.
              use different frequencies for the custom video modes. This
              could reduce flicker, especially in the 224x288noscanlines
              mode. WARNING: THE FREQUENCIES USED MIGHT BE WAY OUTSIDE OF
              YOUR MONITOR RANGE, AND COULD EVEN DAMAGE IT. USE THESE OPTIONS
              AT YOUR OWN RISK.
-vsync/-novsync (default: -novsync)
              syncronize video display with the video beam instead of using
              the timer. This option can only be used if the selected video
              mode has an appropriate refresh rate; MAME will otherwise refuse
              to start, telling you the actual refresh rate of the video mode,
              and the rate it should have.
              If you are using a tweaked mode, MAME will try to automatically
              pick the correct setting for -vgafreq; you can still override it
              using the -vgafreq option. Note: the 224x288 noscanlines mode
              doesn't work on most cards. This mode is used by many games,
              e.g. Pac Man and Galaga. If it doesn't work on your card, either
              use the scanlines version, or don't use -vsync.
              If you are using a VESA mode you should use the program which
              came with your video card to set the appropriate refresh rate.
              Note that when this option is turned on, speed will NOT
              downgrade nicely if your system is not fast enough (i.e.:
			  jerkier gameplay).
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
-sr n         set the audio sample rate. The default is 44100. Smaller values
              (e.g. 22050, 11025) will cause lower audio quality but faster
              emulation speed.
-sb n         set the audio sample bits, either 8 or 16. The default is 8.
              16 will increase quality with certain games, but decrease speed.
              This is a software setting, not hardware. The sound card will
              always be used in 16 bit mode, if possible.
-joy n/-nojoy (default: -nojoy) allow joystick input, n can be:
              0 - normal 2 button joystick
              1 - CH Flightstick Pro
              2 - Stick/Pad with 4 buttons
              3 - Stick/Pad with 6 buttons
              4 - dual joysticks
              5 - Wingman Extreme (or Wingman Warrior without spinner)
              Press F7 to calibrate the joystick. Calibration data will be
              saved in mame.cfg. If you're using different joytypes for
              different games, you may need to recalibrate your joystick
              every time.
-fm/-nofm (default: -nofm) use the SoundBlaster OPL chip for music emulation
              in some games. This is faster, but emulation is less faithful.
-log          create a log of illegal memory accesses in ERROR.LOG
-help, -?     display current mame version and copyright notice
-list         display a list of currently supported games
-listfull     display a list of game directory names + description
-listroms     display selected game required roms
-listsamples  display selected game required samples
-romdir       specify an alternate directory where to load the ROMs from
-mouse/-nomouse (default: -mouse) enable/disable mouse support
-frameskip n  skip frames to speed up the emulation. For example, if the game
              normally runs at 60 fps, "-frameskip 1" will make it run at 30
              fps, and "-frameskip 2" at 20 fps. Use F11 to check the speed
			  your computer is actually reaching. If the game is too slow,
              increase the frameskip value. Note that this setting can also
              affect audio quality (some games sound better, others sound
              worse).  Maximum value for frameskip is 3.
-vg           is no longer supported. It's now the default when using
              vesa for vector games.
-aa/-noaa (default: -noaa) use some crude form of anti_aliasing for the
              vector games.
-cheat        Cheats like the speedup in Pac Man or the level skip in many
              other games are disabled by default. Use this switch to turn
              them on.
-debug        Activate the integrated debugger. During the emulation, press
              tilde to enter the debugger.
-record nnn   Record joystick input on file nnn.
-playback nnn Playback joystick input from file nnn.
-savecfg      save the configuration into mame.cfg
-ignorecfg    ignore mame.cfg and start with the default settings


The following keys work in all emulators:
Tab          Enter coonfiguration menu. Press Tab or Esc to get back to the
             emulation.
P            Pause
F3           Reset
F4           Show the game graphics. Use cursor keys to change set/color,
             F4 or Esc to return to the emulation.
F7           Calibrate the joystick
F8           Change frame skip on the fly (60, 30, 20, or 15)
F9           To change volume percentage thru 100,75,50,25,0 values
F10          Toggle speed throttling
F11          Toggle speed display
F12          Save a screen snapshot. The default target directory is PCX, you
             have to create it yourself, it will not be created by the program
             if it isn't there.
numpad +/-   Volume adjust
left shift + numpad +/- Gamma correction adjust
ESC          Exit emulator
