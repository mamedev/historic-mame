
              M.A.M.E.  -  Multiple Arcade Machine Emulator
       Copyright (C) 1997, 1998  by Nicola Salmoria and the MAME team

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
   You are not allowed to distribute MAME and ROM images on the same physical
   medium. You are allowed to make them available for download on the same web
   site, but only if you warn users about the copyright status of the ROMs and
   the legal issues involved. You are NOT allowed to make MAME available for
   download together with one giant big file containing all of the supported
   ROMs, or any files containing more than one game each.
   You are not allowed to distribute MAME in any form if you sell, advertise or
   publicize illegal CD-ROMs or other media containing ROM images. Note that
   the restriction applies even if you don't directly make money out of that.
   The restriction of course does not apply if the CD-ROMs are published by the
   ROMs copyrights owners.

IV. Source Code Distribution
----------------------------
   If you distribute the binary, you should also distribute the source code. If
   you can't do that, you must provide a pointer to a place where the source
   can be obtained.

V. Distribution Integrity
-------------------------
   This chapter applies to the official MAME distribution. See next chapter for
   limitations on the distribution of derivative works.
   MAME must be distributed only in the original archives. You are not allowed
   to distribute a modified version, nor to remove and/or add files to the
   archive.

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
Zsolt Vasvari      vaszs01@banet.net
Bernd Wiebelt      bernardo@studi.mathematik.hu-berlin.de

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc) or
on the official MAME message board, http://www.macmame.org/wwwboard/mame/.
For help in compiling MAME, check this page:
http://seagate.cns.net.au/~ben/emu/compilemame.html

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

Z80Em Portable Zilog Z80 Emulator Copyright (C) Marcel de Kogel 1996,1997
   Note: the version used in MAME is slightly modified. You can find the
   original version at http://www.komkon.org/~dekogel/misc.html.
M6502 emulator by Juergen Buchmueller.
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M6808 based on L.C. Benschop's 6809 Simulator V09.
80x86 asm M6808 emulator Copyright 1998, Neil Bradley, All rights reserved
M68000 emulator taken from the System 16 Arcade Emulator by Thierry Lescot.
8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
T-11 emulator Copyright (C) Aaron Giles 1998
TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.
TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.

TMS5220 emulator by Frank Palazzolo.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203 and YM-2151 emulation by Tatsuyuki Satoh.
OPL based YM-2203 emulation by Ishmair (ishmair@vnet.es).
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information
   on the Pokey random number generator.
NES sound hardware info by Jeremy Chadwick and Hedley Rainne.
YM3812 and YM3526 emulation by Carl-Henrik Skårstedt.
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
              Decides whether to use tweaked VGA modes or standard VESA modes.
              Note that some hires games require VESA modes, -novesa is
              ignored in this case.
-ntsc         a 288x224 mode with standard NTSC frequencies. You need some
              additional hardware (VGA2TV converter) to make use of this.
-vesa1        discontinued. use -vesa
-vesa2b       discontinued. use -vesa
-vesa2l       discontinued. use -vesa
-resolution XxY
              where X and Y are width and height (ex: '-resolution 800x600')
              MAME goes some lengths to autoselect a good resolution. You can
              override MAME's choice with this option.
              You can use -XxY (e.g. -800x600) as a shortcut. Frontend
              authors are advised to use -resolution XxY, however.
-320          discontinued. Use -320x240.
              If you get an error '320x240 not supported', you probably
              need Scitech's Display Doctor, which provides the 'de facto'
              standard VESA implementation (http://www.scitechsoft.com)
              Note: this is a nice alternative to '-640x480 -noscanlines'
-400          same as above, use -400x300
-512          same as above, use -512x384
-640          same as above, use -640x480
-800          same as above, use -800x600.
-1024         same as above, use -1024x768
-skiplines N / -skipcolumns N
              if you run a game on a video mode smaller than the visible area,
              you can adjust its position using the PGUP and PGDN keys (alone
              for vertical panning, shifted for horizontal panning).
              You can also use these two parameters to set the initial
              position: 0 is the default, menaing that the screen is centered.
              You can specify both positive and negative offsets.
-double/-nodouble (default: auto)
              use nodouble to disable pixel doubling in VESA modes (faster,
              but smaller picture). Use double to force pixel doubling when
              the image doesn't fit in the screen (you'll have to use PGUP and
              PGDN to scroll).
-depth n      (default: 16)
              Some games need 65k color modes to get accurate graphics. To
              improve speed, you can turn that off using -depth 8, which limits
              to the standard 256 color modes.
-gamma n      (default: 1.2)
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
-stereo/-nostereo (default: -stereo)
              enables stereo output for games supporting it.
-joy n/-nojoy (default: -nojoy) allows joystick input, n can be:
              0 - no joystick
              1 - normal 2 button joystick
              2 - dual joysticks
              3 - Stick/Pad with 4 buttons
              4 - Stick/Pad with 6 buttons
              5 - Stick/Pad with 8 buttons
              6 - CH Flightstick Pro
              7 - Wingman Extreme (or Wingman Warrior without spinner)
              8 - Microsoft Sidewinder (up to 4)
              9 - Gravis GamePad Pro

              Press F7 to calibrate analog joysticks. Calibration data
              will be saved in mame.cfg. If you're using different joytypes
              for different games, you may need to recalibrate your joystick
              every time.

-ym2203opl/-noym2203opl (default: -noym2203opl) use the SoundBlaster OPL chip
              for music emulation of the YM2203 chip. This is faster, but
              emulation is less faithful.
-ym3812opl/-noym3812opl (default: -ym3812opl) use the SoundBlaster OPL chip for
              music emulation of the YM3812 chip. This is faster, and is
              reasonably accurate since the chips are 100% compatible. However,
              the pitch might be wrong. Also note that with -no3812opl you need
              some external drum samples.
-log          create a log of illegal memory accesses in ERROR.LOG
-help, -?     display current MAME version and copyright notice
-list         display a list of currently supported games
-listfull     display a list of game directory names + description
-listroms     display selected game required roms
-listsamples  display selected game required samples
-listdetails  display a detailed list of drivers and the hardware they use
-listgames    list the supported games, year, manufacturer
-listinfo     list comprehensive details for all of the supported games
-verifyroms   check selected game for missing and invalid ROMs.
-verifysamples check selected game for missing samples.
-romdir       specify an alternate directory/zip name where to load the ROMs
              for the specified game. E.g. "mame pacman -romdir pachack" will
              run the Pac Man driver but load the roms from the "pachack" dir
              or "pachack.zip" archive.
-mouse/-nomouse (default: -mouse) enable/disable mouse support
-frameskip n  skip frames to speed up the emulation. For example, if the game
              normally runs at 60 fps, "-frameskip 1" will make it run at 30
              fps, and "-frameskip 2" at 20 fps. Use F11 to check the speed
              your computer is actually reaching. If the game is too slow,
              increase the frameskip value. Note that this setting can also
              affect audio quality (some games sound better, others sound
              worse).  Maximum value for frameskip is 3.
-antialias/-noantialias (default: -antialias)
              antialiasing for the vector games.
-beam n       sets the width in pixels of the vectors. n is a float in the
              range of 1.00 through 16.00.
-flicker n    make the vectors flicker. n is an optional argument, a float in
              the range 0.00 - 100.00 (0=none 100=maximum).
-translucency/-notranslucency (default: -translucency)
              enables or disables vector translucency.
-cheat        Cheats like the speedup in Pac Man or the level skip in many
              other games are disabled by default. Use this switch to turn
              them on.
-debug        Activate the integrated debugger. During the emulation, press
              tilde to enter the debugger.
-record name   Record joystick input on file INP/name.inp.
-playback name Playback joystick input from file INP/name.inp.
-savecfg      no longer supported at the moment
-ignorecfg    ignore mame.cfg and start with the default settings


Keys
----
Tab          Toggles the configuration menu
Tilde        Toggles the On Screen Display
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
ESC          Exit emulator
