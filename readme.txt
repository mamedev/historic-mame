
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
   medium. You are allowed to make them available for download on the same web
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
specific to a system other than DOS (e.g. Mac, Win32, Unix), they should be sent
to the respective port maintainer. DON'T SEND THEM TO MIRKO OR NICOLA - they
will be ignored.

You can send source code to mame@lim.dsi.unimi.it. Do not use this address for
non-source code bug reports.

Nicola Salmoria    MC6489@mclink.it
Mirko Buffoni      mix@lim.dsi.unimi.it

Mike Balfour       mab22@po.cwru.edu
Aaron Giles        agiles@sirius.com
Chris Moore        chris.moore@writeme.com
Brad Oliver        bradman@primenet.com
Andrew Scott       ascott@utkux.utcc.utk.edu
Martin Scragg      mnm@netspace.net.au
Zsolt Vasvari      vaszs01@banet.net
Valerio Verrando   v.verrando@mclink.it
Bernd Wiebelt      bernardo@studi.mathematik.hu-berlin.de

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have (e.g. http://www.crcwnet.com/~no-sleep/newic/), or ask on the appropriate
Usenet newsgroups (e.g. comp.emulators.misc) or on the emulation message boards
(e.g. http://www.escape.com/~ego/dave/mame/).

Also, DO NOT SEND REQUESTS FOR NEW GAMES TO ADD, unless you have some original
info on the game hardware or, even better, own the board and have the technical
expertise needed to help us.
Please don't send us information widely available on the Internet - we are
perfectly capable of finding it ourselves, thank you.



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
| Alpine Ski           |   No   |  Yes   |  Yes   |   No   |  Yes   | alpine   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Amidar               |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | amidar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Anteater             |  Yes   |   No   |  Yes   |  Yes   |   No   | anteater |
|----------------------|--------|--------|--------|--------|--------|----------|
| Arabian              |  Yes   | Close  |  Yes   |  Yes   |   No   | arabian  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Armored Car          |  Yes   |   No   |  Yes   |  Yes   |   No   | armorcar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Arkanoid             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | arkanoid |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids            |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | asteroid |
|----------------------|--------|--------|--------|--------|--------|----------|
| Asteroids Deluxe     |  Yes   |  Yes   |  Yes   |  Yes   |   No   | astdelux |
|----------------------|--------|--------|--------|--------|--------|----------|
| Astro Blaster        |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | astrob   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Astro Fighter        |  Yes   |   No   |   No   |  Yes   |   No   | astrof   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Astro Invader        |  Yes   |  Yes   |  Yes   |   No   |   No   | astinvad |
|----------------------|--------|--------|--------|--------|--------|----------|
| Atari Basketball     |  Yes   |  b/w   |Partial |   No   |   No   | bsktball |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bad Dudes            |  Yes   |  Yes   | Yes(3) |   No   |   No   | baddudes |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bagman               |  Yes   |  Yes   | Music  |  Yes   |  Yes   | bagman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bandido              |  Yes   |  b/w   |   No   |   No   |   No   | bandido  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bank Panic           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | bankp    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle of Atlantis   |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | atlantis |
|----------------------|--------|--------|--------|--------|--------|----------|
| Battle Zone          |  Yes   |Overlay | Yes(1) |  Yes   |   No   | bzone    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Berzerk              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | berzerk  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Black Tiger          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | blktiger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Black Widow          |  Yes   | Close  |  Yes   |   No   |   No   | bwidow   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blaster              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | blaster  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Block Out            |  Yes   |  Yes   |  Yes   |   No   |   No   | blockout |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blockade             |  Yes   |  b/w   |Partl(1)|  n/a   |   No   | blockade |
|----------------------|--------|--------|--------|--------|--------|----------|
| Blue Print           |  Yes   |   No   |  Yes   |  Yes   |  Yes   | blueprnt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bomb Jack            |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | bombjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Boot Hill            |  Yes   |  b/w   | Yes(1) |  n/a   |   No   | boothill |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bosconian            |  Yes   |  Yes   |  Yes   |   No   |   No   | bosco    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Break Thru           |  Yes   |  Yes   | Yes(3) |  Yes   |  Yes   | brkthru  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubble Bobble        |  Yes   |  Yes   | Yes(3) |  Yes   |   No   | bublbobl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bubbles              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bubbles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Bump 'n Jump         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | bnj      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Burger Time          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | btime    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Capcom Bowling       |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | capbowl  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Carnival             |  Yes   |  Yes   |Partl(1)|  Yes   |   No   | carnival |
|----------------------|--------|--------|--------|--------|--------|----------|
| Centipede            |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | centiped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Champion Baseball    |  Yes   |  Yes?  |  Yes   |   No   |   No   | champbas |
|----------------------|--------|--------|--------|--------|--------|----------|
| Checkman             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | checkman |
|----------------------|--------|--------|--------|--------|--------|----------|
| Chelnov              |Partial | Close  |  Yes   |   No   |   No   | chelnov  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Choplifter           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | chplftb  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Circus               |  Yes   |Overlay |Partial |  Yes   |   No   | circus   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Circus Charlie       |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | circusc  |
|----------------------|--------|--------|--------|--------|--------|----------|
| City Connection      |  Yes   |  Yes   | Yes(2) |   No   |  Yes   | citycon  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cloak & Dagger       |  Yes   |  Yes   |  Yes   |   No   |   No   | cloak    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Colony 7             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | colony7  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Commando (Capcom)    |  Yes   |  Yes   | Yes(2) |  Yes   |  Yes   | commando |
|----------------------|--------|--------|--------|--------|--------|----------|
| Commando (Sega)      |   No   |   No   |  Yes   |   No   |  Yes   | commsega |
|----------------------|--------|--------|--------|--------|--------|----------|
| Congo Bongo          |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | congo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cops'n Robbers       |  Yes   |Overlay |   No   |  n/a   |  n/a   | copsnrob |
|----------------------|--------|--------|--------|--------|--------|----------|
| Cosmic Avenger       |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | cavenger |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crash                |  Yes   |  Yes   |Partial |  Yes   |   No   | crash    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crater Raider        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | crater   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Climber        |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | cclimber |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crazy Kong           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ckong    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Crystal Castles      |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | ccastles |
|----------------------|--------|--------|--------|--------|--------|----------|
| D-Day                |  Yes   |   No   |  Yes   |  Yes   |  n/a   | dday     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Dark Planet          |   No   |  Yes   |  Yes   |   No   |   No   | darkplnt |
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
| Dominos              |  Yes   |  b/w   |   No   |   No   |   No   | dominos  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong          |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | dkong    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong Jr.      |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | dkongjr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Donkey Kong 3        |  Yes   |  Yes   |Partial |  Yes   |   No   | dkong3   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eggs                 |  Yes   |   No   |  Yes   |  Yes   |   No   | eggs     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Elevator Action      |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | elevator |
|----------------------|--------|--------|--------|--------|--------|----------|
| Eliminator           |  Yes   | Close  | Yes(1) |  Yes   |   No   | elim2    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Espial               |  Yes   |  Yes?  |  Yes   |   No   |   No   | espial   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exed Exes            |  Yes   |  Yes   |Partial |  Yes   |   No   | exedexes |
|----------------------|--------|--------|--------|--------|--------|----------|
| Exerion              |  Yes   |   No   |  Yes   |  Yes   |   No   | exerion  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Extra Bases          |   No   |   No   |   No   |   No   |   No   | ebases   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fantasy              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | fantasy  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fast Freddie         |  Yes   |   No   |  Yes   |  Yes   |  Yes   | fastfred |
|----------------------|--------|--------|--------|--------|--------|----------|
| Final Fight          |  Yes   |  Yes   |  Yes   |   No   |   No   | ffight   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Fire Trap            |  Yes   |  Yes   | Yes(3) |  Yes   |  Yes   | firetrap |
|----------------------|--------|--------|--------|--------|--------|----------|
| Food Fight           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | foodf    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frenzy               |  Yes   |  Yes   | Yes(1) |   No   |   No   | frenzy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frisky Tom           |   No   |   No   |  Yes   |   No   |   No   | friskyt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Frogger              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | frogger  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Front Line           |Partial.|  Yes   |  Yes   |   No   |  Yes   | frontlin |
|----------------------|--------|--------|--------|--------|--------|----------|
| Future Spy           |   No   |   No   |   No   |   No   |   No   | futspy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaga               |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | galaga   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Galaxian             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | galaxian |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gauntlet             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | gauntlet |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gauntlet 2           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | gaunt2   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gemini Wing          |  Yes   |  Yes   | Yes(3) |   No   |   No   | gemini   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ghosts 'n Goblins    |  Yes   |  Yes   | Yes(2) |  Yes   |  Yes   | gng      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gorf                 |  Yes   |   No   | Yes(1) |  Yes   |  Yes   | gorf     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gravitar             |  Yes   | Close  |  Yes   |  Yes   |   No   | gravitar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Green Beret          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | gberet   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gun Dealer           |  Yes   |  Yes   | Yes(2) |   No   |   No   | gundealr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gunsmoke             |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | gunsmoke |
|----------------------|--------|--------|--------|--------|--------|----------|
| Guzzler              |  Yes   |  Yes   |  Yes   |   No   |  Yes   | guzzler  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Gyruss               |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | gyruss   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Heavy Barrel         |Partial.|  Yes   | Yes(3) |   No   |   No   | hbarrel  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Hunchback            |   No   |  n/a   |   No   |  n/a   |   No   | hunchy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Hippodrome           |Partial.|  Yes   | Yes(3) |   No   |   No   | hippodrm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Hyper Sports         |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | hyperspt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Indiana Jones        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | indytemp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Invinco              |  Yes   |  Yes?  |   No   |   No   |   No   | invinco  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Invinco / Head On 2  |  Yes   |  Yes?  |   No   |   No   |   No   | invho2   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Iron Horse           |  Yes   |  Yes   | Yes(2) |   No   |   No   | ironhors |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jack the Giant Killer|  Yes   |  Yes   |  Yes   |  Yes   |   No   | jack     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Journey              |  Yes   |  Yes   |Partial.|  Yes   |   No   | journey  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Joust                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | joust    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jr. Pacman           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | jrpacman |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jump Bug             |  Yes   |   No   |  Yes   |  Yes   |   No   | jumpbug  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jump Coaster         |  Yes   |   No   |  Yes   |  Yes   |  Yes   | jumpcoas |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jumping Jack         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | jjack    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jungle King          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | junglek  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Jungler              |  Yes   |   No   |  Yes   |   No   |  Yes   | jungler  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kangaroo             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kangaroo |
|----------------------|--------|--------|--------|--------|--------|----------|
| Karnov               |  Yes   | Close  |  Yes   |   No   |   No   | karnov   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kick     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Kick Rider           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | kickridr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Killer Comet         |  Yes   |   No   |   No   |   No   |   No   | killcom  |
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
| Leprechaun           |  Yes   |  Yes?? |  Yes   |  Yes   |  Yes   | leprechn |
|----------------------|--------|--------|--------|--------|--------|----------|
| Liberator            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | liberatr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lock'n'Chase         |  Yes   |   No   |  Yes   |  Yes   |   No   | lnc      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Loco-Motion          |  Yes   |  Yes   |  Yes   |   No   |  Yes   | locomotn |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lode Runner          |  Yes   |  Yes   |  Yes   |   No   |   No   | lrunner  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lost Tomb            |  Yes   |   No   |  Yes   |   No   |   No   | losttomb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Lotto Fun            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | lottofun |
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
| Marble Madness       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | marble   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mario Bros.          |  Yes   |  Yes   |Partl(1)|  Yes   |   No   | mario    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mat Mania            |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | matmania |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mega Twins           |  Yes   |  Yes   |  Yes   |   No   |   No   | mtwins   |
|----------------------|--------|--------|--------|--------|--------|----------|
| MegaTack             |  Yes   |   No   |   No   |   No   |   No   | megatack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Midnight Resistance  |  Yes   |  Yes   |   No   |   No   |   No   | midres   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mikie                |  Yes   | Close  |  Yes   |  Yes   |   No   | mikie    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Millipede            |  Yes   |  Yes   |   No   |  Yes   |   No   | milliped |
|----------------------|--------|--------|--------|--------|--------|----------|
| Minefield            |  Yes   |  Yes   |  Yes   |   No   |  n/a   | minefld  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Missile Command      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | missile  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Monster Bash         |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | monsterb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Cresta          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | mooncrst |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Patrol          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | mpatrol  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Moon Quasar          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | moonqsr  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Motos                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | motos    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Mouse Trap           |  Yes   |  Yes   |   No   |  Yes   |   No   | mtrap    |
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
| New Zealand Story    |Partial |   No   |  Yes   |   No   |   No   | tnzs     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Nibbler              |  Yes   |  Yes   |Partial |  Yes   |  Yes   | nibbler  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Night Driver         |  Yes   |  b/w   |   No   |   No   |   No   | nitedrvr |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ninja Gai Den        |  Yes   |  Yes   |Partial |   No   |   No   | gaiden   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Omega Race           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | omegrace |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac-Land             |  Yes   |  Yes   |   No   |   No   |   No   | pacland  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac Man              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pacman   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pac & Pal            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | pacnpal  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pengo                |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | pengo    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pepper II            |  Yes   |  Yes   |   No   |  Yes   |   No   | pepper2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Peter Packrat        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | peterpak |
|----------------------|--------|--------|--------|--------|--------|----------|
| Phoenix              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | phoenix  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pinball Action       |  Yes   |  Yes   |   No   |   No   |   No   | pbaction |
|----------------------|--------|--------|--------|--------|--------|----------|
| Ping Pong            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | pingpong |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pisces               |  Yes   |  Yes   |   Yes  |  Yes   |   No   | pisces   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pitfall 2            |  Yes   |   No   |  Yes   |  Yes   |   No   | pitfall2 |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pleiads              |  Yes   |  Yes   |Limited |  Yes   |   No   | pleiads  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pooyan               |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | pooyan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Popeye               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | popeye   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pop Flamer           |  Yes   |  Yes   |  Yes   |   No   |   No   | popflame |
|----------------------|--------|--------|--------|--------|--------|----------|
| Pulsar               |  Yes   |  Yes?  |   No   |   No   |   No   | pulsar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Punch Out            |  Yes   |  Yes   |Partial |  Yes   |  n/a   | punchout |
|----------------------|--------|--------|--------|--------|--------|----------|
| Punk Shot            |  Yes   |  Yes   |   No   |   No   |   No   | punkshot |
|----------------------|--------|--------|--------|--------|--------|----------|
| Qix                  |  Yes   |  Yes   |  Yes   |  Yes   |   No   | qix      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Quantum              |  Yes   |   No   |  Yes   |  Yes   |   No   | quantum  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert               |  Yes   |  Yes   |Partial |  Yes   |  Yes   | qbert    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Q*Bert Qubes         |  Yes   |  Yes   |Partial |  Yes   |   No   | qbertqub |
|----------------------|--------|--------|--------|--------|--------|----------|
| Radar Scope          |  Yes   |  Yes?  |Partial |   No   |   No   | radarscp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rainbow Islands      | Partl  |  Yes   |   No   |   No   |   No   | rainbow  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rally X              |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | rallyx   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rampage              |  Yes   |  Yes   |  Yes   |   No   |   No   | rampage  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rastan               |  Yes   | Close  |  Yes   |   No   |   No   | rastan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Red Baron            |  Yes   | Close  | Yes(1) |  Yes   |   No   | redbaron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Reactor              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | reactor  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rescue               |  Yes   |  Yes   |  Yes   |   No   |  n/a   | rescue   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Return of the Jedi   |  Yes   |   No   |  Yes   |  Yes   |   No   | jedi     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Road Blasters        |  Yes   |  Yes   |  Yes   |  Yes   |   No   | roadblst |
|----------------------|--------|--------|--------|--------|--------|----------|
| Road Fighter         |  Yes   |   No   |  Yes   |   No   |  Yes   | roadf    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Road Runner          |  Yes   |  Yes   |  Yes   |  Yes   |   No   | roadrunn |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robocop              |  Yes   |  Yes   | Yes(3) |   No   |   No   | robocopp |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robot Bowl           |  Yes   |  Yes   |   No   |   No   |   No   | robotbwl |
|----------------------|--------|--------|--------|--------|--------|----------|
| Robotron             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | robotron |
|----------------------|--------|--------|--------|--------|--------|----------|
| Roc'n Rope           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | rocnrope |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rolling Crash        |  Yes   |  Yes   |   No   |   No   |   No   | rollingc |
|----------------------|--------|--------|--------|--------|--------|----------|
| Route 16             |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | route16  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Rygar                |  Yes   |  Yes   | Yes(3) |   No   |   No   | rygar    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sarge                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | sarge    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Satan's Hollow       |  Yes   |  Yes   |  Yes   |  Yes   |   No   | shollow  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Scramble             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | scramble |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sea Wolf ][          |  Yes   |   No   |   No   |  Yes   |  n/a   | seawolf2 |
|----------------------|--------|--------|--------|--------|--------|----------|
| Section Z            |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | sectionz |
|----------------------|--------|--------|--------|--------|--------|----------|
| Seicross             |  Yes   |  Yes   |  Yes   |   No   |   No   | seicross |
|----------------------|--------|--------|--------|--------|--------|----------|
| Shao-Lin's Road      |  Yes   |  Yes   |  Yes   |  Yes   |   No   | shaolins |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sidearms             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | sidearms |
|----------------------|--------|--------|--------|--------|--------|----------|
| Side Pocket          |  Yes   |   No   | Yes(3) |   No   |   No   | sidepckt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Silkworm             |  Yes   |  Yes   | Yes(3) |   No   |   No   | silkworm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sinistar             |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | sinistar |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sly Spy              |  Yes   |  Yes   |   No   |   No   |   No   | slyspy   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Snap Jack            |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | snapjack |
|----------------------|--------|--------|--------|--------|--------|----------|
| Snow Bros            |  Yes   |  Yes?  | Yes(3) |   No   |  n/a   | snowbros |
|----------------------|--------|--------|--------|--------|--------|----------|
| Solar Fox            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | solarfox |
|----------------------|--------|--------|--------|--------|--------|----------|
| Son Son              |  Yes   |  Yes   |  Yes   |  Yes   |  n/a   | sonson   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Attack         |  Yes   |   No   |   No   |   No   |   No   | sspaceat |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Chaser         |  Yes   |  Yes   |  Yes   |   No   |   No   | schaser  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Duel           |  Yes   | Close  |  Yes   |  Yes   |   No   | spacduel |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Firebird       |  Yes   |  Yes   |   No   |  Yes   |   No   | spacefb  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Fury           |  Yes   | Close  | Yes(1) |  Yes   |   No   | spacfury |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Invaders       |  Yes   |Overlay | Yes(1) |  Yes   |   No   | invaders |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Odyssey        |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | spaceod  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Panic          |  Yes   | Close  |   No   |  Yes   |   No   | panic    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Space Zap            |  Yes   | Maybe  |   No   |  Yes   |   No   | spacezap |
|----------------------|--------|--------|--------|--------|--------|----------|
| Spectar              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | spectar  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Speed Rumbler        |  Yes   |  Yes   | Yes(2) |  Yes   |   No   | srumbler |
|----------------------|--------|--------|--------|--------|--------|----------|
| Splat                |  Yes   |  Yes   |  Yes   |  Yes   |   No   | splat    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Sprint 2             |  Yes   | Maybe  |   No   |  n/a   |  n/a   | sprint2  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Spy Hunter           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | spyhunt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Starfire             |   No   |   No   |   No   |   No   |   No   | starfire |
|----------------------|--------|--------|--------|--------|--------|----------|
| Starforce            |  Yes   |  Yes   |  Yes   |  Yes   |   No   | starforc |
|----------------------|--------|--------|--------|--------|--------|----------|
| Stargate             |  Yes   |  Yes   |  Yes   |  Yes   |   No   | stargate |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Trek            |  Yes   | Close  | Yes(1) |  Yes   |   No   | startrek |
|----------------------|--------|--------|--------|--------|--------|----------|
| Star Wars            |  Yes   | Close  |  Yes   |  Yes   |   No   | starwars |
|----------------------|--------|--------|--------|--------|--------|----------|
| Strategy X           |  Yes   |   No   |  Yes   |   No   |   No   | stratgyx |
|----------------------|--------|--------|--------|--------|--------|----------|
| Stratovox            |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | stratvox |
|----------------------|--------|--------|--------|--------|--------|----------|
| Strider              |  Yes   |  Yes   |  Yes   |   No   |   No   | strider  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Bagman         |  Yes   |  Yes   | Music  |  Yes   |  Yes   | sbagman  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Basketball     |  Yes   |  Yes   |No Speech   No   |   No   | sbasketb |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Breakout       |  Yes   |Overlay | Wrong  |  Yes   |  n/a   | sbrkout  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Cobra          |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | scobra   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super PacMan         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | superpac |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Qix            |  Yes   |  Yes   |  Yes   |   No   |   No   | superqix |
|----------------------|--------|--------|--------|--------|--------|----------|
| Super Zaxxon         |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | szaxxon  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Swimmer              |  Yes   |  Yes   |  Yes   |   No   |  Yes   | swimmer  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tac/Scan             |  Yes   | Close  | Yes(1) |  Yes   |   No   | tacscan  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tank Battalion       |  Yes   |  Yes?  | Yes(1) |  Yes   |  Yes   | tankbatt |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tapper               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tapper   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Targ                 |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | targ     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tazz-Mania           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tazmania |
|----------------------|--------|--------|--------|--------|--------|----------|
| T.M.N.T.             |  Yes   |  Yes   |Partial |   No   |   No   | tmnt     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tempest              |  Yes   | Close  |  Yes   |  Yes   |   No   | tempest  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tetris (Atari)       |  Yes   |  Yes   |  Yes   |  Yes   |  n/a   | atetris  |
|----------------------|--------|--------|--------|--------|--------|----------|
| The Adv.of Robby Roto|  Yes   |   No   |   No   |  Yes   |  Yes   | robby    |
|----------------------|--------|--------|--------|--------|--------|----------|
| The Amazing Maze Game|  Yes   |  b/w   |   No   |  n/a   |   No   | maze     |
|----------------------|--------|--------|--------|--------|--------|----------|
| The End              |  Yes   |   No   |  Yes   |  Yes   |   No   | theend   |
|----------------------|--------|--------|--------|--------|--------|----------|
| The Pit              |  Yes   |  Yes?  |  Yes   |  Yes   |  Yes   | thepit   |
|----------------------|--------|--------|--------|--------|--------|----------|
| The Tower of Druaga  |  Yes   |  Yes   |  Yes   |  Yes   |   No   | todruaga |
|----------------------|--------|--------|--------|--------|--------|----------|
| Three Stooges        |  Yes   |  Yes   |Partial |  Yes   |   No   | 3stooges |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tiger Road           |  Yes   |  Yes   |  Yes   |   No   |   No   | tigeroad |
|----------------------|--------|--------|--------|--------|--------|----------|
| Timber               |  Yes   |  Yes   |  Yes   |  Yes   |   No   | timber   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot           |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | timeplt  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Time Pilot 84        |  Yes   |  Yes?  |  Yes   |  Yes   |   No   | tp84     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Toki                 |  Yes   |  Yes   |Partial |  Yes   |   No   | toki     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Toobin'              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | toobin   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tornado Baseball     |  Yes   |  b/w   |   No   |  n/a   |   No   | tornbase |
|----------------------|--------|--------|--------|--------|--------|----------|
| Track & Field        |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | trackfld |
|----------------------|--------|--------|--------|--------|--------|----------|
| Traverse USA         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | travrusa |
|----------------------|--------|--------|--------|--------|--------|----------|
| Triple Punch         |  Yes   |  Yes   |  Yes   |  Yes   |   No   | triplep  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Trojan               |  Yes   |  Yes   | Yes(2) |   No   |   No   | trojan   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tron                 |  Yes   |  Yes   |  Yes   |  Yes   |   No   | tron     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Turtles              |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | turtles  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Tutankham            |  Yes   |  Yes   |  Yes   |  Yes   |  Yes   | tutankhm |
|----------------------|--------|--------|--------|--------|--------|----------|
| Two Tigers           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | twotiger |
|----------------------|--------|--------|--------|--------|--------|----------|
| UN Squadron          |  Yes   |  Yes   |  Yes   |   No   |   No   | unsquad  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Uniwars              |  Yes   |  Yes   |  Yes   |  Yes   |   No   | uniwars  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Up'n Down            |Partial |   No   |  Yes   |   No   |   No   | upndown  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vanguard             |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | vanguard |
|----------------------|--------|--------|--------|--------|--------|----------|
| Vastar               |  Yes   |   No   |  Yes   |   No   |   No   | vastar   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Venture              |  Yes   |  Yes   |   No   |  Yes   |   No   | venture  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Video Hustler        |  Yes   |  Yes   |  Yes   |   No   |  Yes   | hustler  |
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
| Willow               |  Yes   |  Yes   |  Yes   |   No   |   No   | willow   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wiz                  |  Yes   |   No   |  Yes   |  Yes   |  Yes   | wiz      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wizard of Wor        |  Yes   | Maybe  | Yes(1) |  Yes   |  n/a   | wow      |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wonder Boy Deluxe    |  Yes   |   No   |  Yes   |  Yes   |   No   | wbdeluxe |
|----------------------|--------|--------|--------|--------|--------|----------|
| Wonder Boy in Monster Land    |  Yes   |  Yes   |   No   |   No   | wbml     |
|----------------------|--------|--------|--------|--------|--------|----------|
| World Cup 90         |  Yes   |  Yes   |  Yes   |   No   |   No   | wc90     |
|----------------------|--------|--------|--------|--------|--------|----------|
| Yie Ar Kung Fu       |  Yes   |  Yes   | Yes(1) |  Yes   |  Yes   | yiear    |
|----------------------|--------|--------|--------|--------|--------|----------|
| Xain'd Sleena        |Partial |  Yes   | Yes(2) |   No   |   No   | xsleena  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Xevious              |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | xevious  |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zarzon               |  Yes   |  Yes?  |   No   |   No   |   No   | zarzon   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zaxxon               |  Yes   |  Yes   | Yes(1) |  Yes   |   No   | zaxxon   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zektor               |  Yes   | Close  |Partl(1)|  Yes   |   No   | zektor   |
|----------------------|--------|--------|--------|--------|--------|----------|
| Zoo Keeper           |  Yes   |  Yes   |  Yes   |  Yes   |   No   | zookeep  |
|----------------------|--------|--------|--------|--------|--------|----------|

(1) Needs samples provided separately
(2) Use the -fm option for OPL emulation instead of digital emulation
(3) Needs Sound Blaster OPL chip



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
Known issues:
- cocktail mode doesn't work. This might well be a bug in the original.



Arkanoid ("arkanoid")
=====================
Clones supported:
  US version ("arknoidu")
  bootleg ("arkbl2")
  BETA bootleg ("arkabeta")
  Tayto bootleg, not using the 68705 microcontroller ("arkatayt")

Known issues:
- Due to a missing image for the 68705 microcontroller, the BETA bootleg
  doesn't work.
- Due to features of the 68705 not emulated yet, the "arkbl2" bootleg set
  doesn't work.
- Since it doesn't have to emulate the 68705, the Tayto bootleg is faster than
  the other versions.
- Dual paddle in cocktail mode is not supported.



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



Astro Invader ("astinvad")
=============================
Clones supproted:
  Kamikaze ("kamikaze")



Atari Basketball ("bsktball")
=============================
The original hardware uses the Player 1 and Player 2 Start buttons
as the Jump/Shoot buttons.  I've taken button 1 and mapped it to the Start
buttons to keep people from getting confused.



Bad Dudes ("baddudes")
======================
Clones supported:
  Dragon Ninja ("drgninja")



Battlezone ("bzone")
====================
Clones supported:
  alternate version ("bzone2")

There are two control methods:
  1) E,I,D,K,Space: Close to the original controls (two up/down sticks)
  2) Arrow keys+CTRL: simulates a 4-way joystick



Berzerk ("berzerk")
===================
Clones supported:
  earlier, very rare version ("berzerk1")



Black Tiger ("blktiger")
========================
Clones supported:
  Black Dragon ("blkdrgon")



Blaster ("blaster")
===================
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup



Blockade ("blockade")
=====================
Clones supported:
  Comotion ("comotion")



Bomb Jack ("bombjack")
======================
Press fire to skip the ROM/RAM test at the beginning.

In the dip switch menu, DIFFICULTY 1 refers to the speed of the mechanical
bird, while DIFFICULTY 2 to the number and speed of enemies.
SPECIAL refers to how often the (E) and (S) coins appear.

Known issues:
- There is a bit in the sprite attributes which I don't know what means:
  it seems to be set only when the (B) materializes.
- The INITIAL HIGH SCORE setting doesn't only set that, it does something
  else as well - but I don't know what.



Boot Hill ("boothill")
======================
The mouse aims the gun



Bosconian ("bosco")
=====================
Midway copyright

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
Clones supported:
  bootleg version called "Bobble Bobble" ("boblbobl")
  another bootleg, which has a dip switch setting for Super Bobble Bobble
    ("sboblbob")

Known issues:
- Currently, the original version doesn't work at all due to the copy
  protection. Use the bootleg instead.
- Sound is not perfect yet, it gets out of sync soon and eventually die.
- In boblbobl, service mode works only if Language is set to Japanese. This is
  probably a "feature" of the bootleg.



Bubbles ("bubbles")
===================
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup

Clones supported:
  "red label" version ("bubblesr")



Bump 'n Jump ("bnj")
====================
In test mode, press 3 to advance through the various tests.

Clones supported:
  Burnin' Rubber ("brubber") the service ROM is missing for this one so
    service mode doesn't work.
  bootleg called Car Action ("caractn") no service ROM here either



Burger Time ("btime")
=====================
In test mode, press 3 to advance through the various tests.

Clones supported:
  different ROM set, without Midway copyright and different attract mode
     ("btimea")
  bootleg called Hamburger ("hamburge")

Known issues:
- hamburge doesn't work. The ROMs seem to be encrypted.



Capcom Bowling ("capbowl")
==========================
F2 enter service mode (press it on the high score screen)

Clones supported:
  Coors Light Bowling ("clbowl")

Known issues:
- The sound of the rolling ball is randomly wrong.



Centipede ("centipede")
=======================
Clones supported:
  rev 1 ("centipd1")

To see the color test, keep T pressed then hit F2. Insert a coin to proceed to
the convergence test.



Checkman ("checkman")
=====================
Use 1 and 2 to move the rows left and right.



Chelnov ("chelnov")
===================
Clones supported:
  Japanese version ("chelnovj")

Known issues:
- Partially working due to copy protection. After you lose the first life,
  no more enemies appear.



Choplifter ("chplftb")
======================
original, with no 8751 microcontroller (replaced by a jumper pack).

Clones supported:
  original with 8751 microcontroller ("chplft")
  bootleg ("chplftbl")

Known issues:
- chplft doesn't work because the 8751 is not emulated.
- Due to a limitation of the current SN76496 emulator, speech is played badly
  and at a very low volume.



Circus Charlie ("circusc")
==========================
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen

Clones supported:
  alternate version which allows to select the level to play ("circusc2")



City Connection ("citycon")
===========================
Known issues:
- In cocktail mode, player 2 uses the same controls as player 1. Maybe the
  port is multiplexed.



Colony 7 ("colony7")
====================
Clones supported:
  alternate version ("colony7a")



Commando (Capcom) ("commando")
==============================
Press fire during the startup message to enter the test screen

Clones supported:
  Japanese version 'Senjo no ookami' (The wolf of battlefield) ("commandj")

Known issues:
- some missing sprites here and there, probably caused by sprite multiplexing.



Congo Bongo ("congo")
=====================
Runs on almost the same hardware as Zaxxon.

Use F2 to enter test mode, then 1 to advance from one test to the following.

Clones supported:
  UK version, called Tip Top ("tiptop")

Known issues:
- What do the dip switches do?



Crazy Climber ("cclimber")
==========================
Clones supported:
  Japanese version ("ccjap")
  bootleg version ("ccboot")



Crazy Kong ("ckong")
====================
Clones supported:
  alternate version ("ckonga")
  bootleg version by Jeutel ("ckongjeu")
  bootleg version by Alca ("ckongalc")
  Monkey Donkey ("monkeyd")
  version running on Scramble hardware ("ckongs")

Known issues:
- music doesn't seem to work correctly in "ckongs".
- cocktail mode doesn't work in "ckongs" - the game resets after player 1 dies.
  This is likely a bug in the game.



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
- The self test doesn't pass the EERAM test. I don't know if all of the scores
  are supposed to rusvive a reset, or only the first three as it is now.



D-Day ("dday")
===========================
Press button 1 to fire. Press and hold button 2 to determine the
distance of the shot fired.



Defender ("defender")
===========================
The first time you run the game, it will say "factory settings restored".
To proceed, keep F1+F2 pressed, or just hit F3.

F2 tests
F1+F2 bookeeping/setup



Demolition Derby ("destderb")
=============================
MCR/III hardware
Press F2 before initial screen to configure

Track   Steer player 1
CTRL    Accelerate forward
ALT     Accelerate backward

Known issues:
- Players 2-4 aren't fully supported
- Due to missing ROMs from the Turbo Cheap Squeak board, sound doesn't work.



Dig Dug ("digdugnm")
====================
Namco copyright

Clones supported:
  Atari copyright ("digdugat")



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



Donkey Kong ("dkong")
=====================
Clones supported:
  japanese nintendo version ("dkongjp"). In this version barrels do not
  come down a ladder when you are standing at the top of it, and the
  levels play in the order barrels-pies-elevators-girders instead of
  barrels-girders-barrels-elevators-girders...



Donkey Kong Jr. ("dkongjr")
===========================
Clones supported:
  Japanese, maybe earlier version, with levels playing in the order 1-2-3-4
    instead of 1-4-1-3-4-1-2-3-4, a different title screen, 11 characters
	high scores, and no copyright notice ("dkngjrjp")
  Another Japanese (?) version, with levels playing in the order 1-2-3-4
    ("dkjrjp")
  bootleg version, with levels playing in the order 1-2-3-4 ("dkjrbl")



Donkey Kong 3 ("dkong3")
========================
F2      Service mode (keep it pressed)



Eggs ("eggs")
=============
Clones supported:
  Scrambled Egg ("scregg")



Elevator Action ("elevator")
============================
Clones supported:
  bootleg version ("elevatob")

Known issues:
- The original version doesn't work consistently due to the copy
  protection. Use the bootleg version instead.
- Some sprite priority problems (people half disappearing behind doors etc.)



Eliminator ("elim2")
====================
Two players version
Clones supported:
  four players version ("elim4")



Espial ("espial")
=================
Clones supported:
  european version ("espiale")



Exed Exes ("exedexes")
======================
Clones supported:
  Savage Bees ("savgbees")



Exerion ("exerion")
===================
Known issues:
- The background graphics are missing.
- Sometimes the game resets while you are playing.



Fast Freddie ("fastfred")
=========================
Clones supported:
  Fly-Boy (bootleg?) ("flyboy")

Known issues:
- Cocktail mode doesn't work in Fly-Boy. This seems a problem with the
  original.



Fire Trap ("firetrap")
======================
Clones supported:
  Japanese booleg ("firetpbl")

Known issues:
- Due to copy protection, the original version doesn't work. Use the bootleg
  instead.



Food Fight ("foodf")
====================
The first time you run the game, press Button 1 to advance from the "NVRAM
test fail" screen. The NVRAM will be automatically initialized.



Frisky Tom ("friskyt")
======================
Runs on the same (or similar) hardware as Seicross, but doesn't work due to
protection.



Frogger ("frogger")
===================
Clones supported:
  alternate version, smaller, with different help, but still (C) Sega 1981
     ("frogsega")
  another alternate version, running on different hardware (modified Moon
     Cresta?), but still (C) Sega 1981 ("frogger2")
  bootleg version, which runs on a modified Scramble board ("froggers")



Front Line ("frontlin")
=======================
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
  Another bootleg, with Namco copyright left in ("galagab2")

Known issues:
- Sometimes explosion sprites appear on the left of the screen.



Galaxian ("galaxian")
=====================
Original version with Namco copyright

  original with Midway copyright ("galmidw")
  and several bootlegs:
  one with Namco copyright ("galnamco")
  Super Galaxians ("superg")
  Galaxian Part X ("galapx")
  Galaxian Part 1 ("galap1")
  Galaxian Part 4 ("galap4")
  Galaxian Turbo ("galturbo")
  Defend the Terra Attack on the Red UFO ("redufo")

Known issues:
- The star background is probably not entirely accurate.
- Bullet placement is not correct in cocktail mode



Gauntlet ("gauntlet")
=====================
The start button is also used to use potions.

Clones suported:
  Intermediate Release 1 ("guantir1")
  Intermediate Release 2 ("guantir2")
  2 players version ("gaunt2p")

Known issues:
- The Slapstic protection MIGHT cause some level layouts to be screwed up. Let
  us know if you notice one.



Ghosts 'n Goblins ("gng")
=========================
Clones supported:
  alternate version with different graphics (the last weapon is a cross
    instead of a shield) ("gngcross")
  Japanese version 'Makai-mura' ("gngjap")

Known issues:
- To continue a game, insert a coin and keep fire pressed while pressing 1 (or 2)



Gorf ("gorf")
=============
Clones supported:
  Program 1 ("gorfpgm1")

Known issues:
- Colors are wrong in cocktail mode



Green Beret ("gberet")
======================
Clones supported:
  US version, called Rush'n Attack ("rushatck")

Known issues:
- The music starts with what seems a correct pitch, but changes after you die
  for the first time or finish the first level. Weird.



Gyruss ("gyruss")
===================
Konami copyright

Clones supported:
  licensed to Centuri ("gyrussce")



Gunsmoke ("gunsmoke")
====================
Capcom copyright

Clones supported:
  Romstar license ("gunsmrom")
  Japanese version ("gunsmokj")

Known issues:
- At the end of level 3 (Ninja Stars), if you go to the extreme right of the
  screen, the game resets. This could well be a bug of the original. The game
  doesn't seem to reset if you stay away from the right of the screen.



Hunchback ("hunchy")
====================
Runs on a modified Donkey Kong Jr. board.

Doesn't work at all due to the unsupported CPU.



Invinco ("invinco")
=================
Known issues:
- dip switches not verified



Invinco / Head On 2 dual game ("invho2")
========================================
Press button 2 to select the game.

Known issues:
- dip switches not verified



Iron Horse ("ironhors")
=======================
Clones supported:
  bootleg called Far West, running on different hardware (not working yet)
    ("farwest")

Known issues:
- Far West doesn't work.



Journey ("journey")
===================
Known issues:
- No support for the cassette music



Joust ("joust")
===============
white/green label version

The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup

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



Jump Coaster ("jumpcoast")
==========================
Known issues:
- The color PROMs are there, but association with the graphics is only
  partially correct.



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



Karnov ("karnov")
=================
Clones supported:
  Japanese version. Gameplay is different in this version. ("karnovj")



Kick Rider ("kickridr")
=======================
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
Known issues:
- The high score count page in the bookkeping section of service mode doesn't
  seem to work. Every score is registered as "More".



Kung Fu Master ("kungfum")
==========================
In slow motion mode, press 2 to slow game speed
In stop mode, press 2 to stop and 1 to restart
In level selection mode, press 1 to select and 2 to restart

Clones supported:
  bootleg version copyright by O.K. ("kungfub")



Legendary Wings ("lwings")
==========================
Clones supported:
  Japanese version ("lwingsjp")



Leprechaun ("leprechn")
=======================
Hold down F2 while pressing F3 to enter test mode. Hit Advance (F1) to
cycle through test and hit F2 to execute.



Lode Runner ("lrunner")
=======================
Known issues:
- apart from the other problems, dip switches are wrong.



Lotto Fun ("lottofun")
======================
Known issues:
- Sometimes you have to press F1 to proceed after the first screen.



Lunar Lander ("llander")
========================
Mouse emulates the thrust poti.

Known issues:
- Selftest does not work. It seems page 0 and 1 are mirrored, and
  the cpu emulation can't handle this correctly
- The language dip switch has no effect, this ROM set doesn't have support
  for that.


Lunar Rescue ("lrescue")
========================
Clones supported:
  Destination Earth ("desterth") (uses Space Invaders color scheme)

Known issues:
  A free credit is awarded on the first docking of the first game.



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
Clones supported:
  Japanese version ("mappyjp")



Marble Madness ("marble")
=========================
Clones supported:
  alternate version ("marblea")
  older version ("marble2")



Mario Bros. ("mario")
=====================
F2      Test (keep it pressed)



Mat Mania ("matmania")
======================
Clones supported:
  Exciting Hour ("excthour")
  Mania Challenge ("maniach")

Known issues:
- Most colors seem correct, but the audience graphics use purple instead of
  blue?
- Mania Challenge runs on slightly different hardware and doesn't work yet.



Mikie ("mikie")
===============
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen

Known issues:
- Some colors are not very convincing, they might be wrong.



Minefield ("minefld")
=====================
Press 1+F3 to enter test mode

Known issues:
- Colors are accurate apart from the background, which is an approximation.
- Separate controls for player 2 in cocktail mode are not supported.



Missile Command ("missile")
===========================
Clones supported:
  Super Missile Attack ("suprmatk")



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
  Eagle ("eagle")

Known issues:
- Dip switches for the alternate sets might be inaccurate.
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
  Williams license ("mpatrolw")
  bootleg version, called Moon Ranger ("mranger")

Known issues:
- Colors in test mode are not correct (black instead of R, G, B). The other
  colors in the game seem to be correct, so I don't know what's going on.
- Some problems with sound, which might be due to imperfect 6808 emulation.
- Sometimes, when you kill an enemy ship which is falling down, the ship and
  the score will stick on the screen for some time instead of disappearing.
  I don't know if this is a bug of the original.
- The background might not be entirely accurate



Mouse Trap ("mtrap")
====================
4+F3 to enter service mode



Mr. Do! ("mrdo")
================
CTRL+F3 Test mode

Clones supported:
  Version with additional Taito copyright ("mrdot")
  Mr. Lo! ("mrlo")
  Mr. Du! ("mrdu")



Mr. Do's Castle ("docastle")
============================
CTRL+F3 Test mode

Clones supported:
  Different (probably earier) version called Mr. Do vs the Unicorns. It has
     significant differences in the gameplay ("douni")
  Another alternate version, differences are unknown ("docastl2")



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
Known issues:
- some colors seem wrong, maybe the PROM is bad or I made some mistake in
  its interpretation.



New Zealand Story ("tnzs")
==========================
Clones supported:
  alternate version ("tnzs2")



Nibbler ("nibbler")
===================
Clones supported:
  alternate version ("nibblera")



Night Driver ("nitedrvr")
=========================
The gear and track displays are not a part of the original game, but have been
added for playability.



Ninja Gai Den ("gaiden")
========================
Clones supported:
  Shadow Warriors ("shadoww")



Pac-Land ("pacland")
====================
Midway version

Clones supported:
  Namco version ("paclandn")
  Alternate Namco version ("paclanda")



Pac Man ("pacman")
==================
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

Clones supported:
  Taito copyright ("phoenixt")
  T.P.N. copyright ("phoenix3")

Known issues:
- The startup tune is missing.



Pisces ("pisces")
=================
Known issues:
- The star background is probably not entirely accurate.
- What do the dip switches do?



Pleiads ("pleiads")
===================
Clones supported:
  Tehkan copyright ("pleitek")



Pooyan ("pooyan")
=================
Clones supported:
  bootleg version called Pootan ("pootan")



Popeye ("popeye")
=============================
Clones supported:
  bootleg version ("popeyebl")

Known issues:
- Due to encryption, the original doesn't work. Use the bootleg instead.
- You get about 7 bonus lives when you reach level 4.



Pulsar ("pulsar")
=================
Known issues:
- dip switches not verified



Punch Out ("punchout")
======================
The game runs on two monitors, positioned one above the other. Since the top
monitor is mostly informational, you might want to cut it out to enlarge the
play area. You can do that enforcing a suitable screen size. Good resolutions
are 400x300 (if your card supports it), or 800x600 with pixel doubling.

Clones supported:
  Super Punch Out ("spnchout")

Known issues:
- When you die, a rectangle on the left of the screen blinks.
- Due to copy protection, Super Punch Out doesn't work.



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
To enter your name in the high score list, use 1 or 2.

When the Auto Round Advance dip switch is On, use 1 or 2 to advance to the
following round. You also get infinite lives.

Clones supported:
  Japanese version ("qbertjp")



Q*Bert Qubes ("qbertqub")
=========================
To enter your name in the high score list, use 1 or 2.



Rainbow Islands ("rainbow")
===========================
Clones supported:
  Extra version ("rainbowe")

Known issues:
- Only partially working due to protection


Rally X ("rallyx")
==================
Clones supported:
  New Rally X ("nrallyx")

Known issues:
- Sprites are not turned off appropriately.
- Graphics placement in cocktail mode is not perfect.



Rastan ("rastan")
=================
Clones supported:
  Rastan Saga ("rastsaga")

Known issues:
- Crashes often.
- Rastan Saga doesn't always boot, use F3 to make it start.



Reactor ("reactor")
===================
1       Player 2 Energy
2       Player 2 Decoy

Known issues:
- Speech is not emulated


Red Baron ("redbaron")
======================
- Red Baron tries to calibrate its analog joystick at the start, so you'll
  have to move the "joystick" a bit before you can fly in all four directions.



Rescue ("rescue")
===================
Press 1+F3 to enter test mode

Known issues:
- Colors are accurate apart from the background, which is an approximation.



Return of the jedi ("jedi")
===========================
Known issues:
- background graphics are blocky because the hardware which smooths them is
  not ewmulated.
- sprite/background/text priority is not implemented
- this game has an analog stick. Control with keyboard or mouse doesn't work
  too well.



Robotron ("robotron")
===============
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup

Clones supported:
  yellow/orange label version, where quarks are incorrectly called Cubeoids
    during the demo ("robotryo")



Roc'n Rope ("rocnrope")
=======================
Clones supported:
  bootleg called Rope Man ("ropeman")

Knon issues:
- The bootleg version crashes when you start a game. This might be due to a
  slightly different encryption scheme.



Rygar ("rygar")
===============
Clones supported:
  Japanese version ("rygarj")



Scramble ("scramble")
=====================
Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Seicross ("seicross")
=====================
Runs on almost the same hardware as Crazy Climber, but not exactly the same.

Known issues:
- Doesn't work well; for example, you always start with 99 lives, and coins
  are only accepted from coin C (default key: 5). It looks like there is a
  microcontroller on board, which is not emulated.



Shao-Lin's Road ("shaolins")
============================
Known issues:
- Sometimes there is a blinking sprite in the bottom left corner of the high
  score entry screen.



Sidearms ("sidearms")
=====================
Clones supported:
  Japanese version ("sidearjp")

Known issues:
- The blinking star background is missing.



Silkworm ("silkworm")
=====================
On startup, press the 2P start button to use the test mode.



Sinistar ("sinistar")
=====================
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup



Space Attack ("sspaceat")
=========================
Note that this is NOT the same as "Space Attack II", which runs on Space
Invaders hardware.

Known issues:
- dip switches not verified. The game plays only in cocktail mode.



Space Duel ("spacduel")
=======================
Press "2" to choose level, the game won't start otherwise.



Space Firebird ("spacefb")
==========================
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
Clones supported:
  Super Earth Invasion ("earthinv")
  Space Attack II ("spaceatt")
  Invaders Revenge ("invrvnge")
  Galaxy Wars ("galxwars")
  Space Invaders Part 2 (Taito) - this one is a color game, unlike the
    others ("invadpt2")
  Space Phantoms ("spaceph")
  Cosmic Monsters ("cosmicmo")
  Zzap (not working) ("zzzap")

Known issues:
- The colors used in Invaders Revenge may be wrong.



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
Clones supported:
  alternate version, probably an earlier revision (it doesn't display the
    level number) ("panica")



Speed Rumbler ("srumbler")
==========================
Clones supported:
  alternate version ("srumblr2")

Known issues:
- The start button doesn't work in the alternate version
- Sprites should be buffered, like Commando. They jerk quite a bit when you
  complete a level.
- Tile palette does not get refreshed when you complete a level.
- Background for the level complete doesn't look correct. The background for
  the scoreboard should be either transparent or black.
- Semi-transparent tiles / sprites are not supported.



Splat ("splat")
===============
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup



Sprint 2 ("sprint2")
====================
Clones supported:
  Sprint 1 ("sprint1")

Known issues:
- Dip switches in Sprint 1 are wrong



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
The first time you run the game, it will say "factory settings restored".
Press F2 to proceed.

F2 tests
F1+F2 bookeeping/setup



Star Wars ("starwars")
======================
Known issues:
- Dip switches not functional, but you can change game settings through
  the self test menus.
- Sometimes, after the TIE level, instead of zooming into the Death Star the
  game sits there endlessly.



Strategy X ("stratgyx")
=======================
Stern version

Clones supported:
  bootleg, probably of the Konami version, different ("stratgyb")

Known issues:
- Dip switches not verified, especially in the bootleg version



Super Basketball ("sbasketb")
=============================
Hold down Start 1 & Start 2 keys to enter test mode on start up;
then use Start 1 to advance to the next screen



Super Cobra ("scobra")
======================
Stern copyright.

Clones supported:
  Konami copyright ("scobrak")
  bootleg version ("scobrab")

Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Super Pacman ("superpac")
=========================
Clones supported:
  Namco copyright ("superpcn")



Super Qix ("superqix")
======================
Clones supported:
  bootleg version ("sqixbl")

Known issues:
- Speed is wrong (much too slow)
- Due to protection, the original version doesn't work. Use the bootleg instead.



Super Zaxxon ("szaxxon")
========================
Known issues:
- The program ROMs are encrypted. The game seems to work, but there might be
  misbehaviours caused by imperfect decryption.



Swimmer ("swimmer")
===================
Clones supported:
  alternate version ("swimmera")

Known issues:
- The side panel should be lightish green / blue ( turquoise ) throughout the
  game, instead it is black.



Tapper ("tapper")
=================
Clones supported:
  Root Beer Tapper ("rbtapper")
  Suntory Tapper ("sutapper")



Tempest ("tempest")
===================
Mouse emulates spinner.

Clones supported:
  Tempest Tubes ("temptube")

Known issues:
- Several people complained that mouse control is reversed. This is not the
  case. The more obvious place where this can be seen is the level selection
  screen at the beginning: move the mouse right, the block goes right.
  Anyway, if you don't like the key assignments, you can change them.



Tetris (Atari) ("atetris")
==========================
To start a game, press the ROTATE button.

Clones supported:
  alternate version ("atetrisa")
  bootleg version ("atetrisb")
  cocktail version ("atetcktl")



The End ("theend")
==================
Known issues:
- The star background is probably not entirely accurate. Also, maybe it should
  be clipped at the top and bottom of the screen?



Three Stooges ("3stooges")
==========================
Up to 3 players can play at once. After inserting coins,
Button 1 Player 1 = start a 1 player game
Button 1 Player 2 = start a 2 players game
Button 1 Player 3 = start a 3 players game



Time Pilot ("timeplt")
======================
Clones supported:
  bootleg version ("spaceplt")



Time Pilot 84 ("tp84")
======================
Known issues:
- sound is quite wrong
- some sprites are displayed with wrong colors



Track & Field ("trackfld")
==========================
Clones supported:
  Hyper Olympic ("hyprolym")



Triple Punch ("triplep")
========================
Known issues:
- sometimes the game resets after you die. This is probably caused by the
  copy protection.



Trojan ("trojan")
=================
Clones supported:
  Japanese version ("trojanj")



Turtles ("turtles")
===================
Clones supported:
  Turpin ("turpin")



Tutankham ("tutankhm")
======================
Konami copyright.

Clones supported:
  Stern copyeight ("tutankst")



Two Tigers ("twotiger")
=======================
Known issues:
- Two players not well supported



Uniwars ("uniwars")
===================
Clones supported:
  The original Japanese version, Gingateikoku No Gyakushu ("japirem")

Known issues:
- The star background is probably not entirely accurate.



Vastar ("vastar")
=================
Known issues:
- Some wrong sprite graphics (e.g. when you die)



Venture ("venture")
===================
3+F3    Test mode

Clones supported:
  alternate version ("venture2")



Video Hustler ("hustler")
=========================
Clones supported:
  Pool ("pool")

Known issues:
- Pool doesn't work due to a different encryption.



Warlords ("warlord")
====================
Q,W   Move player 1
I,O   Move player 2
1,2   Fire player 1/2

Known issues:
- The movement keys cannot be remapped.



Wild Western ("wwestern")
=========================
Known issues:
- Probably due to copy protection, the game is not playable and resets often.



Wizard of Wor ("wow")
=====================
The original machine had a special joystick which could be moved either
partially or fully in a direction. Pushing it slightly would turn around the
player without making it move. The emulator assumes that you are always
pushing the joystick fully, to simulate the "half press" you can press Alt.



World Cup 90 ("wc90")
=====================
Clones supported:
  bootleg version ("wc90b")

Known issues:
- No ADPCM samples.
- The second YM2203 starts outputting static after a few seconds. This is in
  the original version, the bootleg doesn't have a second YM2203.



Xain'd Sleena ("xsleena")
=========================
Clones suported:
  Solar Warriow ("solarwar")

Known issues:
- several.



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
name using F5/F6 and F2, then F1 to proceed through all the configuration
screens, or just reset (F3).

Known issues:
- Every time you start the emulation, you get a free credit. This is the
  correct behaviour and has been verified on a real machine.



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
8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
TMS5220 emulator by Frank Palazzolo.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203 and YM-2151 emulation by Tatsuyuki Satoh.
OPL based YM-2203 emulation by Ishmair (ishmair@vnet.es).
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
NES sound hardware info by Jeremy Chadwick and Hedley Rainne.

Allegro library by Shawn Hargreaves, 1994/97
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c.
"inflate" code for zip file support by Mark Adler.

Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention them all.

Many thanks to Nicholas Alwin (www.alpha1.net/~v-stick) for the V-Stick. If you
   are seriously into arcade emulation, check it out.


Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -soundcard 0    will run Ms Pac Man without sound


options:
-scanlines/-noscanlines (default: -scanlines)
              if the default mode doesn't work with your monitor/video card
              (double image/picture squashed vertically), use -noscanlines
              or -vesa.
-vesa/-novesa (default: -novesa)
              Decides wether to use tweaked VGA modes or standard VESA modes.
              Note that some hires games require VESA modes, -novesa is
              ignored in this case.
-ntsc         a 288x224 mode with standard NTSC frequencies. You need some
              additional hardware (VGA2TV converter) to make use of this.
-vesa1        discontinued. use -vesa
-vesa2b       discontinued. use -vesa
-vesa2l       discontinued. use -vesa
-resolution XxY
              where X and Y are width and height (ex: '-resolution 800x600')
              Mame goes some lengths to autoselect a good resolution. You can
              override Mame's choice with this option.
              You can use -XxY (e.g. -800x600) as a shortcut. Frontend
              authors are advised to use -resolution XxY, however.
-320          discontinued. Use -320x240.
              If you get an error '320x240 not supported', you probably
              need Scitech's Display Doctor, which provides the 'de facto'
              standard VESA implementaion (http://www.scitechsoft.com)
              Note: this is a nice alternative to '-640x480 -noscanlines'
-400          same as above, use -400x300
-512          same as above, use -512x384
-640          same as above, use -640x480
-800          same as above, use -800x600.
-1024         same as above, use -1024x768
-skiplines N  since many games use a screen taller than 240 lines,
              they won't fit in the screen. The parameter 'N' sets
              the initial number of lines to skip at the top of
              the screen. You can adjust the position while the game
              is running using the PGUP and PGDN keys.
-skipcolumns N same as above but moves the screen horizontally. This is rarely
              used since the screen usually fits in the screen. To adjust the
              position at run time, use SHIFT + PGUP/PGDN.
-double/-nodouble (default: auto)
              use nodouble to disable pixel doubling in VESA modes (faster,
              but smaller picture). Use double to force pixel doubling when
              the image doesn't fit in the screen (you'll have to use PGUP and
              PGDN to scroll).
-depth n      (default: 16)
              Some games need 65k color modes to get accurate graphics. To
              improve speed, you can turn that off using -depth 8, which limits
              to the standard 256 color modes.
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

-norotate     This is supposed to disable all internal rotations of the image,
              therefore displaying the video output as it is supposed to be
              (so you need a vertical monitor to see vertical games). However,
              many drivers still don't use MAME centralized rotation, but
              instead rotate the image on their own, so -norotate has no
              effect on them. In some cases, the image will be upside down. To
              correct that, use
              -norotate -flipx -flipy.

-soundcard n  select sound card (if this is not specified, you will be asked
              interactively)
-sr n         set the audio sample rate. The default is 22050. Smaller values
              (e.g. 11025) will cause lower audio quality but faster emulation
			  speed. Higher values (e.g. 44100) will cause higher audio quality
			  but slower emulation speed.
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
              every time. When calibrating two analog joysticks, move
              both at the same time!
-fm/-nofm (default: -nofm) use the SoundBlaster OPL chip for music emulation
              in some games. This is faster, but emulation is less faithful.
-log          create a log of illegal memory accesses in ERROR.LOG
-help, -?     display current mame version and copyright notice
-list         display a list of currently supported games
-listfull     display a list of game directory names + description
-listroms     display selected game required roms
-listsamples  display selected game required samples
-verifyroms   check selected game for missing and invalid ROMs.
-verifysamples check selected game for missing samples.
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
-antialias/-noantialias (default: -antialias)
              antialiasing for the vector games.
-beam n       sets the width in pixels of the vectors. n is a float in the
              range of 1.00 through 16.00.
-flicker n    make the vectors flicker. n is an optional argument, a float in
              the range range 0.00 - 100.00 (0=none 100=maximum).
-cheat        Cheats like the speedup in Pac Man or the level skip in many
              other games are disabled by default. Use this switch to turn
              them on.
-debug        Activate the integrated debugger. During the emulation, press
              tilde to enter the debugger.
-record name   Record joystick input on file INP/name.inp.
-playback name Playback joystick input from file INP/name.inp.
-savecfg      no longer supported at the moment
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
F10          Toggle speed throttling
F11          Toggle speed display
F12          Save a screen snapshot. The default target directory is PCX, you
             have to create it yourself, it will not be created by the program
             if it isn't there.
numpad +/-   Volume adjust
left shift + numpad +/- Gamma correction adjust
ESC          Exit emulator
