
              M.A.M.E.  -  Multiple Arcade Machine Emulator
     Copyright (C) 1997, 1998, 1999  by Nicola Salmoria and the MAME team

Please note that many people helped with this project, either directly or by
releasing source code that was used to write the drivers. We are not trying to
appropriate merit that isn't ours. See the acknowledgments section for a list
of contributors, however please note that the list is largely incomplete. See
also the CREDITS section in the emulator to see the people who contributed to a
specific driver. Again, that list might be incomplete. We apologize in advance
for any omission.

All trademarks cited in this document are property of their respective owners.


Usage and Distribution License
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
   ROM images are copyrighted material, and most of them cannot be distributed
   freely. Distribution of MAME on the same physical medium as illegal copies
   of ROM images is strictly forbidden.
   You are not allowed to distribute MAME in any form if you sell, advertise or
   publicize illegal CD-ROMs or other media containing ROM images. Note that
   the restriction applies even if you don't directly make money out of that.
   You are allowed to make ROMs and MAME available for download on the same web
   site, but only if you warn users about the copyright status of the ROMs and
   make it clear that they must not download the ROMs unless they are entitled
   to do so.

IV. Source Code Distribution
----------------------------
   If you distribute the binary, you should also distribute the source code. If
   you can't do that, you must provide a pointer to a place where the source
   can be obtained.

V. Distribution Integrity
-------------------------
   This chapter applies to the official MAME distribution. See below for
   limitations on the distribution of derivative works.
   MAME must be distributed only in the original archives. You are not allowed
   to distribute a modified version, nor to remove and/or add files to the
   archive.

VI. Reuse of Source Code
--------------------------
   This chapter might not apply to specific portions of MAME (e.g. CPU
   emulators) which bear different copyright notices.
   The source code cannot be used in a commercial product without a written
   authorization of the authors. Use in non commercial products is allowed and
   indeed encouraged; however if you use portions of the MAME source code in
   your program, you must make the full source code freely available as well.
   Usage of the _information_ contained in the source code is free for any use.
   However, given the amount of time and energy it took to collect this
   information, we would appreciate if you made the additional information you
   might have freely available as well.

VII. Derivative Works
---------------------
   Derivative works are allowed (provided source code is available), but
   discouraged: MAME is a project continuously evolving, and you should, in
   your best interest, submit your contributions to the development team, so
   that they are integrated in the main distribution.
   There are some trivial modifications to the source code that anybody could
   do, but go against the spirit of the project. They are NOT considered a
   derivative work, and distribution of executables with them applied is
   strictly forbidden. Such changes include, but are not limited to:
   - enabling games that are disabled
   - changing the ROM verification commands so that they report missing games
   - removing the startup information screens
   If you make a derivative work, you are not allowed to call it MAME. You must
   use a different name to make clear that it is a MAME derivative, but it isn't
   an official distribution from the MAME team. Simply calling it MAME followed
   or preceded by a punctuation (e.g. MAME+) will not be enough. The name must
   be clearly distinct, e.g. REMAME. The version number must also match the one
   of the official MAME you derived your version from.


How to Contact Us
-----------------

The official MAME home page is http://www.mame.net/. You can always find the
latest release there, including beta versions and information on things being
worked on. Also, a totally legal and free ROM set of Robby Roto is available
on the same page.

If you have bugs to report, check the MAME Testing Project at
http://mameworld.retrogames.com/mametesters

Here are some of the people contributing to MAME. If you have comments,
suggestions or bug reports about an existing driver, check the driver's Credits
section to find who has worked on it, and send comments to that person. If you
are not sure who to contact, write to Nicola. If you have comments specific to
a system other than DOS (e.g. Mac, Win32, Unix), they should be sent to the
respective port maintainer (check the documentation to know who he is). DON'T
SEND THEM TO NICOLA - they will be ignored.

Nicola Salmoria    MC6489@mclink.it

Mike Balfour       mab22@po.cwru.edu
Aaron Giles        agiles@sirius.com
Chris Moore        chris.moore@writeme.com
Brad Oliver        bradman@pobox.com
Andrew Scott       ascott@utkux.utcc.utk.edu
Zsolt Vasvari      vaszs01@banet.net

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc) or
on the official MAME message board, http://www.mame.net/msg/.

For help in compiling MAME, check this page:
http://mameworld.retrogames.com

Also, DO NOT SEND REQUESTS FOR NEW GAMES TO ADD, unless you have some original
info on the game hardware or, even better, own the board and have the technical
expertise needed to help us.
Please don't send us information widely available on the Internet - we are
perfectly capable of finding it ourselves, thank you.



Acknowledgments
---------------

First of all, thanks to Allard van der Bas (avdbas@wi.leidenuniv.nl) for
starting the Arcade Emulation Programming Repository at
http://valhalla.ph.tn.tudelft.nl/emul8
Without the Repository, I would never have even tried to write an emulator.
Unfortunately, the original Repository is now closed, but its spirit lives
on in MAME.

Z80 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
M6502 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
Hu6280 Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M6808 based on L.C. Benschop's 6809 Simulator V09.
M68000 emulator Copyright 1999 Karl Stenerud.  All rights reserved.
80x86 M68000 emulator Copyright 1998, Mike Coates, Darren Olafson.
8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
T-11 emulator Copyright (C) Aaron Giles 1998
TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.
TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.
Cinematronics CPU emulator by Jeff Mitchell, Zonn Moore, Neil Bradley.
Atari AVG/DVG emulation based on VECSIM by Hedley Rainnie, Eric Smith and
Al Kossow.

TMS5220 emulator by Frank Palazzolo.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203, YM-2151, YM3812 emulation by Tatsuyuki Satoh.
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
NES sound hardware info by Jeremy Chadwick and Hedley Rainne.
YM2610 emulation by Hiromitsu Shioya.

Background art by Peter Hirschberg (PeterH@cronuscom.com).

Allegro library by Shawn Hargreaves, 1994/97
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c.
"inflate" code for zip file support by Mark Adler.

Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things
   to mention them all.

Thanks to Brian Deuel, Neil Bradley and the Retrocade dev team for allowing us
to use Retrocade's game history database. [John Butler]


Usage
-----

MAME [name of the game to run] [options]

for example

MAME mspacman -soundcard 0    will run Ms Pac Man without sound


options:
-tweak/-notweak (default: notweak)
              MAME supports a large number of tweaked VGA modes, with
              resolutions matching the ones of the emulated games. Those modes
              look MUCH better than VESA modes (and usually faster), but may
              have compatibility problems with some video cards and monitors,
              so they are not enabled by default. You should by all means use
              -tweak if your hardware supports it. Note that some modes might
              work and other might not, e.g. your card could support 256x256
			  but not 384x224. In that case you'll have to turn -tweak on or
              off depending on the game you run. -noscanlines can also solve
              many compatibility problems.
-ntsc         a 288x224 mode with standard NTSC frequencies. You need some
              additional hardware (VGA2TV converter) to make use of this.
-vesamode vesa1/vesa2b/vesa2l/vesa3
              Forces the VESA mode to use. By default the best available one
              is used, but some video cards might crash during autodetection,
              so you should force a lower standard (start with vesa1 and go
              upwards until it stops working).
-resolution XxY
              where X and Y are width and height (ex: '-resolution 800x600')
              MAME goes some lengths to autoselect a good resolution. You can
              override MAME's choice with this option.
              You can use -XxY (e.g. -800x600) as a shortcut. Frontend
              authors are advised to use -resolution XxY, however.
-skiplines N / -skipcolumns N
              if you run a game on a video mode smaller than the visible area,
              you can adjust its position using the PGUP and PGDN keys (alone
              for vertical panning, shifted for horizontal panning).
              You can also use these two parameters to set the initial
              position: 0 is the default, menaing that the screen is centered.
              You can specify both positive and negative offsets.
-scanlines/-noscanlines (default: -scanlines)
              turns on or off visible scanlines, which give an image more
              similar to that of a real arcade monitor.
-stretch/-nostretch (default: stretch)
              use nostretch to disable pixel doubling in VESA modes (faster,
              but smaller picture).
-depth n      (default: auto)
              Some games need 16-bit color modes to get accurate graphics. To
              improve speed, you can turn that off using -depth 8, which limits
              to the standard 256 color modes. You can also use -depth 16 to
              force games to use a 16-bit diplay even if they fit in 256 colors,
              but this isn't suggested.
-gamma n      (default: 1.0)
              Set the initial gamma correction value.
-vgafreq n    where n can be 0 (default) 1, 2 or 3.
              use different frequencies for the custom video modes. This
              could reduce flicker, especially in the 224x288noscanlines
              mode. WARNING: THE FREQUENCIES USED MIGHT BE WAY OUTSIDE OF
              YOUR MONITOR RANGE, AND COULD EVEN DAMAGE IT. USE THESE OPTIONS
              AT YOUR OWN RISK.
-vsync/-novsync (default: -novsync)
              synchronize video display with the video beam instead of using
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
-alwayssynced/-noalwayssynced (default: -noalwayssynced)
              for many tweaked VGA modes, MAME has two definitions: one which
              is more compatible, and one which is less compatible but uses
              frequencies compatible with -vsync. By default, the less
              compatible definition is used only when -vsync is requested;
              using this option, you can force it to be used always.
-triplebuffer/-notriplebuffer (default: -notriplebuffer)
              Enables triple buffering with VESA modes. This is faster than
              -vsync, but doesn't work on all cards and, even it does remove
              tearing during scrolling, it might not be as smooth as vsync.
-monitor NNNN (default: standard)
              selects the monitor type:
              standard: standard PC monitor
              ntsc:     NTSC monitor
              pal:      PAL monitor
              arcade:   arcade monitor
-centerx N and -centery N
              each take a signed value (-8 to 8 for centerx, -16 to 16 for
              centery) and let you shift the low scanrate modes (monitor=ntsc,
                pal,arcade) around.
-waitinterlace
              forces update of both odd and even fields of an interlaced low
              scanrate display (monitor=ntsc,pal,arcade) for each game loop.
-ror          rotate the display clockwise by 90 degrees.
-rol          rotate display anticlockwise
-flipx        flip display horizontally
-flipy        flip display vertically
              -ror and -rol provide authentic *vertical* scanlines, given you
              turn your monitor to its side.
              CAUTION:
              A monitor is a complicated, high voltage electronic device.
              There are some monitors that were designed to be rotated.
              If yours is _not_ one of those, but you absolutely must
              turn it to its side, you do so at your own risk.

              ******************************************************
              PLEASE DO NOT LET YOUR MONITOR WITHOUT ATTENTION IF IT
              IS PLUGGED IN AND TURNED TO ITS SIDE
              ******************************************************

-norotate     This is supposed to disable all internal rotations of the image,
              therefore displaying the video output as it is supposed to be
              (so you need a vertical monitor to see vertical games). In some
              cases, the image will be upside down. To correct that, use
              -norotate -flipx -flipy.
-frameskip n (default: auto)
              skip frames to speed up the emulation. The argument is the number
              of frames to skip out of 12. For example, if the game normally
              runs at 60 fps, "-frameskip 2" will make it run at 50 fps, and
              "-frameskip 6" at 30 fps. Use F11 to check the speed your
              computer is actually reaching. If it is below 100%, increase the
              frameskip value. You can press F9 to change frameskip while
              running the game.
			  When set to auto (the default), the frameskip setting is
              dynamically adjusted at run time to display the maximum possible
              frames without dropping below 100% speed.
-antialias/-noantialias (default: -antialias)
              antialiasing for the vector games.
-beam n       sets the width in pixels of the vectors. n is a float in the
              range of 1.00 through 16.00.
-flicker n    make the vectors flicker. n is an optional argument, a float in
              the range 0.00 - 100.00 (0=none 100=maximum).
-translucency/-notranslucency (default: -translucency)
              enables or disables vector translucency.

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
-stereo/-nostereo (default: -stereo)
              enables stereo output for games supporting it.
-volume n     (default: 0) set the startup volume. It can later be changed
              using the On Screen Display. The volume is an attenuation in dB,
              e.g. "-volume -12" will start with a -12dB attenuation.
-ym3812opl/-noym3812opl (default: -noym3812opl) use the SoundBlaster OPL chip for
              music emulation of the YM3812 chip. This is faster, and is
			  reasonably accurate since the chips are 100% compatible. However,
              there is no control on the volume, and you need a real OPL chip
			  for it to work (if you are using a SB compatible card that
			  emulates the OPL in software, the built in digirtal emulation
			  will probably sound better).

-joy name (default: none) allows joystick input, name can be:
              none         - no joystick
              auto         - attempts auto detection
              standard     - normal 2 button joystick
              dual         - dual joysticks
              4button      - Stick/Pad with 4 buttons
              6button      - Stick/Pad with 6 buttons
              8button      - Stick/Pad with 8 buttons
              fspro        - CH Flightstick Pro
              wingex       - Wingman Extreme
              wingwarrior  - Wingman Warrior
              sidewinder   - Microsoft Sidewinder (up to 4)
              gamepadpro   - Gravis GamePad Pro
              grip         - Gravis GrIP
              grip4        - Gravis GrIP constrained to only move along the
			                 four main axis
              sneslpt1     - SNES pad on LPT1 (needs special hardware)
              sneslpt2     - SNES pad on LPT2 (needs special hardware)
              sneslpt3     - SNES pad on LPT3 (needs special hardware)
              psxlpt1      - PSX pad on LPT1 (needs special hardware)
              psxlpt2      - PSX pad on LPT2 (needs special hardware)
              psxlpt3      - PSX pad on LPT3 (needs special hardware)
              n64lpt1      - N64 pad on LPT1 (needs special hardware)
              n64lpt2      - N64 pad on LPT2 (needs special hardware)
              n64lpt3      - N64 pad on LPT3 (needs special hardware)

              Notes:
              1) Use the TAB menu to calibrate analog joysticks. Calibration
              data will be saved in mame.cfg. If you're using different
              joytypes for different games, you may need to recalibrate your
              joystick every time.
              2) Extra buttons of noname joysticks may not work.
			  3) the "official" Snespad-Support site is
              http://snespad.emulationworld.com
              4) http://www.debaser.force9.co.uk/ccmame has info on how to
              connect PSX and N64 pads.

-hotrod       sets a default keyboard configuration suitable for the HotRod
              joystick by HanaHo Games.

-log          create a log of illegal memory accesses in ERROR.LOG
-help, -?     display current MAME version and copyright notice
-list         display a list of currently supported games
-listfull     display a list of game directory names + description
-listroms     display selected game required roms
-listsamples  display selected game required samples
-listdetails  display a detailed list of drivers and the hardware they use
-listgames    list the supported games, year, manufacturer
-listinfo     list comprehensive details for all of the supported games
-listclones   list all clones of the specified game
-noclones     used together with the list commands, doesn't list alternate
              versions of the same game
-verifyroms   check selected game(s) for missing and invalid ROMs. * checks all
              available games.
-verifysets   check selected game(s) and report their status. * checks all
              available games.
-verifysamples check selected game for missing samples.
-romdir       specify an alternate directory/zip name where to load the ROMs
              for the specified game. E.g. "mame pacman -romdir pachack" will
              run the Pac Man driver but load the roms from the "pachack" dir
              or "pachack.zip" archive.
-mouse/-nomouse (default: -mouse) enable/disable mouse support
-cheat        Cheats like the speedup in Pac Man or the level skip in many
              other games are disabled by default. Use this switch to turn
              them on.
-debug        Activate the integrated debugger. During the emulation, press
              tilde to enter the debugger. This is only available if the
              program is compiled with MAME_DEBUG defined.
-record name   Record joystick input on file INP/name.inp.
-playback name Playback joystick input from file INP/name.inp.
-savecfg      no longer supported at the moment
-ignorecfg    ignore mame.cfg and start with the default settings


Keys
----
Tab          Toggles the configuration menu
Tilde        Toggles the On Screen Display
             Use the up and down arrow keys to select the parameter (global
             volume, mixing level, gamma correction etc.), left and right to
             arrow keys to modify it.
P            Pause
Shift+P      While paused, advance to next frame
F3           Reset
F4           Show the game graphics. Use cursor keys to change set/color,
             F4 or Esc to return to the emulation.
F9           Change frame skip on the fly
F10          Toggle speed throttling
F11          Toggle speed display
Shift+F11    Toggle profiler display
F12          Save a screen snapshot. The default target directory is SNAP, you
             have to create it yourself, it will not be created by the program
             if it isn't there.
ESC          Exit emulator
