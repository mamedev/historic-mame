This archive contains an mcs48 (8039, 8048, etc) simulator and disassembler.

The simulator was designed as part of an arcade "Gyruss" emulator (see
below for URL).  The arcade gyruss has an 8039 slaved to a Z80.  The Z80
writes commands via an I/O port to a byte-wide latch, and then interrupts
the 8039 by writing to another port.  The 8039 code reads the latch, and
uses it as a 'drum number' (or 'command').  The simulator will create
files "audioNN.out" where NN is a number from 00 to 08 (the valid "drum"
numbers).


Without the GYRUSS roms (not provided here, and I won't tell you where
I got them from ... if you're clever, you can figure it out, if not
ask around ;-), the simulator will be of little use in it's
current form.  Feel free to rip, shred, fold, spindle, and mutilate the
code.  Please give me credit if you use it in a product.  If you use
it in a product you plan on charging money for, you must get my permission
in writing, first. (I'll give it, I just want to know ... )

Check out the makefile and the file 'cmds.mcs' for some (small amount)
of usage information.

This code was a weekend hack to get the drum sounds that I needed.  The
drum sound output sucks because the real 8039 was connected to an imperfect
and rather forgiving R2R ladder for a DAC, not the "highly precise" DAC
that we all have on our sound blasters and GUS'  ...

Enjoy.

--------------------------------------------------------------------------

The mcs-48 disassembler usage is simple:

	dis39 <romfile> [ <start> [ <len> ] ]

Output is to stdout.

It's not terribly efficient, but it's a reasonable framework to build
a disassembler for other simple microcontrollers (Microchip PIC comes to
mind).

The skeleton/calling convention of the DAsm() function came from
Marat Fayzullin's Z80 disassembler 'dasm'.

This software is Copyright 1996 Michael Cuddy, Fen's Ende Software, but
May be freely distributed as long as the copyright notices remain intact.

Consider it a small gift to the emulation community for thier fine work
in preserving Arcade & Computer History

Contact Information:
	mcuddy@FensEnde.com
	http://www.fensende.com/Users/mcuddy/
	11/27/1996

Check out my gyruss emulation page:
	http://www.fensende.com/Users/mcuddy/gyruss/
