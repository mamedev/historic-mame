#define VERBOSE 0

/***************************************************************************

TODO:
- It seems shadows can both affect underlying sprites and not. This is currently
  hardcoded in the drivers; there might be a control bit somewhere.
  Games requiring shadows to affect sprites behind them:
  - Surprise Attack (dark glass walls in level 3)
  - 88 Games (angle indicator in the long jump event).
  - Sunset Riders (bull's eye in the saloon cutscene)
  Games requiring shadows to NOT affect sprites behind them:
  - Asterix (Asterix's shadow would be over his feet otherwise)
  - X-Men is dubious, see enemies halfway through level 1 coming from above with
    boulders over their heads.

- scrollcontrol = 30 in Golfing Greats (leader board)

- detatwin: sprites are left on screen during attract mode


                      Emulated
                         |
                  board #|year    CPU      tiles        sprites  priority palette    other
                    -----|---- ------- ------------- ------------- ------ ------ ----------------
Hyper Crash         GX401 1985                   GX400
Twinbee             GX412*1985   68000           GX400
Yie Ar Kung Fu      GX407*1985    6809
Gradius / Nemesis   GX456*1985   68000           GX400
Shao-lins Road      GX477*1985    6809
Jail Break          GX507*1986 KONAMI-1          005849                   PROMs
Finalizer           GX523*1985 KONAMI-1          005885                   PROMs
Konami's Ping Pong  GX555*1985     Z80
Iron Horse          GX560*1986    6809           005885                   PROMs
Konami GT           GX561*1985   68000           GX400
Green Beret         GX577*1985     Z80           005849                   PROMs
Galactic Warriors   GX578*1985   68000           GX400
Salamander          GX587*1986   68000           GX400
WEC Le Mans 24      GX602*1986 2x68000
BAW / Black Panther GX604*1987   68000           GX400                    007593
Combat School /     GX611*1987    6309           007121(x2)               007327
  Boot Camp
Rock 'n Rage /      GX620*1986    6309 007342        007420               007327
  Koi no Hotrock
Mr Kabuki/Mr Goemon GX621*1986     Z80           005849
Jackal              GX631*1986    6809?          005885(x2)
Contra / Gryzor     GX633*1987    6809?          007121(x2)               007593
Flak Attack         GX669*1987    6309           007121                   007327 007452
Devil World / Dark  GX687*1987 2x68000           TWIN16
  Adventure / Majuu no Oukoku
Double Dribble      GX690*1986  3x6809           005885(x2)               007327 007452
Kitten Kaboodle /   GX712*1988                   GX400                    007593 051550
  Nyan Nyan Panic
Chequered Flag      GX717*1988  052001               051960 051937(x2)           051316(x2) (roz) 051733 (protection)
Fast Lane           GX752*1987    6309           007121                          051733 (protection) 007801
Hot Chase           GX763*1988 2x68000                                           051316(x3) (roz) 007634 007635 007558 007557
Rack 'Em Up /       GX765*1987    6309 007342        007420               007327 007324
  The Hustler
Haunted Castle      GX768*1988  052001           007121(x2)               007327
Ajax / Typhoon      GX770*1987   6309+ 052109 051962 051960 051937  PROM  007327 051316 (roz)
                                052001
Labyrinth Runner /  GX771*1987    6309           007121                   007593 051733 (protection) 051550
  Trick Trap
Super Contra        GX775*1988  052001 052109 051962 051960 051937  PROM  007327
Battlantis          GX777*1987    6309 007342        007420               007327 007324
Vulcan Venture /    GX785*1988 2x68000           TWIN16
  Gradius 2
City Bomber         GX787*1987   68000           GX400                    007593 051550
Over Drive          GX789*1990 2x68000               053247 053246 053251 051316(x2) (roz) 053249 053250(x2) (road) 053252(*)
Hyper Crash         GX790 1987
Blades of Steel     GX797*1987    6309 007342        007420               007327 051733 (protection)
The Main Event      GX799*1988    6309 052109 051962 051960 051937  PROM
Missing in Action   GX808*1989   68000 052109 051962 051960 051937  PROM
Missing in Action J GX808*1989 2x68000           TWIN16
Crime Fighters      GX821*1989  052526 052109 051962 051960 051937  PROM
Special Project Y   GX857*1989    6309 052109 051962 051960 051937  PROM         052591 (protection)
'88 Games           GX861*1988  052001 052109 051962 051960 051937  PROM         051316 (roz)
Final Round /       GX870*1988 1x68000           TWIN16?
  Hard Puncher
Thunder Cross       GX873*1988  052001 052109 051962 051960 051937  PROM  007327 052591 (protection)
Aliens              GX875*1990  052526 052109 051962 051960 051937  PROM
Gang Busters        GX878*1988  052526 052109 051962 051960 051937  PROM
Devastators         GX890*1988    6309 052109 051962 051960 051937  PROM         007324 051733 (protection)
Bottom of the Ninth GX891*1989    6809 052109 051962 051960 051937  PROM         051316 (roz)
Cue Brick           GX903*1989 2x68000           TWIN16
Punk Shot           GX907*1990   68000 052109 051962 051960 051937 053251
Ultraman            GX910*1991   68000 ------ ------ 051960 051937  PROM         051316(x3) (roz) 051550
Surprise Attack     GX911*1990  053248 052109 051962 053245 053244 053251
Lightning Fighters /GX939*1990   68000 052109 051962 053245 053244 053251
  Trigon
Gradius 3           GX945*1989 2x68000 052109 051962 051960 051937  PROM
Parodius            GX955*1990  053248 052109 051962 053245 053244 053251
TMNT                GX963*1989   68000 052109 051962 051960 051937  PROM
Block Hole          GX973*1989  052526 052109 051962 051960 051937  PROM
Escape Kids         GX975 1991  053248 052109 051962 053247 053246 053251        053252(*) - same board as Vendetta
Rollergames         GX999*1991  053248 ------ ------ 053245 053244               051316 (roz) 053252(*)
Bells & Whistles /  GX060*1991   68000 052109 051962 053245 053244 053251        054000 (collision)
  Detana!! Twin Bee
Golfing Greats      GX061*1991   68000 052109 051962 053245 053244 053251        053936 (roz+)
TMNT 2              GX063*1991   68000 052109 051962 053245 053244 053251        053990 (protection) 051550
Sunset Riders       GX064*1991   68000 052109 051962 053245 053244 053251        054358
X-Men               GX065*1992   68000 052109 051962 053247 053246 053251        054539 (sound)
XEXEX               GX067*1991   68000 054157 054156 053247 053246 053251        053250?("road") 054338 054539 (sound)
Asterix             GX068*1992   68000 054157 054156 053245 053244 053251        054358
G.I. Joe            GX069*1992   68000 054157 054156 053247 053246 053251        054539 (sound)
The Simpsons        GX072*1991  053248 052109 051962 053247 053246 053251
Thunder Cross 2     GX073*1991   68000 052109 051962 051960 051937 053251        054000 (collision)
Vendetta /          GX081*1991  053248 052109 051962 053247 053246 053251        054000 (collision)
  Crime Fighters 2
Premier Soccer      GX101*1993   68000 052109 051962 053245 053244 053251        053936 (roz+) 054986
Hexion              GX122*1992     Z80                                           052591 (protection) 053252(*)
Entapous /          GX123+1993   68000 054157 054156 055673 053246 055555        053252(*) 054000 053936 (roz+)
  Gaiapolis
Mystic Warrior      GX128+1993   68000 054157 054156 055673 053246 055555        054338 (alpha) 053252(*) 054539(x2) (sound)
Cowboys of Moo Mesa GX151*1992   68000 054157 054156 053247 053246 053251        053252(*) 054338 053990 (protection)
Violent Storm       GX168+1993   68000 054157 054156 055673 053246 055555        054338 (alpha) 055550 054539(x2) (sound)
Monster Maulers /   GX170+1993   68000 054157 054156 055673 053246 055555        053252(*) 055550 054338 (alpha) 054539 (sound) 053936 (roz+)
  Ultimate Battler Dadandarn
Bucky 'O Hare       GX173*1992   68000 054157 054156 053247 053246 053251        054338 (alpha) 054539 (sound)
Potrio              GX174 1992
Lethal Enforcers    GX191+1992    6309 054157(x2) 054156 053245 053244(x2)       054000 054906 054539 (sound)
Metamorphic Force   GX224+1993   68000 054157 054157 055673 053246 055555
Martial Champion    GX234+1993   68000 054157 054156 055673 053246 055555        053252(*) 054338 (alpha) 053990 054539 (sound)
Run and Gun         GX247*1993   68000 (TTL tilemap) 055673 053246               053253(x2) 053252(*) 053936 (roz+) 054539(x2) (sound)
Quiz Gakumon no     GX248*1993   68000 052109 051962 053245 053244 053251        053990 (protection) 051550 - same board as TMNT2
  Susume
Polygonet Commander GX305+1993   68020 (TTL tilemap)                             XC56156-40(3D DSP) 054009(x2) 054010(x2) 054539 (sound)
System GX           GX300*1993   68020 056832 054156 055673 053246 055555        054338 (alpha) 054539(x2) (sound) 053252(*) 053936 (optional on ROM board, roz+)
Tail to Nose             *1989   68000          V-System                         051316 (roz)
F-1 Grand Prix           *1991 2x68000          V-System                         053936 (roz+)
Lethal Crash Race        *1993   68000          V-System                         053936 (roz+)
Super Slams              *1995   68000          V-System                         053936 (roz+)
Blazing Tornado          *1991   68000            Metro                          053936 (roz+)
Dragonball Z 2           *1994   68000 054157 054156 053247 053246 053251(x2)    053936(x2) (roz+) 053252(*)


Notes:
* 053252 seems to be just a timing/interrupt controller (see Vendetta schematics).

- Old games use 051961 instead of 052109, it is an earlier version functionally
  equivalent (maybe 052109 had bugs fixed). The list always shows 052109 because
  the two are exchangeable and 052109's are found also on original boards whose
  schematics show a 051961.



Status of the ROM tests in the emulated games:

Chequered Flag      pass
Ajax / Typhoon      pass
Super Contra        pass
Over Drive          fails 16..20 (053250)
The Main Event      pass
Missing in Action   pass
Crime Fighters      pass
Special Project Y   pass
Konami 88           pass
Thunder Cross       pass
Aliens              pass
Gang Busters        pass
Devastators         pass
Bottom of the Ninth pass
Punk Shot           pass
Surprise Attack     fails D05-6 (052109) because it uses mirror addresses to
                    select banks, and supporting those addresses breaks the
                    normal game ;-(
Lightning Fighters  pass
Gradius 3           pass
Parodius            pass
TMNT                pass
Block Hole          pass
Rollergames         pass
Bells & Whistles    pass
Golfing Greats      pass
TMNT 2              pass
Sunset Riders       pass
X-Men               pass
The Simpsons        pass
Thunder Cross 2     pass
Xexex               pass
Asterix             pass
GiJoe				pass
Vendetta            pass
Premier Soccer		fails 16D 18D 18F (053936)
Hexion              pass
Dragonball Z 2      fails


THE FOLLOWING INFORMATION IS PRELIMINARY AND INACCURATE. DON'T RELY ON IT.


005885
------
Some games use two of these in pair. Jackal even puts together the two 4bpp
tilemaps to form a single 8bpp one.
It manages sprites and 32x32 or 64x32 tilemap (only Double Dribble uses the
64x32 one).
The chip also generates clock and interrupt signals suitable for a 6809.
It uses 0x2000 bytes of RAM for the tilemaps and sprites, and an additional
0x100 bytes, maybe for scroll RAM and line buffers. The maximum addressable
ROM is 0x20000 bytes (addressed 16 bits at a time). Tile and sprite data both
come from the same ROM space. Double Dribble and Jackal have external circuitry
to extend the limits and use separated addressing spaces for sprites and tiles.
All games use external circuitry to reuse one or both the tile flip attributes
as an additional address bit.
Two 256x4 lookup PROMs are also used to increase the color combinations.
All tilemap / sprite priority handling is done internally and the chip exports
5 bits of color code, composed of 1 bit indicating tile or sprite, and 4 bits
of ROM data remapped through the PROM.

inputs:
- address lines (A0-A13)
- data lines (DB0-DB7)
- misc interface stuff
- data from the gfx ROMs (RDL0-RDL7, RDU0-RDU7)
- data from the tile lookup PROMs (VCD0-VCD3)
- data from the sprite lookup PROMs (OCD0-OCD3)

outputs:
- address lines for tilemap RAM (AX0-AX12)
- data lines for tilemap RAM (VO0-VO7)
- address lines for the small RAM (FA0-FA7)
- data lines for the small RAM (FD0-FD7)
- address lines for the gfx ROMs (R0-R15)
- address lines for the tile lookup PROMs (VCF0-VCF3, VCB0-VCB3)
- address lines for the sprite lookup PROMs (OCB0-OCB3, OCF0-OCF3)
- NNMI, NIRQ, NFIR, NCPE, NCPQ, NEQ for the main CPU
- misc interface stuff
- color code to be output on screen (COL0-COL4)


control registers
000:          scroll y
001:          scroll x (low 8 bits)
002: -------x scroll x (high bit)
     ----xxx- row/colscroll control
              000 = solid scroll (finalizr, ddribble bg)
              100 = solid scroll (jackal)
              001 = ? (ddribble fg)
              011 = colscroll (jackal high scores)
              101 = rowscroll (ironhors, jackal map)
003: ------xx high bits of the tile code
     -----x-- unknown (finalizr)
     ----x--- selects sprite buffer (and makes a copy to a private buffer?)
     --x----- unknown (ironhors)
     -x------ unknown (ironhors)
     x------- unknown (ironhors, jackal)
004: -------x nmi enable
     ------x- irq enable
     -----x-- firq enable
     ----x--- flip screen



007121
------
This is an interesting beast. It is an evolution of the 005885, with more
features. Many games use two of these in pair.
It manages sprites and two 32x32 tilemaps. The tilemaps can be joined to form
a single 64x32 one, or one of them can be moved to the side of screen, giving
a high score display suitable for vertical games.
The chip also generates clock and interrupt signals suitable for a 6809.
It uses 0x2000 bytes of RAM for the tilemaps and sprites, and an additional
0x100 bytes, maybe for scroll RAM and line buffers. The maximum addressable
ROM is 0x80000 bytes (addressed 16 bits at a time). Tile and sprite data both
come from the same ROM space.
Two 256x4 lookup PROMs are also used to increase the color combinations.
All tilemap / sprite priority handling is done internally and the chip exports
7 bits of color code, composed of 2 bits of palette bank, 1 bit indicating tile
or sprite, and 4 bits of ROM data remapped through the PROM.

inputs:
- address lines (A0-A13)
- data lines (DB0-DB7)
- misc interface stuff
- data from the gfx ROMs (RDL0-RDL7, RDU0-RDU7)
- data from the tile lookup PROMs (VCD0-VCD3)
- data from the sprite lookup PROMs (OCD0-OCD3)

outputs:
- address lines for tilemap RAM (AX0-AX12)
- data lines for tilemap RAM (VO0-VO7)
- address lines for the small RAM (FA0-FA7)
- data lines for the small RAM (FD0-FD7)
- address lines for the gfx ROMs (R0-R17)
- address lines for the tile lookup PROMs (VCF0-VCF3, VCB0-VCB3)
- address lines for the sprite lookup PROMs (OCB0-OCB3, OCF0-OCF3)
- NNMI, NIRQ, NFIR, NE, NQ for the main CPU
- misc interface stuff
- color code to be output on screen (COA0-COA6)


control registers
000:          scroll x (low 8 bits)
001: -------x scroll x (high bit)
     ------x- enable rowscroll? (combasc)
     ----x--- this probably selects an alternate screen layout used in combat
              school where tilemap #2 is overlayed on front and doesn't scroll.
              The 32 lines of the front layer can be individually turned on or
              off using the second 32 bytes of scroll RAM.
002:          scroll y
003: -------x bit 13 of the tile code
     ------x- unknown (contra)
     -----x-- might be sprite / tilemap priority (0 = sprites have priority)
              (combat school, contra, haunted castle(0/1), labyrunr)
     ----x--- selects sprite buffer (and makes a copy to a private buffer?)
     ---x---- screen layout selector:
              when this is set, 5 columns are added on the left of the screen
              (that means 5 rows at the top for vertical games), and the
              rightmost 2 columns are chopped away.
              Tilemap #2 is used to display the 5 additional columns on the
              left. The rest of tilemap #2 is not used and can be used as work
              RAM by the program.
              The visible area becomes 280x224.
              Note that labyrunr changes this at runtime, setting it during
              gameplay and resetting it on the title screen and crosshatch.
     --x----- might be sprite / tilemap priority (0 = sprites have priority)
              (combat school, contra, haunted castle(0/1), labyrunr)
     -x------ Chops away the leftmost and rightmost columns, switching the
              visible area from 256 to 240 pixels. This is used by combasc on
              the scrolling stages, and by labyrunr on the title screen.
              At first I thought that this enabled an extra bank of 0x40
              sprites, needed by combasc, but labyrunr proves that this is not
              the case
     x------- unknown (contra)
004: ----xxxx bits 9-12 of the tile code. Only the bits enabled by the following
              mask are actually used, and replace the ones selected by register
			  005.
     xxxx---- mask enabling the above bits
005: selects where in the attribute byte to pick bits 9-12 of the tile code,
     output to pins R12-R15. The bit of the attribute byte to use is the
     specified bit (0-3) + 3, that is one of bits 3-6. Bit 7 is hardcoded as
     bit 8 of the code. Bits 0-2 are used for the color, however note that
     some games use bit 3 as well (see below).
     ------xx attribute bit to use for tile code bit  9
     ----xx-- attribute bit to use for tile code bit 10
     --xx---- attribute bit to use for tile code bit 11
     xx------ attribute bit to use for tile code bit 12
006: ----xxxx select additional effect for bits 3-6 of the tile attribute (the
              same ones indexed by register 005). Note that an attribute bit
              can therefore be used at the same time to be BOTH a tile code bit
              and an additional effect.
     -------x bit 3 of attribute is bit 3 of color (combasc, fastlane, flkatck)
	 ------x- bit 4 of attribute is tile flip X (assumption - no game uses this)
     -----x-- bit 5 of attribute is tile flip Y (flkatck)
     ----x--- bit 6 of attribute is tile priority over sprites (combasc, hcastle,
              labyrunr)
              Note that hcastle sets this bit for layer 0, and bit 6 of the
              attribute is also used as bit 12 of the tile code, however that
              bit is ALWAYS set throughout the game.
              combasc uses the bit in the "graduation" scene during attract mode,
              to place soldiers behind the stand.
              Use in labyrunr has not been investigated yet.
     --xx---- palette bank (both tiles and sprites, see contra)
007: -------x nmi enable
     ------x- irq enable
     -----x-- firq enable
     ----x--- flip screen
     ---x---- unknown (contra, labyrunr)



007342
------
The 007342 manages 2 64x32 scrolling tilemaps with 8x8 characters, and
optionally generates timing clocks and interrupt signals. It uses 0x2000
bytes of RAM, plus 0x0200 bytes for scrolling, and a variable amount of ROM.
It cannot read the ROMs.

control registers
000: ------x- INT control
     ---x---- flip screen (TODO: doesn't work with thehustl)
001: Used for banking in Rock'n'Rage
002: -------x MSB of x scroll 1
     ------x- MSB of x scroll 2
     ---xxx-- layer 1 row/column scroll control
              000 = disabled
			  010 = unknown (bladestl shootout between periods)
              011 = 32 columns (Blades of Steel)
              101 = 256 rows (Battlantis, Rock 'n Rage)
     x------- enable sprite wraparound from bottom to top (see Blades of Steel
              high score table)
003: x scroll 1
004: y scroll 1
005: x scroll 2
006: y scroll 2
007: not used


007420
------
Sprite generator. 8 bytes per sprite with zoom. It uses 0x200 bytes of RAM,
and a variable amount of ROM. Nothing is known about its external interface.



052109/051962
-------------
These work in pair.
The 052109 manages 3 64x32 scrolling tilemaps with 8x8 characters, and
optionally generates timing clocks and interrupt signals. It uses 0x4000
bytes of RAM, and a variable amount of ROM. It cannot read the ROMs:
instead, it exports 21 bits (16 from the tilemap RAM + 3 for the character
raster line + 2 additional ones for ROM banking) and these are externally
used to generate the address of the required data on the ROM; the output of
the ROMs is sent to the 051962, along with a color code. In theory you could
have any combination of bits in the tilemap RAM, as long as they add to 16.
In practice, all the games supported so far standardize on the same format
which uses 3 bits for the color code and 13 bits for the character code.
The 051962 multiplexes the data of the three layers and converts it into
palette indexes and transparency bits which will be mixed later in the video
chain.
Priority is handled externally: these chips only generate the tilemaps, they
don't mix them.
Both chips are interfaced with the main CPU. When the RMRD pin is asserted,
the CPU can read the gfx ROM data. This is done by telling the 052109 which
dword to read (this is a combination of some banking registers, and the CPU
address lines), and then reading it from the 051962.

052109 inputs:
- address lines (AB0-AB15, AB13-AB15 seem to have a different function)
- data lines (DB0-DB7)
- misc interface stuff

052109 outputs:
- address lines for the private RAM (RA0-RA12)
- data lines for the private RAM (VD0-VD15)
- NMI, IRQ, FIRQ for the main CPU
- misc interface stuff
- ROM bank selector (CAB1-CAB2)
- character "code" (VC0-VC10)
- character "color" (COL0-COL7); used foc color but also bank switching and tile
  flipping. Exact meaning depends on externl connections. All evidence indicates
  that COL2 and COL3 select the tile bank, and are replaced with the low 2 bits
  from the bank register. The top 2 bits of the register go to CAB1-CAB2.
  However, this DOES NOT WORK with Gradius III. "color" seems to pass through
  unaltered.
- layer A horizontal scroll (ZA1H-ZA4H)
- layer B horizontal scroll (ZB1H-ZB4H)
- ????? (BEN)

051962 inputs:
- gfx data from the ROMs (VC0-VC31)
- color code (COL0-COL7); only COL4-COL7 seem to really be used for color; COL0
  is tile flip X.
- layer A horizontal scroll (ZA1H-ZA4H)
- layer B horizontal scroll (ZB1H-ZB4H)
- let main CPU read the gfx ROMs (RMRD)
- address lines to be used with RMRD (AB0-AB1)
- data lines to be used with RMRD (DB0-DB7)
- ????? (BEN)
- misc interface stuff

051962 outputs:
- FIX layer palette index (DFI0-DFI7)
- FIX layer transparency (NFIC)
- A layer palette index (DSA0-DSAD); DSAA-DSAD seem to be unused
- A layer transparency (NSAC)
- B layer palette index (DSB0-DSBD); DSBA-DSBD seem to be unused
- B layer transparency (NSBC)
- misc interface stuff


052109 memory layout:
0000-07ff: layer FIX tilemap (attributes)
0800-0fff: layer A tilemap (attributes)
1000-1fff: layer B tilemap (attributes)
180c-1833: A y scroll
1a00-1bff: A x scroll
1c00     : ?
1c80     : row/column scroll control
           ------xx layer A row scroll
                    00 = disabled
                    01 = disabled? (gradius3, vendetta)
                    10 = 32 lines
                    11 = 256 lines
           -----x-- layer A column scroll
                    0 = disabled
                    1 = 64 (actually 40) columns
           ---xx--- layer B row scroll
           --x----- layer B column scroll
           surpratk sets this register to 70 during the second boss. There is
           nothing obviously wrong so it's not clear what should happen.
		   glfgreat sets it to 30 when showing the leader board
1d00     : bits 0 & 1 might enable NMI and FIRQ, not sure
         : bit 2 = IRQ enable
1d80     : ROM bank selector bits 0-3 = bank 0 bits 4-7 = bank 1
1e00     : ROM subbank selector for ROM testing
1e80     : bit 0 = flip screen (applies to tilemaps only, not sprites)
         : bit 1 = set by crimfght, mainevt, surpratk, xmen, mia, punkshot, thndrx2, spy
         :         it seems to enable tile flip X, however flip X is handled by the
         :         051962 and it is not hardwired to a specific tile attribute.
         :         Note that xmen, punkshot and thndrx2 set the bit but the current
         :         drivers don't use flip X and seem to work fine.
         : bit 2 = enables tile flip Y when bit 1 of the tile attribute is set
1f00     : ROM bank selector bits 0-3 = bank 2 bits 4-7 = bank 3
2000-27ff: layer FIX tilemap (code)
2800-2fff: layer A tilemap (code)
3000-37ff: layer B tilemap (code)
3800-3807: nothing here, so the chip can share address space with a 051937
380c-3833: B y scroll
3a00-3bff: B x scroll
3c00-3fff: nothing here, so the chip can share address space with a 051960
3d80     : mirror of 1d80, but ONLY during ROM test (surpratk)
3e00     : mirror of 1e00, but ONLY during ROM test (surpratk)
3f00     : mirror of 1f00, but ONLY during ROM test (surpratk)
EXTRA ADDRESSING SPACE USED BY X-MEN:
4000-47ff: layer FIX tilemap (code high bits)
4800-4fff: layer A tilemap (code high bits)
5000-57ff: layer B tilemap (code high bits)

The main CPU doesn't have direct acces to the RAM used by the 052109, it has
to through the chip.



054156/054157
-------------

[Except for tilemap sizes, all numbers are in hex]

These work in pair.  Similar in principle to the 052109/051962, they
manage 4 64x32 or 64x64 tilemaps.  They also handle linescroll on each
layer, and optional tile banking.  They use 4000 to 10000 bytes of
RAM, organized in 1000 or 2000 bytes banks.

Known configurations:
4 1000 bytes banks, tile banking, 64x32 tilemaps:
  - asterix

4 2000 bytes banks, no tile banking, 64x32 tilemaps:
  - gijoe
  - bucky 'o hare

8 2000 bytes banks, no tile banking, 64x32 or 64x64 tilemaps:
  - xexex

16 2000 byte banks, external tile banking, tilemaps up to 256x128:
  - System GX, some "PreGX" games
In 4 banks configurations, the banks are numbered 0, 1, 8 and 9 and
are associated to tilemaps 0..3 in that order.

In 8 banks configurations, the banks are numbered 0, 1, 8, 9, 10, 11,
18, and 19, and are associated in pairs to the tilemaps as (11, 19)
for layer 0, (0, 8) for 1, (10, 11) for 2 and (1, 9) for 3.

Tile encoding 2 bytes/tile (banks of 1000 bytes):
        pppx bbcc cccc cccc
  p = color palette
  x = flip x
  b = tile bank (0..3)
  c = tile code (0..3ff)


Tile encoding 4 bytes/tile (banks of 2000 bytes):
        ---- ---- pppp --yx  cccc cccc cccc cccc
  p = color palette
  x = flip x
  y = flip y
  b = tile bank (0..3)
  c = tile code (0..3ff)


Communication with these ics go through 4 memory zones:
  1000/2000 bytes: access to the currently selected ram bank
       2000 bytes: readonly access the the currently select tile
                   rom bank for rom checksumming
         40 bytes: writeonly access to the first register bank
          8 bytes: writeonly access to the second register bank

One of the register banks is probably on the 054156, and the other on
the 054157.

First register bank map (offsets in bytes, '-' means unused):
00    ---- ---- ??yx ????
  flip control

02    ---- ---- ???? ????
  unknown

04    ---- ---- ???? ????
  unknown (bit 1 may be bank count selection, 0 in xexex, 1 everywhere
  else)

06    ---- ---- ???? ???e
  enable irq

08    ---- ---- ???? ????
  unknown

0a    ---- ---- 3322 1100
  linescroll control, each pair of bits indicates the mode for the
  corresponding layer:
    0: per-line linescroll
    1: unused/unknown
    2: per-8 lines linescroll
    3: no linescroll

0c    ---- ---- ???? ????
  unknown (bit 1 may be bank size selection, 1 in asterix, 0 everywhere
  else)

0e    ---- ---- ---- ----

10-17 ---- ---- ???? ???s
  layer type control
    s: size (0=64x32, 1=64x64)

18-1f ---- ---- ???? ????

20-27 yyyy yyyy yyyy yyyy
  scroll y position for each layer

28-2f xxxx xxxx xxxx xxxx
  scroll x position for each layer

30    ---- ---- ---b b--b
  linescroll ram bank selection

32    ---- ---- ---b b--b
  cpu-accessible ram bank selection

34    bbbb bbbb bbbb bbbb
  rom bank selection for checksumming (each bank is 0x2000 bytes)

36    ---- ---- ---- bbbb
  secondary rom bank selection for checksumming when tile banking is
  used

38    3333 2222 1111 0000
  tile banking look up table.  4 bits are looked up here for the two
  bits in the tile data.

3a    ???? ???? ???? ????
  unknown

3c    ???? ???? ???? ????
  unknown

3e    ---- ---- ---- ----


Second register bank map:
00    ---- ---- ???? ????
  unknown

02-07 are copies of the 02-07 registers from the first bank.


  Linescroll:

The linescroll is controlled by the register 0b, and uses the data in
the ram bank pointed by register 31.  The data for tilemap <n> starts
at offset 400*n in the bank for 1000 bytes ram banks, and 800*n+2 for
2000 bytes ram banks.  The scrolling information is a vector of half
words separated by 1 word padding for 2000 bytes banks.

This is a source-oriented linescroll, i.e. the first word is
associated to the first one of the tilemap, not matter what the
current scrolly position is.

In per-line mode, each word indicates the horizontal scroll of the
associated line.  Global scrollx is ignored.

In per-8 lines mode, each word associated to a line multiple of 8
indicates the horizontal scroll for that line and the 7 following
ones.  The other 7 words are ignored.  Global scrollx is ignored.



051960/051937
-------------
Sprite generators. Designed to work in pair. The 051960 manages the sprite
list and produces and address that is fed to the gfx ROMs. The data from the
ROMs is sent to the 051937, along with color code and other stuff from the
051960. The 051937 outputs up to 12 bits of palette index, plus "shadow" and
transparency information.
Both chips are interfaced to the main CPU, through 8-bit data buses and 11
bits of address space. The 051937 sits in the range 000-007, while the 051960
in the range 400-7ff (all RAM). The main CPU can read the gfx ROM data though
the 051937 data bus, while the 051960 provides the address lines.
The 051960 is designed to directly address 1MB of ROM space, since it produces
18 address lines that go to two 16-bit wide ROMs (the 051937 has a 32-bit data
bus to the ROMs). However, the addressing space can be increased by using one
or more of the "color attribute" bits of the sprites as bank selectors.
Moreover a few games store the gfx data in the ROMs in a format different from
the one expected by the 051960, and use external logic to reorder the address
lines.
The 051960 can also genenrate IRQ, FIRQ and NMI signals.

memory map:
000-007 is for the 051937, but also seen by the 051960
400-7ff is 051960 only
000     R  bit 0 = unknown, looks like a status flag or something
                   aliens waits for it to be 0 before starting to copy sprite data
				   thndrx2 needs it to pulse for the startup checks to succeed
000     W  bit 0 = irq enable/acknowledge?
           bit 2 = nmi enable?
           bit 3 = flip screen (applies to sprites only, not tilemaps)
           bit 4 = unknown, used by Devastators, TMNT, Aliens, Chequered Flag, maybe others
                   aliens sets it just after checking bit 0, and before copying
                   the sprite data
           bit 5 = enable gfx ROM reading
001     W  Devastators sets bit 1, function unknown.
           Ultraman sets the register to 0x0f.
           None of the other games I tested seem to set this register to other than 0.
002-003 W  selects the portion of the gfx ROMs to be read.
004     W  Aliens uses this to select the ROM bank to be read, but Punk Shot
           and TMNT don't, they use another bit of the registers above. Many
           other games write to this register before testing.
           It is possible that bits 2-7 of 003 go to OC0-OC5, and bits 0-1 of
           004 go to OC6-OC7.
004-007 R  reads data from the gfx ROMs (32 bits in total). The address of the
           data is determined by the register above and by the last address
           accessed on the 051960; plus bank switch bits for larger ROMs.
           It seems that the data can also be read directly from the 051960
           address space: 88 Games does this. First it reads 004 and discards
           the result, then it reads from the 051960 the data at the address
           it wants. The normal order is the opposite, read from the 051960 at
           the address you want, discard the result, and fetch the data from
           004-007.
400-7ff RW sprite RAM, 8 bytes per sprite



053245/053244
-------------
Sprite generators. The 053245 has a 16-bit data bus to the main CPU.
The sprites are buffered, a write to 006 activates to copy between the
main ram and the buffer.

053244 memory map (but the 053245 sees and processes them too):
000-001  W global X offset
002-003  W global Y offset
004      W unknown
005      W bit 0 = flip screen X
           bit 1 = flip screen Y
           bit 2 = unknown, used by Parodius
           bit 4 = enable gfx ROM reading
           bit 5 = unknown, used by Rollergames
006     RW accessing this register copies the sprite ram to the internal buffer
007      W unknown
008-009  W low 16 bits of the ROM address to read
00a-00b  W high bits of the ROM address to read.  3 bits for most games, 1 for asterix
00c-00f R  reads data from the gfx ROMs (32 bits in total). The address of the
           data is determined by the registers above; plus bank switch bits for
           larger ROMs.



053247/053246
-------------
Sprite generators. Nothing is known about their external interface.
The sprite RAM format is very similar to the 053245.

053246 memory map (but the 053247 sees and processes them too):
000-001 W  global X offset
002-003 W  global Y offset
004     W  low 8 bits of the ROM address to read
005     W  bit 0 = flip screen X
           bit 1 = flip screen Y
           bit 2 = unknown
           bit 4 = interrupt enable
           bit 5 = unknown
006-007 W  high 16 bits of the ROM address to read

???-??? R  reads data from the gfx ROMs (16 bits in total). The address of the
           data is determined by the registers above



051316
------
Manages a 32x32 tilemap (16x16 tiles, 512x512 pixels) which can be zoomed,
distorted and rotated.
It uses two internal 24 bit counters which are incremented while scanning the
picture. The coordinates of the pixel in the tilemap that has to be drawn to
the current beam position are the counters / (2^11).
The chip doesn't directly generate the color information for the pixel, it
just generates a 24 bit address (whose top 16 bits are the contents of the
tilemap RAM), and a "visible" signal. It's up to external circuitry to convert
the address into a pixel color. Most games seem to use 4bpp graphics, but Ajax
uses 7bpp.
If the value in the internal counters is out of the visible range (0..511), it
is truncated and the corresponding address is still generated, but the "visible"
signal is not asserted. The external circuitry might ignore that signal and
still generate the pixel, therefore making the tilemap a continuous playfield
that wraps around instead of a large sprite.

control registers
000-001 X counter starting value / 256
002-003 amount to add to the X counter after each horizontal pixel
004-005 amount to add to the X counter after each line (0 = no rotation)
006-007 Y counter starting value / 256
008-009 amount to add to the Y counter after each horizontal pixel (0 = no rotation)
00a-00b amount to add to the Y counter after each line
00c-00d ROM bank to read, used during ROM testing
00e     bit 0 = enable ROM reading (active low). This only makes the chip output the
                requested address: the ROM is actually read externally, not through
                the chip's data bus.
        bit 1 = unknown
        bit 2 = unknown
00f     unused



053936
------
Evolution of the 051316. The data bus is 16-bit instead of 8-bit.
When used in "simple" mode it can generate the same effects of the 051316, but it
doesn't have internal tilemap RAM, so it just generates a couple of X/Y coordinates
indicating the pixel to display at each moment. Therefore, the tilemap (and tile)
size are not fixed.
The important addition over the 051316 is 512x4 words of internal RAM used to control
rotation and zoom scanline by scanline instead that on the whole screen, allowing for
effects like linescroll (Super Slams) or 3D rotation of the tilemap (Golfing Greats,
Premier Soccer).

control registers
000 X counter starting value / 256
001 Y counter starting value / 256
002 ["simple" mode only] amount to add to the X counter after each line (0 = no rotation)
003 ["simple" mode only] amount to add to the X counter after each horizontal pixel
004 ["simple" mode only] amount to add to the Y counter after each line
005 ["simple" mode only] amount to add to the Y counter after each horizontal pixel (0 = no rotation)
006 x------- -------- when set, register (line*4)+2 must be multiplied by 256
    -x------ -------- when set, registers 002 and 003 must be multiplied by 256
    --xxxxxx -------- clipping for the generated address? usually 3F, Premier Soccer
                      sets it to 07 before penalty kicks
    -------- x------- when set, register (line*4)+3 must be multiplied by 256
    -------- -x------ when set, registers 004 and 005 must be multiplied by 256
    -------- --xxxxxx clipping for the generated address? usually 3F, Premier Soccer
                      sets it to 0F before penalty kicks
007 -------- -x------ enable "super" mode
    -------- --x----- unknown (enable address clipping from register 006?)
    -------- ---x---- unknown
    -------- ------x- (not sure) enable clipping with registers 008-00b
008 min x screen coordinate to draw to (only when enabled by register 7)
009 max x screen coordinate to draw to (only when enabled by register 7)
00a min y screen coordinate to draw to (only when enabled by register 7)
00b max y screen coordinate to draw to (only when enabled by register 7)
00c unknown
00d unknown
00e unknown
00f unknown

additional control from extra RAM:
(line*4)+0 X counter starting value / 256 (add to register 000)
(line*4)+1 Y counter starting value / 256 (add to register 001)
(line*4)+2 amount to add to the X counter after each horizontal pixel
(line*4)+3 amount to add to the Y counter after each horizontal pixel



053251
------
Priority encoder.

The chip has inputs for 5 layers (CI0-CI4); only 4 are used (CI1-CI4)
CI0-CI2 are 9(=5+4) bits inputs, CI3-CI4 8(=4+4) bits

The input connctions change from game to game. E.g. in Simpsons,
CI0 = grounded (background color)
CI1 = sprites
CI2 = FIX
CI3 = A
CI4 = B

in lgtnfght:
CI0 = grounded
CI1 = sprites
CI2 = FIX
CI3 = B
CI4 = A

there are three 6 bit priority inputs, PR0-PR2

simpsons:
PR0 = 111111
PR1 = xxxxx0 x bits coming from the sprite attributes
PR2 = 111111

lgtnfght:
PR0 = 111111
PR1 = 1xx000 x bits coming from the sprite attributes
PR2 = 111111

also two shadow inputs, SDI0 and SDI1 (from the sprite attributes)

the chip outputs the 11 bit palette index, CO0-CO10, and two shadow bits.

16 internal registers; registers are 6 bits wide (input is D0-D5)
For the most part, their meaning is unknown
All registers are write only.
There must be a way to enable/disable the three external PR inputs.
Some games initialize the priorities of the sprite & background layers,
others don't. It isn't clear whether the data written to those registers is
actually used, since the priority is taken from the external ports.

 0  priority of CI0 (higher = lower priority)
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 3f = 111111
    xmen:     05 = 000101  default value
    xmen:     09 = 001001  used to swap CI0 and CI2
 1  priority of CI1 (higher = lower priority)
    punkshot: 28 = 101000
    lgtnfght: unused?
    simpsons: unused?
    xmen:     02 = 000010
 2  priority of CI2 (higher = lower priority)
    punkshot: 24 = 100100
    lgtnfght: 24 = 100100
    simpsons: 04 = 000100
    xmen:     09 = 001001  default value
    xmen:     05 = 000101  used to swap CI0 and CI2
 3  priority of CI3 (higher = lower priority)
    punkshot: 34 = 110100
    lgtnfght: 34 = 110100
    simpsons: 28 = 101000
    xmen:     00 = 000000
 4  priority of CI4 (higher = lower priority)
    punkshot: 2c = 101100  default value
    punkshot: 3c = 111100  used to swap CI3 and CI4
    punkshot: 26 = 100110  used to swap CI1 and CI4
    lgtnfght: 2c = 101100
    simpsons: 18 = 011000
    xmen:     fe = 111110
 5  unknown
    punkshot: unused?
    lgtnfght: 2a = 101010
    simpsons: unused?
    xmen: unused?
 6  unknown
    punkshot: 26 = 100110
    lgtnfght: 30 = 110000
    simpsons: 17 = 010111
    xmen:     03 = 000011 (written after initial tests)
 7  unknown
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 27 = 100111
    xmen:     07 = 000111 (written after initial tests)
 8  unknown
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 37 = 110111
    xmen:     ff = 111111 (written after initial tests)
 9  ----xx CI0 palette index base (CO9-CO10)
    --xx-- CI1 palette index base (CO9-CO10)
    xx---- CI2 palette index base (CO9-CO10)
10  ---xxx CI3 palette index base (CO8-CO10)
    xxx--- CI4 palette index base (CO8-CO10)
11  unknown
    punkshot: 00 = 000000
    lgtnfght: 00 = 000000
    simpsons: 00 = 000000
    xmen:     00 = 000000 (written after initial tests)
12  unknown
    punkshot: 04 = 000100
    lgtnfght: 04 = 000100
    simpsons: 05 = 000101
    xmen:     05 = 000101
13  unused
14  unused
15  unused


054000
------
Sort of a protection device, used for collision detection.
It is passed a few parameters, and returns a boolean telling if collision
happened. It has no access to gfx data, it only does arithmetical operations
on the parameters.

Memory map:
00      unused
01-03 W A center X
04    W unknown, needed by thndrx2 to pass the startup check, we use a hack
05      unused
06    W A semiaxis X
07    W A semiaxis Y
08      unused
09-0b W A center Y
0c    W unknown, needed by thndrx2 to pass the startup check, we use a hack
0d      unused
0e    W B semiaxis X
0f    W B semiaxis Y
10      unused
11-13 W B center Y
14      unused
15-17 W B center X
18    R 0 = collision, 1 = no collision


051733
------
Sort of a protection device, used for collision detection, and for
arithmetical operations.
It is passed a few parameters, and returns the result.

Memory map(preliminary):
------------------------
00-01 W operand 1
02-03 W operand 2
04-05 W operand 3

00-01 R operand 1 / operand 2
02-03 R operand 1 % operand 2?
04-05 R sqrt(operand 3<<16)

06-07 W distance for collision check
08-09 W Y pos of obj1
0a-0b W X pos of obj1
0c-0d W Y pos of obj2
0e-0f W X pos of obj2
13	  W unknown

07	  R collision (0x80 = no, 0x00 = yes)
0a-0b R unknown (chequered flag), might just read back X pos
0e-0f R unknown (chequered flag), might just read back X pos

Other addresses are unknown or unused.

Fast Lane:
----------
$9def:
This routine is called only after a collision.
(R) 0x0006:	unknown. Only bits 0-3 are used.

Blades of Steel:
----------------
$ac2f:
(R) 0x2f86: unknown. Only uses bit 0.

$a5de:
writes to 0x2f84-0x2f85, waits a little, and then reads from 0x2f84.

$7af3:
(R) 0x2f86: unknown. Only uses bit 0.


Devastators:
------------
$6ce8:
reads from 0x0006, and only uses bit 1.

K055555
-------

Priority encoder.  Always found in conjunction with K054338, but the reverse 
isn't true.

Lots of byte-wise registers.  Few are yet known.  Those are:

1: related to tilemap brightness control
7-14: priority levels
23-27: palette bases
38: ???
40-41: related to shadow/highlight operation.  0 in both disables s/h.
42-43: related to tilemap brightness control.  0 in both disables tilemap brightness.
45: input enables.  bits 0-3 = tilemaps, bit 4 = sprites for GX (as per gokuparo level 1)

Additionally the 55555 can provide a gradient background with one palette entry
per scanline.  This is fairly rarely used, but does turn up in Gokujou Parodius as
well as the Sexy Parodius title screen.


K054338
-------
Color combiner engine.  Designed for use with the 55555, but also found in games 
without one.

Registers (word-wise):

0: first 8 bits unknown, second 8 bits are the R component of the background color
1: G and B components (8 bits each) of the background color
2+3: both shadow/highlight related
4+5: both shadow/highlight related 
6+7: both shadow/highlight related 
8+9: both shadow/highlight related 
A: shadow/highlight related
B: second 8 bits R component of tilemap brightness
C: G and B components of tilemap brightness
D: second 8 bits = blend level for tilemap planes (0-1f, 1f = totally transparent, 0 = totally opaque)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "state.h"


/*
	This recursive function doesn't use additional memory
	(it could be easily converted into an iterative one).
	It's called shuffle because it mimics the shuffling of a deck of cards.
*/
static void shuffle(UINT16 *buf,int len)
{
	int i;
	UINT16 t;

	if (len == 2) return;

	if (len % 4) exit(1);   /* must not happen */

	len /= 2;

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	shuffle(buf,len);
	shuffle(buf + len,len);
}


/* helper function to join two 16-bit ROMs and form a 32-bit data stream */
void konami_rom_deinterleave_2(int mem_region)
{
	shuffle((UINT16 *)memory_region(mem_region),memory_region_length(mem_region)/2);
}

/* helper function to join four 16-bit ROMs and form a 64-bit data stream */
void konami_rom_deinterleave_4(int mem_region)
{
	konami_rom_deinterleave_2(mem_region);
	konami_rom_deinterleave_2(mem_region);
}






/***************************************************************************/
/*                                                                         */
/*                                 007121                                  */
/*                                                                         */
/***************************************************************************/


/*static*/ unsigned char K007121_ctrlram[MAX_K007121][8];
static int K007121_flipscreen[MAX_K007121];


void K007121_ctrl_w(int chip,int offset,int data)
{
	switch (offset)
	{
		case 6:
/* palette bank change */
if ((K007121_ctrlram[chip][offset] & 0x30) != (data & 0x30))
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
			break;
		case 7:
			K007121_flipscreen[chip] = data & 0x08;
			break;
	}

	K007121_ctrlram[chip][offset] = data;
}

WRITE_HANDLER( K007121_ctrl_0_w )
{
	K007121_ctrl_w(0,offset,data);
}

WRITE_HANDLER( K007121_ctrl_1_w )
{
	K007121_ctrl_w(1,offset,data);
}


/*
 * Sprite Format
 * ------------------
 *
 * There are 0x40 sprites, each one using 5 bytes. However the number of
 * sprites can be increased to 0x80 with a control register (Combat School
 * sets it on and off during the game).
 *
 * Byte | Bit(s)   | Use
 * -----+-76543210-+----------------
 *   0  | xxxxxxxx | sprite code
 *   1  | xxxx---- | color
 *   1  | ----xx-- | sprite code low 2 bits for 16x8/8x8 sprites
 *   1  | ------xx | sprite code bank bits 1/0
 *   2  | xxxxxxxx | y position
 *   3  | xxxxxxxx | x position (low 8 bits)
 *   4  | xx------ | sprite code bank bits 3/2
 *   4  | --x----- | flip y
 *   4  | ---x---- | flip x
 *   4  | ----xxx- | sprite size 000=16x16 001=16x8 010=8x16 011=8x8 100=32x32
 *   4  | -------x | x position (high bit)
 *
 * Flack Attack uses a different, "wider" layout with 32 bytes per sprite,
 * mapped as follows, and the priority order is reversed. Maybe it is a
 * compatibility mode with an older custom IC. It is not known how this
 * alternate layout is selected.
 *
 * 0 -> e
 * 1 -> f
 * 2 -> 6
 * 3 -> 4
 * 4 -> 8
 *
 */

void K007121_sprites_draw(int chip,struct mame_bitmap *bitmap,const struct rectangle *cliprect,
		const unsigned char *source,int base_color,int global_x_offset,int bank_base,
		UINT32 pri_mask)
{
	const struct GfxElement *gfx = Machine->gfx[chip];
	int flipscreen = K007121_flipscreen[chip];
	int i,num,inc,offs[5],trans;
	int is_flakatck = K007121_ctrlram[chip][0x06] & 0x04;	/* WRONG!!!! */

#if 0
usrintf_showmessage("%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
	K007121_ctrlram[0][0x00],K007121_ctrlram[0][0x01],K007121_ctrlram[0][0x02],K007121_ctrlram[0][0x03],K007121_ctrlram[0][0x04],K007121_ctrlram[0][0x05],K007121_ctrlram[0][0x06],K007121_ctrlram[0][0x07],
	K007121_ctrlram[1][0x00],K007121_ctrlram[1][0x01],K007121_ctrlram[1][0x02],K007121_ctrlram[1][0x03],K007121_ctrlram[1][0x04],K007121_ctrlram[1][0x05],K007121_ctrlram[1][0x06],K007121_ctrlram[1][0x07]);
#endif
#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen(chip?"SPRITE1.DMP":"SPRITE0.DMP", "w+b");
	if (fp)
	{
		fwrite(source, 0x800, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif

	if (is_flakatck)
	{
		num = 0x40;
		inc = -0x20;
		source += 0x3f*0x20;
		offs[0] = 0x0e;
		offs[1] = 0x0f;
		offs[2] = 0x06;
		offs[3] = 0x04;
		offs[4] = 0x08;
		/* Flak Attack doesn't use a lookup PROM, it maps the color code directly */
		/* to a palette entry */
		trans = TRANSPARENCY_PEN;
	}
	else	/* all others */
	{
		//num = (K007121_ctrlram[chip][0x03] & 0x40) ? 0x80 : 0x40;	/* WRONG!!! (needed by combasc)  */
		num = 0x40; //AT: Combasc writes 70 sprites to VRAM at peak but the chip only processes the first 64.

		inc = 5;
		offs[0] = 0x00;
		offs[1] = 0x01;
		offs[2] = 0x02;
		offs[3] = 0x03;
		offs[4] = 0x04;
		trans = TRANSPARENCY_COLOR;
		/* when using priority buffer, draw front to back */
		if (pri_mask != -1)
		{
			source += (num-1)*inc;
			inc = -inc;
		}
	}

	for (i = 0;i < num;i++)
	{
		int number = source[offs[0]];				/* sprite number */
		int sprite_bank = source[offs[1]] & 0x0f;	/* sprite bank */
		int sx = source[offs[3]];					/* vertical position */
		int sy = source[offs[2]];					/* horizontal position */
		int attr = source[offs[4]];				/* attributes */
		int xflip = source[offs[4]] & 0x10;		/* flip x */
		int yflip = source[offs[4]] & 0x20;		/* flip y */
		int color = base_color + ((source[offs[1]] & 0xf0) >> 4);
		int width,height;
		static int x_offset[4] = {0x0,0x1,0x4,0x5};
		static int y_offset[4] = {0x0,0x2,0x8,0xa};
		int x,y, ex, ey;

		if (attr & 0x01) sx -= 256;
		if (sy >= 240) sy -= 256;

		number += ((sprite_bank & 0x3) << 8) + ((attr & 0xc0) << 4);
		number = number << 2;
		number += (sprite_bank >> 2) & 3;

		if (!is_flakatck || source[0x00])	/* Flak Attack needs this */
		{
			number += bank_base;

			switch( attr&0xe )
			{
				case 0x06: width = height = 1; break;
				case 0x04: width = 1; height = 2; number &= (~2); break;
				case 0x02: width = 2; height = 1; number &= (~1); break;
				case 0x00: width = height = 2; number &= (~3); break;
				case 0x08: width = height = 4; number &= (~3); break;
				default: width = 1; height = 1;
//					logerror("Unknown sprite size %02x\n",attr&0xe);
//					usrintf_showmessage("Unknown sprite size %02x\n",attr&0xe);
			}

			for (y = 0;y < height;y++)
			{
				for (x = 0;x < width;x++)
				{
					ex = xflip ? (width-1-x) : x;
					ey = yflip ? (height-1-y) : y;

					if (flipscreen)
					{
						if (pri_mask != -1)
							pdrawgfx(bitmap,gfx,
								number + x_offset[ex] + y_offset[ey],
								color,
								!xflip,!yflip,
								248-(sx+x*8),248-(sy+y*8),
								cliprect,trans,0,
								pri_mask);
						else
							drawgfx(bitmap,gfx,
								number + x_offset[ex] + y_offset[ey],
								color,
								!xflip,!yflip,
								248-(sx+x*8),248-(sy+y*8),
								cliprect,trans,0);
					}
					else
					{
						if (pri_mask != -1)
							pdrawgfx(bitmap,gfx,
								number + x_offset[ex] + y_offset[ey],
								color,
								xflip,yflip,
								global_x_offset+sx+x*8,sy+y*8,
								cliprect,trans,0,
								pri_mask);
						else
							drawgfx(bitmap,gfx,
								number + x_offset[ex] + y_offset[ey],
								color,
								xflip,yflip,
								global_x_offset+sx+x*8,sy+y*8,
								cliprect,trans,0);
					}
				}
			}
		}

		source += inc;
	}
}



/***************************************************************************/
/*                                                                         */
/*                                 007342                                  */
/*                                                                         */
/***************************************************************************/



static unsigned char *K007342_ram,*K007342_scroll_ram;
static int K007342_gfxnum;
static int K007342_int_enabled;
static int K007342_flipscreen;
static int K007342_scrollx[2];
static int K007342_scrolly[2];
static unsigned char *K007342_videoram_0,*K007342_colorram_0;
static unsigned char *K007342_videoram_1,*K007342_colorram_1;
static int K007342_regs[8];
static void (*K007342_callback)(int tilemap, int bank, int *code, int *color);
static struct tilemap *K007342_tilemap[2];

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/*
  data format:
  video RAM     xxxxxxxx    tile number (bits 0-7)
  color RAM     x-------    tiles with priority over the sprites
  color RAM     -x------    depends on external conections
  color RAM     --x-----    flip Y
  color RAM     ---x----    flip X
  color RAM     ----xxxx    depends on external connections (usually color and banking)
*/

static UINT32 K007342_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

INLINE void K007342_get_tile_info(int tile_index,int layer,data8_t *cram,data8_t *vram)
{
	int color, code;

	color = cram[tile_index];
	code = vram[tile_index];

	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
	tile_info.priority = (color & 0x80) >> 7;

	(*K007342_callback)(layer, K007342_regs[1], &code, &color);

	SET_TILE_INFO(
			K007342_gfxnum,
			code,
			color,
			tile_info.flags)
}

static void K007342_get_tile_info0(int tile_index) { K007342_get_tile_info(tile_index,0,K007342_colorram_0,K007342_videoram_0); }
static void K007342_get_tile_info1(int tile_index) { K007342_get_tile_info(tile_index,1,K007342_colorram_1,K007342_videoram_1); }



int K007342_vh_start(int gfx_index, void (*callback)(int tilemap, int bank, int *code, int *color))
{
	K007342_gfxnum = gfx_index;
	K007342_callback = callback;

	K007342_tilemap[0] = tilemap_create(K007342_get_tile_info0,K007342_scan,TILEMAP_TRANSPARENT,8,8,64,32);
	K007342_tilemap[1] = tilemap_create(K007342_get_tile_info1,K007342_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	K007342_ram = auto_malloc(0x2000);
	K007342_scroll_ram = auto_malloc(0x0200);

	if (!K007342_ram || !K007342_scroll_ram || !K007342_tilemap[0] || !K007342_tilemap[1])
		return 1;

	memset(K007342_ram,0,0x2000);

	K007342_colorram_0 = &K007342_ram[0x0000];
	K007342_colorram_1 = &K007342_ram[0x1000];
	K007342_videoram_0 = &K007342_ram[0x0800];
	K007342_videoram_1 = &K007342_ram[0x1800];

	tilemap_set_transparent_pen(K007342_tilemap[0],0);
	tilemap_set_transparent_pen(K007342_tilemap[1],0);

	return 0;
}

READ_HANDLER( K007342_r )
{
	return K007342_ram[offset];
}

WRITE_HANDLER( K007342_w )
{
	if (offset < 0x1000)
	{		/* layer 0 */
		if (K007342_ram[offset] != data)
		{
			K007342_ram[offset] = data;
			tilemap_mark_tile_dirty(K007342_tilemap[0],offset & 0x7ff);
		}
	}
	else
	{						/* layer 1 */
		if (K007342_ram[offset] != data)
		{
			K007342_ram[offset] = data;
			tilemap_mark_tile_dirty(K007342_tilemap[1],offset & 0x7ff);
		}
	}
}

READ_HANDLER( K007342_scroll_r )
{
	return K007342_scroll_ram[offset];
}

WRITE_HANDLER( K007342_scroll_w )
{
	K007342_scroll_ram[offset] = data;
}

WRITE_HANDLER( K007342_vreg_w )
{
	switch(offset)
	{
		case 0x00:
			/* bit 1: INT control */
			K007342_int_enabled = data & 0x02;
			K007342_flipscreen = data & 0x10;
			tilemap_set_flip(K007342_tilemap[0],K007342_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			tilemap_set_flip(K007342_tilemap[1],K007342_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			break;
		case 0x01:  /* used for banking in Rock'n'Rage */
			if (data != K007342_regs[1])
				tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		case 0x02:
			K007342_scrollx[0] = (K007342_scrollx[0] & 0xff) | ((data & 0x01) << 8);
			K007342_scrollx[1] = (K007342_scrollx[1] & 0xff) | ((data & 0x02) << 7);
			break;
		case 0x03:  /* scroll x (register 0) */
			K007342_scrollx[0] = (K007342_scrollx[0] & 0x100) | data;
			break;
		case 0x04:  /* scroll y (register 0) */
			K007342_scrolly[0] = data;
			break;
		case 0x05:  /* scroll x (register 1) */
			K007342_scrollx[1] = (K007342_scrollx[1] & 0x100) | data;
			break;
		case 0x06:  /* scroll y (register 1) */
			K007342_scrolly[1] = data;
		case 0x07:  /* unused */
			break;
	}
	K007342_regs[offset] = data;
}

void K007342_tilemap_update(void)
{
	int offs;


	/* update scroll */
	switch (K007342_regs[2] & 0x1c)
	{
		case 0x00:
		case 0x08:	/* unknown, blades of steel shootout between periods */
			tilemap_set_scroll_rows(K007342_tilemap[0],1);
			tilemap_set_scroll_cols(K007342_tilemap[0],1);
			tilemap_set_scrollx(K007342_tilemap[0],0,K007342_scrollx[0]);
			tilemap_set_scrolly(K007342_tilemap[0],0,K007342_scrolly[0]);
			break;

		case 0x0c:	/* 32 columns */
			tilemap_set_scroll_rows(K007342_tilemap[0],1);
			tilemap_set_scroll_cols(K007342_tilemap[0],512);
			tilemap_set_scrollx(K007342_tilemap[0],0,K007342_scrollx[0]);
			for (offs = 0;offs < 256;offs++)
				tilemap_set_scrolly(K007342_tilemap[0],(offs + K007342_scrollx[0]) & 0x1ff,
						K007342_scroll_ram[2*(offs/8)] + 256 * K007342_scroll_ram[2*(offs/8)+1]);
			break;

		case 0x14:	/* 256 rows */
			tilemap_set_scroll_rows(K007342_tilemap[0],256);
			tilemap_set_scroll_cols(K007342_tilemap[0],1);
			tilemap_set_scrolly(K007342_tilemap[0],0,K007342_scrolly[0]);
			for (offs = 0;offs < 256;offs++)
				tilemap_set_scrollx(K007342_tilemap[0],(offs + K007342_scrolly[0]) & 0xff,
						K007342_scroll_ram[2*offs] + 256 * K007342_scroll_ram[2*offs+1]);
			break;

		default:
usrintf_showmessage("unknown scroll ctrl %02x",K007342_regs[2] & 0x1c);
			break;
	}

	tilemap_set_scrollx(K007342_tilemap[1],0,K007342_scrollx[1]);
	tilemap_set_scrolly(K007342_tilemap[1],0,K007342_scrolly[1]);

#if 0
	{
		static int current_layer = 0;

		if (keyboard_pressed_memory(KEYCODE_Z)) current_layer = !current_layer;
		tilemap_set_enable(K007342_tilemap[current_layer], 1);
		tilemap_set_enable(K007342_tilemap[!current_layer], 0);

		usrintf_showmessage("regs:%02x %02x %02x %02x-%02x %02x %02x %02x:%02x",
			K007342_regs[0], K007342_regs[1], K007342_regs[2], K007342_regs[3],
			K007342_regs[4], K007342_regs[5], K007342_regs[6], K007342_regs[7],
			current_layer);
	}
#endif
}

void K007342_tilemap_set_enable(int tilemap, int enable)
{
	tilemap_set_enable(K007342_tilemap[tilemap], enable);
}

void K007342_tilemap_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int num,int flags,UINT32 priority)
{
	tilemap_draw(bitmap,cliprect,K007342_tilemap[num],flags,priority);
}

int K007342_is_INT_enabled(void)
{
	return K007342_int_enabled;
}



static struct GfxElement *K007420_gfx;
static void (*K007420_callback)(int *code,int *color);
static unsigned char *K007420_ram;

int K007420_vh_start(int gfxnum, void (*callback)(int *code,int *color))
{
	K007420_gfx = Machine->gfx[gfxnum];
	K007420_callback = callback;
	K007420_ram = auto_malloc(0x200);
	if (!K007420_ram) return 1;

	memset(K007420_ram,0,0x200);

	return 0;
}

READ_HANDLER( K007420_r )
{
	return K007420_ram[offset];
}

WRITE_HANDLER( K007420_w )
{
	K007420_ram[offset] = data;
}

/*
 * Sprite Format
 * ------------------
 *
 * Byte | Bit(s)   | Use
 * -----+-76543210-+----------------
 *   0  | xxxxxxxx | y position
 *   1  | xxxxxxxx | sprite code (low 8 bits)
 *   2  | xxxxxxxx | depends on external conections. Usually banking
 *   3  | xxxxxxxx | x position (low 8 bits)
 *   4  | x------- | x position (high bit)
 *   4  | -xxx---- | sprite size 000=16x16 001=8x16 010=16x8 011=8x8 100=32x32
 *   4  | ----x--- | flip y
 *   4  | -----x-- | flip x
 *   4  | ------xx | zoom (bits 8 & 9)
 *   5  | xxxxxxxx | zoom (low 8 bits)  0x080 = normal, < 0x80 enlarge, > 0x80 reduce
 *   6  | xxxxxxxx | unused
 *   7  | xxxxxxxx | unused
 */

void K007420_sprites_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
#define K007420_SPRITERAM_SIZE 0x200
	int offs;

	for (offs = K007420_SPRITERAM_SIZE - 8; offs >= 0; offs -= 8)
	{
		int ox,oy,code,color,flipx,flipy,zoom,w,h,x,y;
		static int xoffset[4] = { 0, 1, 4, 5 };
		static int yoffset[4] = { 0, 2, 8, 10 };

		code = K007420_ram[offs+1];
		color = K007420_ram[offs+2];
		ox = K007420_ram[offs+3] - ((K007420_ram[offs+4] & 0x80) << 1);
		oy = 256 - K007420_ram[offs+0];
		flipx = K007420_ram[offs+4] & 0x04;
		flipy = K007420_ram[offs+4] & 0x08;

		(*K007420_callback)(&code,&color);

		/* kludge for rock'n'rage */
		if ((K007420_ram[offs+4] == 0x40) && (K007420_ram[offs+1] == 0xff) &&
			(K007420_ram[offs+2] == 0x00) && (K007420_ram[offs+5] == 0xf0)) continue;

		/* 0x080 = normal scale, 0x040 = double size, 0x100 half size */
		zoom = K007420_ram[offs+5] | ((K007420_ram[offs+4] & 0x03) << 8);
		if (!zoom) continue;
		zoom = 0x10000 * 128 / zoom;

		switch (K007420_ram[offs+4] & 0x70)
		{
			case 0x30: w = h = 1; break;
			case 0x20: w = 2; h = 1; code &= (~1); break;
			case 0x10: w = 1; h = 2; code &= (~2); break;
			case 0x00: w = h = 2; code &= (~3); break;
			case 0x40: w = h = 4; code &= (~3); break;
			default: w = 1; h = 1;
//logerror("Unknown sprite size %02x\n",(K007420_ram[offs+4] & 0x70)>>4);
		}

		if (K007342_flipscreen)
		{
			ox = 256 - ox - ((zoom * w + (1<<12)) >> 13);
			oy = 256 - oy - ((zoom * h + (1<<12)) >> 13);
			flipx = !flipx;
			flipy = !flipy;
		}

		if (zoom == 0x10000)
		{
			int sx,sy;

			for (y = 0;y < h;y++)
			{
				sy = oy + 8 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 8 * x;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfx(bitmap,K007420_gfx,
						c,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0);

					if (K007342_regs[2] & 0x80)
						drawgfx(bitmap,K007420_gfx,
							c,
							color,
							flipx,flipy,
							sx,sy-256,
							cliprect,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			int sx,sy,zw,zh;
			for (y = 0;y < h;y++)
			{
				sy = oy + ((zoom * y + (1<<12)) >> 13);
				zh = (oy + ((zoom * (y+1) + (1<<12)) >> 13)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoom * x + (1<<12)) >> 13);
					zw = (ox + ((zoom * (x+1) + (1<<12)) >> 13)) - sx;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfxzoom(bitmap,K007420_gfx,
						c,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0,
						(zw << 16) / 8,(zh << 16) / 8);

					if (K007342_regs[2] & 0x80)
						drawgfxzoom(bitmap,K007420_gfx,
							c,
							color,
							flipx,flipy,
							sx,sy-256,
							cliprect,TRANSPARENCY_PEN,0,
							(zw << 16) / 8,(zh << 16) / 8);
				}
			}
		}
	}
#if 0
	{
		static int current_sprite = 0;

		if (keyboard_pressed_memory(KEYCODE_Z)) current_sprite = (current_sprite+1) & ((K007420_SPRITERAM_SIZE/8)-1);
		if (keyboard_pressed_memory(KEYCODE_X)) current_sprite = (current_sprite-1) & ((K007420_SPRITERAM_SIZE/8)-1);

		usrintf_showmessage("%02x:%02x %02x %02x %02x %02x %02x %02x %02x", current_sprite,
			K007420_ram[(current_sprite*8)+0], K007420_ram[(current_sprite*8)+1],
			K007420_ram[(current_sprite*8)+2], K007420_ram[(current_sprite*8)+3],
			K007420_ram[(current_sprite*8)+4], K007420_ram[(current_sprite*8)+5],
			K007420_ram[(current_sprite*8)+6], K007420_ram[(current_sprite*8)+7]);
	}
#endif
}




/***************************************************************************/
/*                                                                         */
/*                                 052109                                  */
/*                                                                         */
/***************************************************************************/



static int K052109_memory_region;
static int K052109_gfxnum;
static void (*K052109_callback)(int tilemap,int bank,int *code,int *color);
static unsigned char *K052109_ram;
static unsigned char *K052109_videoram_F,*K052109_videoram2_F,*K052109_colorram_F;
static unsigned char *K052109_videoram_A,*K052109_videoram2_A,*K052109_colorram_A;
static unsigned char *K052109_videoram_B,*K052109_videoram2_B,*K052109_colorram_B;
static unsigned char K052109_charrombank[4];
static int has_extra_video_ram;
static int K052109_RMRD_line;
static int K052109_tileflip_enable;
static int K052109_irq_enabled;
static unsigned char K052109_romsubbank,K052109_scrollctrl;
struct tilemap *K052109_tilemap[3];



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/*
  data format:
  video RAM    xxxxxxxx  tile number (low 8 bits)
  color RAM    xxxx----  depends on external connections (usually color and banking)
  color RAM    ----xx--  bank select (0-3): these bits are replaced with the 2
                         bottom bits of the bank register before being placed on
                         the output pins. The other two bits of the bank register are
                         placed on the CAB1 and CAB2 output pins.
  color RAM    ------xx  depends on external connections (usually banking, flip)
*/

INLINE void K052109_get_tile_info(int tile_index,int layer,data8_t *cram,data8_t *vram1,data8_t *vram2)
{
	int flipy = 0;
	int code = vram1[tile_index] + 256 * vram2[tile_index];
	int color = cram[tile_index];
	int bank = K052109_charrombank[(color & 0x0c) >> 2];
if (has_extra_video_ram) bank = (color & 0x0c) >> 2;	/* kludge for X-Men */
	color = (color & 0xf3) | ((bank & 0x03) << 2);
	bank >>= 2;

	flipy = color & 0x02;

	tile_info.flags = 0;

	(*K052109_callback)(layer,bank,&code,&color);

	SET_TILE_INFO(
			K052109_gfxnum,
			code,
			color,
			tile_info.flags);

	/* if the callback set flip X but it is not enabled, turn it off */
	if (!(K052109_tileflip_enable & 1)) tile_info.flags &= ~TILE_FLIPX;

	/* if flip Y is enabled and the attribute but is set, turn it on */
	if (flipy && (K052109_tileflip_enable & 2)) tile_info.flags |= TILE_FLIPY;
}

static void K052109_get_tile_info0(int tile_index) { K052109_get_tile_info(tile_index,0,K052109_colorram_F,K052109_videoram_F,K052109_videoram2_F); }
static void K052109_get_tile_info1(int tile_index) { K052109_get_tile_info(tile_index,1,K052109_colorram_A,K052109_videoram_A,K052109_videoram2_A); }
static void K052109_get_tile_info2(int tile_index) { K052109_get_tile_info(tile_index,2,K052109_colorram_B,K052109_videoram_B,K052109_videoram2_B); }


static void K052109_tileflip_reset(void)
{
	int data = K052109_ram[0x1e80];
	tilemap_set_flip(K052109_tilemap[0],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	tilemap_set_flip(K052109_tilemap[1],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	tilemap_set_flip(K052109_tilemap[2],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	K052109_tileflip_enable = ((data & 0x06) >> 1);
}


int K052109_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int tilemap,int bank,int *code,int *color))
{
	int gfx_index;
	static struct GfxLayout charlayout =
	{
		8,8,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
		32*8
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	charlayout.total = memory_region_length(gfx_memory_region) / 32;
	charlayout.planeoffset[0] = plane3 * 8;
	charlayout.planeoffset[1] = plane2 * 8;
	charlayout.planeoffset[2] = plane1 * 8;
	charlayout.planeoffset[3] = plane0 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&charlayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	K052109_memory_region = gfx_memory_region;
	K052109_gfxnum = gfx_index;
	K052109_callback = callback;
	K052109_RMRD_line = CLEAR_LINE;

	has_extra_video_ram = 0;

	K052109_tilemap[0] = tilemap_create(K052109_get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	K052109_tilemap[1] = tilemap_create(K052109_get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	K052109_tilemap[2] = tilemap_create(K052109_get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	K052109_ram = auto_malloc(0x6000);

	if (!K052109_ram || !K052109_tilemap[0] || !K052109_tilemap[1] || !K052109_tilemap[2])
		return 1;

	memset(K052109_ram,0,0x6000);

	K052109_colorram_F = &K052109_ram[0x0000];
	K052109_colorram_A = &K052109_ram[0x0800];
	K052109_colorram_B = &K052109_ram[0x1000];
	K052109_videoram_F = &K052109_ram[0x2000];
	K052109_videoram_A = &K052109_ram[0x2800];
	K052109_videoram_B = &K052109_ram[0x3000];
	K052109_videoram2_F = &K052109_ram[0x4000];
	K052109_videoram2_A = &K052109_ram[0x4800];
	K052109_videoram2_B = &K052109_ram[0x5000];

	tilemap_set_transparent_pen(K052109_tilemap[0],0);
	tilemap_set_transparent_pen(K052109_tilemap[1],0);
	tilemap_set_transparent_pen(K052109_tilemap[2],0);

	state_save_register_UINT8("k052109", 0, "ram",        K052109_ram, 0x6000);
	state_save_register_int  ("k052109", 0, "rmrd",       &K052109_RMRD_line);
	state_save_register_UINT8("k052109", 0, "romsubbank", &K052109_romsubbank, 1);
	state_save_register_UINT8("k052109", 0, "scrollctrl", &K052109_scrollctrl, 1);
	state_save_register_int  ("k052109", 0, "irqen",      &K052109_irq_enabled);
	state_save_register_UINT8("k052109", 0, "charbank",   K052109_charrombank, 4);
	state_save_register_int  ("k052109", 0, "extra",      &has_extra_video_ram);

	state_save_register_func_postload(K052109_tileflip_reset);
	return 0;
}



READ_HANDLER( K052109_r )
{
	if (K052109_RMRD_line == CLEAR_LINE)
	{
		if ((offset & 0x1fff) >= 0x1800)
		{
			if (offset >= 0x180c && offset < 0x1834)
			{	/* A y scroll */	}
			else if (offset >= 0x1a00 && offset < 0x1c00)
			{	/* A x scroll */	}
			else if (offset == 0x1d00)
			{	/* read for bitwise operations before writing */	}
			else if (offset >= 0x380c && offset < 0x3834)
			{	/* B y scroll */	}
			else if (offset >= 0x3a00 && offset < 0x3c00)
			{	/* B x scroll */	}
			else
logerror("%04x: read from unknown 052109 address %04x\n",activecpu_get_pc(),offset);
		}

		return K052109_ram[offset];
	}
	else	/* Punk Shot and TMNT read from 0000-1fff, Aliens from 2000-3fff */
	{
		int code = (offset & 0x1fff) >> 5;
		int color = K052109_romsubbank;
		int bank = K052109_charrombank[(color & 0x0c) >> 2] >> 2;   /* discard low bits (TMNT) */
		int addr;

if (has_extra_video_ram) code |= color << 8;	/* kludge for X-Men */
else
		(*K052109_callback)(0,bank,&code,&color);

		addr = (code << 5) + (offset & 0x1f);
		addr &= memory_region_length(K052109_memory_region)-1;

#if 0
	usrintf_showmessage("%04x: off%04x sub%02x (bnk%x) adr%06x",activecpu_get_pc(),offset,K052109_romsubbank,bank,addr);
#endif

		return memory_region(K052109_memory_region)[addr];
	}
}

WRITE_HANDLER( K052109_w )
{
	if ((offset & 0x1fff) < 0x1800) /* tilemap RAM */
	{
		if (K052109_ram[offset] != data)
		{
			if (offset >= 0x4000) has_extra_video_ram = 1;  /* kludge for X-Men */
			K052109_ram[offset] = data;
			tilemap_mark_tile_dirty(K052109_tilemap[(offset & 0x1800) >> 11],offset & 0x7ff);
		}
	}
	else	/* control registers */
	{
		K052109_ram[offset] = data;

		if (offset >= 0x180c && offset < 0x1834)
		{	/* A y scroll */	}
		else if (offset >= 0x1a00 && offset < 0x1c00)
		{	/* A x scroll */	}
		else if (offset == 0x1c80)
		{
if (K052109_scrollctrl != data)
{
#if 0
usrintf_showmessage("scrollcontrol = %02x",data);
#endif
logerror("%04x: rowscrollcontrol = %02x\n",activecpu_get_pc(),data);
			K052109_scrollctrl = data;
}
		}
		else if (offset == 0x1d00)
		{
#if VERBOSE
logerror("%04x: 052109 register 1d00 = %02x\n",activecpu_get_pc(),data);
#endif
			/* bit 2 = irq enable */
			/* the custom chip can also generate NMI and FIRQ, for use with a 6809 */
			K052109_irq_enabled = data & 0x04;
		}
		else if (offset == 0x1d80)
		{
			int dirty = 0;

			if (K052109_charrombank[0] != (data & 0x0f)) dirty |= 1;
			if (K052109_charrombank[1] != ((data >> 4) & 0x0f)) dirty |= 2;
			if (dirty)
			{
				int i;

				K052109_charrombank[0] = data & 0x0f;
				K052109_charrombank[1] = (data >> 4) & 0x0f;

				for (i = 0;i < 0x1800;i++)
				{
					int bank = (K052109_ram[i]&0x0c) >> 2;
					if ((bank == 0 && (dirty & 1)) || (bank == 1 && dirty & 2))
					{
						tilemap_mark_tile_dirty(K052109_tilemap[(i & 0x1800) >> 11],i & 0x7ff);
					}
				}
			}
		}
		else if (offset == 0x1e00)
		{
logerror("%04x: 052109 register 1e00 = %02x\n",activecpu_get_pc(),data);
			K052109_romsubbank = data;
		}
		else if (offset == 0x1e80)
		{
if ((data & 0xfe)) logerror("%04x: 052109 register 1e80 = %02x\n",activecpu_get_pc(),data);
			tilemap_set_flip(K052109_tilemap[0],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			tilemap_set_flip(K052109_tilemap[1],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			tilemap_set_flip(K052109_tilemap[2],(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
			if (K052109_tileflip_enable != ((data & 0x06) >> 1))
			{
				K052109_tileflip_enable = ((data & 0x06) >> 1);

				tilemap_mark_all_tiles_dirty(K052109_tilemap[0]);
				tilemap_mark_all_tiles_dirty(K052109_tilemap[1]);
				tilemap_mark_all_tiles_dirty(K052109_tilemap[2]);
			}
		}
		else if (offset == 0x1f00)
		{
			int dirty = 0;

			if (K052109_charrombank[2] != (data & 0x0f)) dirty |= 1;
			if (K052109_charrombank[3] != ((data >> 4) & 0x0f)) dirty |= 2;
			if (dirty)
			{
				int i;

				K052109_charrombank[2] = data & 0x0f;
				K052109_charrombank[3] = (data >> 4) & 0x0f;

				for (i = 0;i < 0x1800;i++)
				{
					int bank = (K052109_ram[i] & 0x0c) >> 2;
					if ((bank == 2 && (dirty & 1)) || (bank == 3 && dirty & 2))
						tilemap_mark_tile_dirty(K052109_tilemap[(i & 0x1800) >> 11],i & 0x7ff);
				}
			}
		}
		else if (offset >= 0x380c && offset < 0x3834)
		{	/* B y scroll */	}
		else if (offset >= 0x3a00 && offset < 0x3c00)
		{	/* B x scroll */	}
		else
logerror("%04x: write %02x to unknown 052109 address %04x\n",activecpu_get_pc(),data,offset);
	}
}

READ16_HANDLER( K052109_word_r )
{
	return K052109_r(offset + 0x2000) | (K052109_r(offset) << 8);
}

WRITE16_HANDLER( K052109_word_w )
{
	if (ACCESSING_MSB)
		K052109_w(offset,(data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K052109_w(offset + 0x2000,data & 0xff);
}

READ16_HANDLER(K052109_lsb_r)
{
	return K052109_r(offset);
}

WRITE16_HANDLER(K052109_lsb_w)
{
	if(ACCESSING_LSB)
		K052109_w(offset, data & 0xff);
}

void K052109_set_RMRD_line(int state)
{
	K052109_RMRD_line = state;
}


void K052109_tilemap_update(void)
{
#if 0
{
usrintf_showmessage("%x %x %x %x",
	K052109_charrombank[0],
	K052109_charrombank[1],
	K052109_charrombank[2],
	K052109_charrombank[3]);
}
#endif
	if ((K052109_scrollctrl & 0x03) == 0x02)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(K052109_tilemap[1],256);
		tilemap_set_scroll_cols(K052109_tilemap[1],1);
		yscroll = K052109_ram[0x180c];
		tilemap_set_scrolly(K052109_tilemap[1],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*(offs&0xfff8)+0] + 256 * scrollram[2*(offs&0xfff8)+1];
			xscroll -= 6;
			tilemap_set_scrollx(K052109_tilemap[1],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((K052109_scrollctrl & 0x03) == 0x03)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(K052109_tilemap[1],256);
		tilemap_set_scroll_cols(K052109_tilemap[1],1);
		yscroll = K052109_ram[0x180c];
		tilemap_set_scrolly(K052109_tilemap[1],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*offs+0] + 256 * scrollram[2*offs+1];
			xscroll -= 6;
			tilemap_set_scrollx(K052109_tilemap[1],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((K052109_scrollctrl & 0x04) == 0x04)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1800];


		tilemap_set_scroll_rows(K052109_tilemap[1],1);
		tilemap_set_scroll_cols(K052109_tilemap[1],512);
		xscroll = K052109_ram[0x1a00] + 256 * K052109_ram[0x1a01];
		xscroll -= 6;
		tilemap_set_scrollx(K052109_tilemap[1],0,xscroll);
		for (offs = 0;offs < 512;offs++)
		{
			yscroll = scrollram[offs/8];
			tilemap_set_scrolly(K052109_tilemap[1],(offs+xscroll)&0x1ff,yscroll);
		}
	}
	else
	{
		int xscroll,yscroll;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(K052109_tilemap[1],1);
		tilemap_set_scroll_cols(K052109_tilemap[1],1);
		xscroll = scrollram[0] + 256 * scrollram[1];
		xscroll -= 6;
		yscroll = K052109_ram[0x180c];
		tilemap_set_scrollx(K052109_tilemap[1],0,xscroll);
		tilemap_set_scrolly(K052109_tilemap[1],0,yscroll);
	}

	if ((K052109_scrollctrl & 0x18) == 0x10)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(K052109_tilemap[2],256);
		tilemap_set_scroll_cols(K052109_tilemap[2],1);
		yscroll = K052109_ram[0x380c];
		tilemap_set_scrolly(K052109_tilemap[2],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*(offs&0xfff8)+0] + 256 * scrollram[2*(offs&0xfff8)+1];
			xscroll -= 6;
			tilemap_set_scrollx(K052109_tilemap[2],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((K052109_scrollctrl & 0x18) == 0x18)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(K052109_tilemap[2],256);
		tilemap_set_scroll_cols(K052109_tilemap[2],1);
		yscroll = K052109_ram[0x380c];
		tilemap_set_scrolly(K052109_tilemap[2],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*offs+0] + 256 * scrollram[2*offs+1];
			xscroll -= 6;
			tilemap_set_scrollx(K052109_tilemap[2],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((K052109_scrollctrl & 0x20) == 0x20)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3800];


		tilemap_set_scroll_rows(K052109_tilemap[2],1);
		tilemap_set_scroll_cols(K052109_tilemap[2],512);
		xscroll = K052109_ram[0x3a00] + 256 * K052109_ram[0x3a01];
		xscroll -= 6;
		tilemap_set_scrollx(K052109_tilemap[2],0,xscroll);
		for (offs = 0;offs < 512;offs++)
		{
			yscroll = scrollram[offs/8];
			tilemap_set_scrolly(K052109_tilemap[2],(offs+xscroll)&0x1ff,yscroll);
		}
	}
	else
	{
		int xscroll,yscroll;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(K052109_tilemap[2],1);
		tilemap_set_scroll_cols(K052109_tilemap[2],1);
		xscroll = scrollram[0] + 256 * scrollram[1];
		xscroll -= 6;
		yscroll = K052109_ram[0x380c];
		tilemap_set_scrollx(K052109_tilemap[2],0,xscroll);
		tilemap_set_scrolly(K052109_tilemap[2],0,yscroll);
	}

#ifdef MAME_DEBUG
if ((K052109_scrollctrl & 0x03) == 0x01 ||
		(K052109_scrollctrl & 0x18) == 0x08 ||
		((K052109_scrollctrl & 0x04) && (K052109_scrollctrl & 0x03)) ||
		((K052109_scrollctrl & 0x20) && (K052109_scrollctrl & 0x18)) ||
		(K052109_scrollctrl & 0xc0) != 0)
	usrintf_showmessage("scrollcontrol = %02x",K052109_scrollctrl);
#endif

#if 0
if (keyboard_pressed(KEYCODE_F))
{
	FILE *fp;
	fp=fopen("TILE.DMP", "w+b");
	if (fp)
	{
		fwrite(K052109_ram, 0x6000, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
}

int K052109_is_IRQ_enabled(void)
{
	return K052109_irq_enabled;
}





/***************************************************************************/
/*                                                                         */
/*                                 051960                                  */
/*                                                                         */
/***************************************************************************/



static int K051960_memory_region;
static struct GfxElement *K051960_gfx;
static void (*K051960_callback)(int *code,int *color,int *priority,int *shadow);
static int K051960_romoffset;
static int K051960_spriteflip,K051960_readroms;
static unsigned char K051960_spriterombank[3];
static unsigned char *K051960_ram;
static int K051960_irq_enabled, K051960_nmi_enabled;


int K051960_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int *code,int *color,int *priority,int *shadow))
{
	int gfx_index,i;
	static struct GfxLayout spritelayout =
	{
		16,16,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 0, 1, 2, 3, 4, 5, 6, 7,
				8*32+0, 8*32+1, 8*32+2, 8*32+3, 8*32+4, 8*32+5, 8*32+6, 8*32+7 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
				16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
		128*8
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	spritelayout.total = memory_region_length(gfx_memory_region) / 128;
	spritelayout.planeoffset[0] = plane0 * 8;
	spritelayout.planeoffset[1] = plane1 * 8;
	spritelayout.planeoffset[2] = plane2 * 8;
	spritelayout.planeoffset[3] = plane3 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&spritelayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

if (!(Machine->drv->video_attributes & VIDEO_HAS_SHADOWS))
	usrintf_showmessage("driver should use VIDEO_HAS_SHADOWS");

	/* prepare shadow draw table */
	gfx_drawmode_table[0] = DRAWMODE_NONE;
	for (i = 1;i < 15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	gfx_drawmode_table[15] = DRAWMODE_SHADOW;

	K051960_memory_region = gfx_memory_region;
	K051960_gfx = Machine->gfx[gfx_index];
	K051960_callback = callback;
	K051960_ram = auto_malloc(0x400);
	if (!K051960_ram) return 1;
	memset(K051960_ram,0,0x400);

	return 0;
}


static int K051960_fetchromdata(int byte)
{
	int code,color,pri,shadow,off1,addr;


	addr = K051960_romoffset + (K051960_spriterombank[0] << 8) +
			((K051960_spriterombank[1] & 0x03) << 16);
	code = (addr & 0x3ffe0) >> 5;
	off1 = addr & 0x1f;
	color = ((K051960_spriterombank[1] & 0xfc) >> 2) + ((K051960_spriterombank[2] & 0x03) << 6);
	pri = 0;
	shadow = color & 0x80;
	(*K051960_callback)(&code,&color,&pri,&shadow);

	addr = (code << 7) | (off1 << 2) | byte;
	addr &= memory_region_length(K051960_memory_region)-1;

#if 0
	usrintf_showmessage("%04x: addr %06x",activecpu_get_pc(),addr);
#endif

	return memory_region(K051960_memory_region)[addr];
}

READ_HANDLER( K051960_r )
{
	if (K051960_readroms)
	{
		/* the 051960 remembers the last address read and uses it when reading the sprite ROMs */
		K051960_romoffset = (offset & 0x3fc) >> 2;
		return K051960_fetchromdata(offset & 3);	/* only 88 Games reads the ROMs from here */
	}
	else
		return K051960_ram[offset];
}

WRITE_HANDLER( K051960_w )
{
	K051960_ram[offset] = data;
}

READ16_HANDLER( K051960_word_r )
{
	return K051960_r(offset*2 + 1) | (K051960_r(offset*2) << 8);
}

WRITE16_HANDLER( K051960_word_w )
{
	if (ACCESSING_MSB)
		K051960_w(offset*2,(data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K051960_w(offset*2 + 1,data & 0xff);
}

READ_HANDLER( K051937_r )
{
	if (K051960_readroms && offset >= 4 && offset < 8)
	{
		return K051960_fetchromdata(offset & 3);
	}
	else
	{
		if (offset == 0)
		{
			static int counter;

			/* some games need bit 0 to pulse */
			return (counter++) & 1;
		}
logerror("%04x: read unknown 051937 address %x\n",activecpu_get_pc(),offset);
		return 0;
	}
}

WRITE_HANDLER( K051937_w )
{
	if (offset == 0)
	{
#ifdef MAME_DEBUG
if (data & 0xc2)
	usrintf_showmessage("051937 reg 00 = %02x",data);
#endif
		/* bit 0 is IRQ enable */
		K051960_irq_enabled = (data & 0x01);

		/* bit 1: probably FIRQ enable */

		/* bit 2 is NMI enable */
		K051960_nmi_enabled = (data & 0x04);

		/* bit 3 = flip screen */
		K051960_spriteflip = data & 0x08;

		/* bit 4 used by Devastators and TMNT, unknown */

		/* bit 5 = enable gfx ROM reading */
		K051960_readroms = data & 0x20;
#if VERBOSE
logerror("%04x: write %02x to 051937 address %x\n",activecpu_get_pc(),data,offset);
#endif
	}
	else if (offset == 1)
	{
#if 0
	usrintf_showmessage("%04x: write %02x to 051937 address %x",activecpu_get_pc(),data,offset);
#endif
logerror("%04x: write %02x to unknown 051937 address %x\n",activecpu_get_pc(),data,offset);
	}
	else if (offset >= 2 && offset < 5)
	{
		K051960_spriterombank[offset - 2] = data;
	}
	else
	{
#if 0
	usrintf_showmessage("%04x: write %02x to 051937 address %x",activecpu_get_pc(),data,offset);
#endif
logerror("%04x: write %02x to unknown 051937 address %x\n",activecpu_get_pc(),data,offset);
	}
}

READ16_HANDLER( K051937_word_r )
{
	return K051937_r(offset*2 + 1) | (K051937_r(offset*2) << 8);
}

WRITE16_HANDLER( K051937_word_w )
{
	if (ACCESSING_MSB)
		K051937_w(offset*2,(data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K051937_w(offset*2 + 1,data & 0xff);
}


/*
 * Sprite Format
 * ------------------
 *
 * Byte | Bit(s)   | Use
 * -----+-76543210-+----------------
 *   0  | x------- | active (show this sprite)
 *   0  | -xxxxxxx | priority order
 *   1  | xxx----- | sprite size (see below)
 *   1  | ---xxxxx | sprite code (high 5 bits)
 *   2  | xxxxxxxx | sprite code (low 8 bits)
 *   3  | xxxxxxxx | "color", but depends on external connections (see below)
 *   4  | xxxxxx-- | zoom y (0 = normal, >0 = shrink)
 *   4  | ------x- | flip y
 *   4  | -------x | y position (high bit)
 *   5  | xxxxxxxx | y position (low 8 bits)
 *   6  | xxxxxx-- | zoom x (0 = normal, >0 = shrink)
 *   6  | ------x- | flip x
 *   6  | -------x | x position (high bit)
 *   7  | xxxxxxxx | x position (low 8 bits)
 *
 * Example of "color" field for Punk Shot:
 *   3  | x------- | shadow
 *   3  | -xx----- | priority
 *   3  | ---x---- | use second gfx ROM bank
 *   3  | ----xxxx | color code
 *
 * shadow enables transparent shadows. Note that it applies to pen 0x0f ONLY.
 * The rest of the sprite remains normal.
 * Note that Aliens also uses the shadow bit to select the second sprite bank.
 */

void K051960_sprites_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int min_priority,int max_priority)
{
#define NUM_SPRITES 128
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < 0x400;offs += 8)
	{
		if (K051960_ram[offs] & 0x80)
		{
			if (max_priority == -1)	/* draw front to back when using priority buffer */
				sortedlist[(K051960_ram[offs] & 0x7f) ^ 0x7f] = offs;
			else
				sortedlist[K051960_ram[offs] & 0x7f] = offs;
		}
	}

	for (pri_code = 0;pri_code < NUM_SPRITES;pri_code++)
	{
		int ox,oy,code,color,pri,shadow,size,w,h,x,y,flipx,flipy,zoomx,zoomy;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
		static int width[8] =  { 1, 2, 1, 2, 4, 2, 4, 8 };
		static int height[8] = { 1, 1, 2, 2, 2, 4, 4, 8 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		code = K051960_ram[offs+2] + ((K051960_ram[offs+1] & 0x1f) << 8);
		color = K051960_ram[offs+3] & 0xff;
		pri = 0;
		shadow = color & 0x80;
		(*K051960_callback)(&code,&color,&pri,&shadow);

		if (max_priority != -1)
			if (pri < min_priority || pri > max_priority) continue;

		size = (K051960_ram[offs+1] & 0xe0) >> 5;
		w = width[size];
		h = height[size];

		if (w >= 2) code &= ~0x01;
		if (h >= 2) code &= ~0x02;
		if (w >= 4) code &= ~0x04;
		if (h >= 4) code &= ~0x08;
		if (w >= 8) code &= ~0x10;
		if (h >= 8) code &= ~0x20;

		ox = (256 * K051960_ram[offs+6] + K051960_ram[offs+7]) & 0x01ff;
		oy = 256 - ((256 * K051960_ram[offs+4] + K051960_ram[offs+5]) & 0x01ff);
		flipx = K051960_ram[offs+6] & 0x02;
		flipy = K051960_ram[offs+4] & 0x02;
		zoomx = (K051960_ram[offs+6] & 0xfc) >> 2;
		zoomy = (K051960_ram[offs+4] & 0xfc) >> 2;
		zoomx = 0x10000 / 128 * (128 - zoomx);
		zoomy = 0x10000 / 128 * (128 - zoomy);

		if (K051960_spriteflip)
		{
			ox = 512 - (zoomx * w >> 12) - ox;
			oy = 256 - (zoomy * h >> 12) - oy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (zoomx == 0x10000 && zoomy == 0x10000)
		{
			int sx,sy;

			for (y = 0;y < h;y++)
			{
				sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 16 * x;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					if (max_priority == -1)
						pdrawgfx(bitmap,K051960_gfx,
								c,
								color,
								flipx,flipy,
								sx & 0x1ff,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,pri);
					else
						drawgfx(bitmap,K051960_gfx,
								c,
								color,
								flipx,flipy,
								sx & 0x1ff,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			int sx,sy,zw,zh;

			for (y = 0;y < h;y++)
			{
				sy = oy + ((zoomy * y + (1<<11)) >> 12);
				zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoomx * x + (1<<11)) >> 12);
					zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					if (max_priority == -1)
						pdrawgfxzoom(bitmap,K051960_gfx,
								c,
								color,
								flipx,flipy,
								sx & 0x1ff,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,
								(zw << 16) / 16,(zh << 16) / 16,pri);
					else
						drawgfxzoom(bitmap,K051960_gfx,
								c,
								color,
								flipx,flipy,
								sx & 0x1ff,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,
								(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(K051960_ram, 0x400, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
#undef NUM_SPRITES
}

int K051960_is_IRQ_enabled(void)
{
	return K051960_irq_enabled;
}

int K051960_is_NMI_enabled(void)
{
	return K051960_nmi_enabled;
}




READ_HANDLER( K052109_051960_r )
{
	if (K052109_RMRD_line == CLEAR_LINE)
	{
		if (offset >= 0x3800 && offset < 0x3808)
			return K051937_r(offset - 0x3800);
		else if (offset < 0x3c00)
			return K052109_r(offset);
		else
			return K051960_r(offset - 0x3c00);
	}
	else return K052109_r(offset);
}

WRITE_HANDLER( K052109_051960_w )
{
	if (offset >= 0x3800 && offset < 0x3808)
		K051937_w(offset - 0x3800,data);
	else if (offset < 0x3c00)
		K052109_w(offset,data);
	else
		K051960_w(offset - 0x3c00,data);
}





/***************************************************************************/
/*                                                                         */
/*                                 053245                                  */
/*                                                                         */
/***************************************************************************/



static int K053245_memory_region=2;
static struct GfxElement *K053245_gfx;
static void (*K053245_callback)(int *code,int *color,int *priority);
static int K053244_rombank;
static int K053245_ramsize;
static data16_t *K053245_ram, *K053245_buffer;
static data8_t K053244_regs[0x10];

int K053245_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int *code,int *color,int *priority))
{
	int gfx_index,i;
	static struct GfxLayout spritelayout =
	{
		16,16,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 0, 1, 2, 3, 4, 5, 6, 7,
				8*32+0, 8*32+1, 8*32+2, 8*32+3, 8*32+4, 8*32+5, 8*32+6, 8*32+7 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
				16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
		128*8
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	spritelayout.total = memory_region_length(gfx_memory_region) / 128;
	spritelayout.planeoffset[0] = plane3 * 8;
	spritelayout.planeoffset[1] = plane2 * 8;
	spritelayout.planeoffset[2] = plane1 * 8;
	spritelayout.planeoffset[3] = plane0 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&spritelayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

if (!(Machine->drv->video_attributes & VIDEO_HAS_SHADOWS))
	usrintf_showmessage("driver should use VIDEO_HAS_SHADOWS");

	/* prepare shadow draw table */
	gfx_drawmode_table[0] = DRAWMODE_NONE;
	for (i = 1;i < 15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	gfx_drawmode_table[15] = DRAWMODE_SHADOW;

	K053245_memory_region = gfx_memory_region;
	K053245_gfx = Machine->gfx[gfx_index];
	K053245_callback = callback;
	K053244_rombank = 0;
	K053245_ramsize = 0x800;
	K053245_ram = auto_malloc(K053245_ramsize);
	if (!K053245_ram) return 1;

	K053245_buffer = auto_malloc(K053245_ramsize);
	if (!K053245_buffer)
		return 1;

	memset(K053245_ram,0,K053245_ramsize);
	memset(K053245_buffer,0,K053245_ramsize);

	return 0;
}

READ16_HANDLER( K053245_word_r )
{
	return K053245_ram[offset];
}

WRITE16_HANDLER( K053245_word_w )
{
	COMBINE_DATA(K053245_ram+offset);
}

READ_HANDLER( K053245_r )
{
	if(offset & 1)
		return K053245_ram[offset>>1] & 0xff;
	else
		return (K053245_ram[offset>>1]>>8) & 0xff;
}

WRITE_HANDLER( K053245_w )
{
	if(offset & 1)
		K053245_ram[offset>>1] = (K053245_ram[offset>>1] & 0xff00) | data;
	else
		K053245_ram[offset>>1] = (K053245_ram[offset>>1] & 0x00ff) | (data<<8);
}

INLINE void K053245_update_buffer( void )
{
	memcpy(K053245_buffer, K053245_ram, K053245_ramsize);
}

READ_HANDLER( K053244_r )
{
	if ((K053244_regs[5] & 0x10) && offset >= 0x0c && offset < 0x10)
	{
		int addr;

		addr = (K053244_rombank << 19) | ((K053244_regs[11] & 0x7) << 18)
			| (K053244_regs[8] << 10) | (K053244_regs[9] << 2)
			| ((offset & 3) ^ 1);
		addr &= memory_region_length(K053245_memory_region)-1;

#if 0
	usrintf_showmessage("%04x: offset %02x addr %06x",activecpu_get_pc(),offset&3,addr);
#endif

		return memory_region(K053245_memory_region)[addr];
	}
	else if (offset == 0x06)
	{
		K053245_update_buffer();
		return 0;
	}
	else
	{
logerror("%04x: read from unknown 053244 address %x\n",activecpu_get_pc(),offset);
		return 0;
	}
}

WRITE_HANDLER( K053244_w )
{
	K053244_regs[offset] = data;

	switch(offset) {
	case 0x05: {
#ifdef MAME_DEBUG
		if (data & 0xc8)
			usrintf_showmessage("053244 reg 05 = %02x",data);
#endif
		/* bit 2 = unknown, Parodius uses it */
		/* bit 5 = unknown, Rollergames uses it */
#if VERBOSE
		logerror("%04x: write %02x to 053244 address 5\n",activecpu_get_pc(),data);
#endif
		break;
	}
	case 0x06:
		K053245_update_buffer();
		break;
	}
}

READ16_HANDLER( K053244_lsb_r )
{
	return K053244_r(offset);
}

WRITE16_HANDLER( K053244_lsb_w )
{
	if (ACCESSING_LSB)
		K053244_w(offset, data & 0xff);
}

READ16_HANDLER( K053244_word_r )
{
	return (K053244_r(offset*2)<<8)|K053244_r(offset*2+1);
}

WRITE16_HANDLER( K053244_word_w )
{
	if (ACCESSING_MSB)
		K053244_w(offset*2, (data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K053244_w(offset*2+1, data & 0xff);
}

void K053244_bankselect(int bank)
{
	K053244_rombank = bank;
}

/*
 * Sprite Format
 * ------------------
 *
 * Word | Bit(s)           | Use
 * -----+-fedcba9876543210-+----------------
 *   0  | x--------------- | active (show this sprite)
 *   0  | -x-------------- | maintain aspect ratio (when set, zoom y acts on both axis)
 *   0  | --x------------- | flip y
 *   0  | ---x------------ | flip x
 *   0  | ----xxxx-------- | sprite size (see below)
 *   0  | ---------xxxxxxx | priority order
 *   1  | --xxxxxxxxxxxxxx | sprite code. We use an additional bit in TMNT2, but this is
 *                           probably not accurate (protection related so we can't verify)
 *   2  | ------xxxxxxxxxx | y position
 *   3  | ------xxxxxxxxxx | x position
 *   4  | xxxxxxxxxxxxxxxx | zoom y (0x40 = normal, <0x40 = enlarge, >0x40 = reduce)
 *   5  | xxxxxxxxxxxxxxxx | zoom x (0x40 = normal, <0x40 = enlarge, >0x40 = reduce)
 *   6  | ------x--------- | mirror y (top half is drawn as mirror image of the bottom)
 *   6  | -------x-------- | mirror x (right half is drawn as mirror image of the left)
 *   6  | --------x------- | shadow
 *   6  | ---------xxxxxxx | "color", but depends on external connections
 *   7  | ---------------- |
 *
 * shadow enables transparent shadows. Note that it applies to pen 0x0f ONLY.
 * The rest of the sprite remains normal.
 */

void K053245_sprites_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
#define NUM_SPRITES 128
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];
	int flipscreenX, flipscreenY, spriteoffsX, spriteoffsY;

	flipscreenX = K053244_regs[5] & 0x01;
	flipscreenY = K053244_regs[5] & 0x02;
	spriteoffsX = (K053244_regs[0] << 8) | K053244_regs[1];
	spriteoffsY = (K053244_regs[2] << 8) | K053244_regs[3];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < K053245_ramsize / 2;offs += 8)
	{
		if (K053245_buffer[offs] & 0x8000)
		{
			sortedlist[K053245_buffer[offs] & 0x007f] = offs;
		}
	}

	for (pri_code = NUM_SPRITES-1;pri_code >= 0;pri_code--)
	{
		int ox,oy,color,code,size,w,h,x,y,flipx,flipy,mirrorx,mirrory,shadow,zoomx,zoomy,pri;


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		/* the following changes the sprite draw order from
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63

			to

			 0  1  2  3  4  5  6  7
			 8  9 10 11 12 13 14 15
			16 17 18 19 20 21 22 23
			24 25 26 27 28 29 30 31
			32 33 34 35 36 37 38 39
			40 41 42 43 44 45 46 47
			48 49 50 51 52 53 54 55
			56 57 58 59 60 61 62 63
		*/

		/* NOTE: from the schematics, it looks like the top 2 bits should be ignored */
		/* (there are not output pins for them), and probably taken from the "color" */
		/* field to do bank switching. However this applies only to TMNT2, with its */
		/* protection mcu creating the sprite table, so we don't know where to fetch */
		/* the bits from. */
		code = K053245_buffer[offs+1];
		code = ((code & 0xffe1) + ((code & 0x0010) >> 2) + ((code & 0x0008) << 1)
				 + ((code & 0x0004) >> 1) + ((code & 0x0002) << 2));
		color = K053245_buffer[offs+6] & 0x00ff;
		pri = 0;

		(*K053245_callback)(&code,&color,&pri);

		size = (K053245_buffer[offs] & 0x0f00) >> 8;

		w = 1 << (size & 0x03);
		h = 1 << ((size >> 2) & 0x03);

		/* zoom control:
		   0x40 = normal scale
		  <0x40 enlarge (0x20 = double size)
		  >0x40 reduce (0x80 = half size)
		*/
		zoomy = K053245_buffer[offs+4];
		if (zoomy > 0x2000) continue;
		if (zoomy) zoomy = (0x400000+zoomy/2) / zoomy;
		else zoomy = 2 * 0x400000;
		if ((K053245_buffer[offs] & 0x4000) == 0)
		{
			zoomx = K053245_buffer[offs+5];
			if (zoomx > 0x2000) continue;
			if (zoomx) zoomx = (0x400000+zoomx/2) / zoomx;
//			else zoomx = 2 * 0x400000;
else zoomx = zoomy; /* workaround for TMNT2 */
		}
		else zoomx = zoomy;

		ox = K053245_buffer[offs+3] + spriteoffsX;
		oy = K053245_buffer[offs+2];

		flipx = K053245_buffer[offs] & 0x1000;
		flipy = K053245_buffer[offs] & 0x2000;
		mirrorx = K053245_buffer[offs+6] & 0x0100;
		mirrory = K053245_buffer[offs+6] & 0x0200;
		shadow = K053245_buffer[offs+6] & 0x0080;

		if (flipscreenX)
		{
			ox = 512 - ox;
			if (!mirrorx) flipx = !flipx;
		}
		if (flipscreenY)
		{
			oy = -oy;
			if (!mirrory) flipy = !flipy;
		}

		ox = (ox + 0x5d) & 0x3ff;
		if (ox >= 768) ox -= 1024;
		oy = (-(oy + spriteoffsY + 0x07)) & 0x3ff;
		if (oy >= 640) oy -= 1024;

		/* the coordinates given are for the *center* of the sprite */
		ox -= (zoomx * w) >> 13;
		oy -= (zoomy * h) >> 13;

		for (y = 0;y < h;y++)
		{
			int sx,sy,zw,zh;

			sy = oy + ((zoomy * y + (1<<11)) >> 12);
			zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

			for (x = 0;x < w;x++)
			{
				int c,fx,fy;

				sx = ox + ((zoomx * x + (1<<11)) >> 12);
				zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
				c = code;
				if (mirrorx)
				{
					if ((flipx == 0) ^ (2*x < w))
					{
						/* mirror left/right */
						c += (w-x-1);
						fx = 1;
					}
					else
					{
						c += x;
						fx = 0;
					}
				}
				else
				{
					if (flipx) c += w-1-x;
					else c += x;
					fx = flipx;
				}
				if (mirrory)
				{
					if ((flipy == 0) ^ (2*y >= h))
					{
						/* mirror top/bottom */
						c += 8*(h-y-1);
						fy = 1;
					}
					else
					{
						c += 8*y;
						fy = 0;
					}
				}
				else
				{
					if (flipy) c += 8*(h-1-y);
					else c += 8*y;
					fy = flipy;
				}

				/* the sprite can start at any point in the 8x8 grid, but it must stay */
				/* in a 64 entries window, wrapping around at the edges. The animation */
				/* at the end of the saloon level in Sunset Riders breaks otherwise. */
				c = (c & 0x3f) | (code & ~0x3f);

				if (zoomx == 0x10000 && zoomy == 0x10000)
				{
					pdrawgfx(bitmap,K053245_gfx,
							c,
							color,
							fx,fy,
							sx,sy,
							cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,pri);
				}
				else
				{
					pdrawgfxzoom(bitmap,K053245_gfx,
							c,
							color,
							fx,fy,
							sx,sy,
							cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16,pri);
				}
			}
		}
	}
#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(K053245_buffer, 0x800, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
#undef NUM_SPRITES
}




/***************************************************************************/
/*                                                                         */
/*                                 053247                                  */
/*                                                                         */
/***************************************************************************/



static int K053247_memory_region, K053247_dx, K053247_dy;
static unsigned char K053246_regs[8];
static data16_t *K053247_ram;
static struct GfxElement *K053247_gfx;
static void (*K053247_callback)(int *code,int *color,int *priority);
static int K053246_OBJCHA_line;


int K053247_vh_start(int gfx_memory_region, int dx, int dy, int plane0,int plane1,int plane2,int plane3,
					 void (*callback)(int *code,int *color,int *priority))
{
	int gfx_index,i;
	static struct GfxLayout spritelayout =
	{
		16,16,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
				10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
		{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
				8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
		128*8
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	spritelayout.total = memory_region_length(gfx_memory_region) / 128;
	spritelayout.planeoffset[0] = plane0;
	spritelayout.planeoffset[1] = plane1;
	spritelayout.planeoffset[2] = plane2;
	spritelayout.planeoffset[3] = plane3;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&spritelayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

if (!(Machine->drv->video_attributes & VIDEO_HAS_SHADOWS))
	usrintf_showmessage("driver should use VIDEO_HAS_SHADOWS");

	/* prepare shadow draw table */
	gfx_drawmode_table[0] = DRAWMODE_NONE;
	for (i = 1;i < 15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	gfx_drawmode_table[15] = DRAWMODE_SHADOW;

	K053247_dx = dx;
	K053247_dy = dy;
	K053247_memory_region = gfx_memory_region;
	K053247_gfx = Machine->gfx[gfx_index];
	K053247_callback = callback;
	K053246_OBJCHA_line = CLEAR_LINE;
	K053247_ram = auto_malloc(0x1000);
	if (!K053247_ram) return 1;

	memset(K053247_ram,  0, 0x1000);
	memset(K053246_regs, 0, 8);

	state_save_register_UINT16("K053246", 0, "memory",    K053247_ram,  0x800);
	state_save_register_UINT8 ("K053246", 0, "registers", K053246_regs, 8);
	state_save_register_int   ("K053246", 0, "objcha",    &K053246_OBJCHA_line);

	return 0;
}

/* K055673 used with the 54246 in PreGX/Run and Gun/System GX games */
int K055673_vh_start(int gfx_memory_region, int alt_layout, int dx, int dy, void (*callback)(int *code,int *color,int *priority))
{
	int gfx_index;
	static struct GfxLayout spritelayout =	/* System GX sprite layout */
	{
		16,16,
		32768,				/* filled in later */
		5,
		{ 32, 24, 16, 8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47 },
		{ 0, 10*8, 10*8*2, 10*8*3, 10*8*4, 10*8*5, 10*8*6, 10*8*7, 10*8*8, 
		  10*8*9, 10*8*10, 10*8*11, 10*8*12, 10*8*13, 10*8*14, 10*8*15 },
		16*16*5
	};
	static struct GfxLayout spritelayout2 =	/* Run and Gun sprite layout */
	{
		16,16,
		32768,				/* filled in later */
		4,
		{ 24, 8, 16, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 32, 33, 34, 35, 36, 37, 38, 39 },
		{ 0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960 },
		16*16*4
	};
	unsigned char *s1, *s2, *d;
	long i;
	data16_t *K055673_rom;

	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	if (!alt_layout)
	{
		/* set the # of tiles based on the 4bpp section */
		spritelayout.total = (4*1024*1024) / 128;

		K055673_rom = (data16_t *)auto_malloc(20*1024*1024);
		d = (data8_t *)K055673_rom;

		// now combine the graphics together to form 5bpp
		s1 = memory_region(gfx_memory_region); // 4bpp area
		s2 = s1 + (4*1024*1024);	 // 1bpp area

		for (i = 0; i < 4*1024*1024; i+= 4)
		{
			*d++ = *s1++;
			*d++ = *s1++;
			*d++ = *s1++;
			*d++ = *s1++;
			*d++ = *s2++;
		}

		/* decode the graphics */
		Machine->gfx[gfx_index] = decodegfx((data8_t *)K055673_rom, &spritelayout);
	}
	else
	{
		K055673_rom = (data16_t *)memory_region(gfx_memory_region);
		spritelayout2.total = memory_region_length(gfx_memory_region) / (16*16/2);

		/* decode the graphics */
		Machine->gfx[gfx_index] = decodegfx((data8_t *)K055673_rom, &spritelayout2);
	}

	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

if (!(Machine->drv->video_attributes & VIDEO_HAS_SHADOWS))
	usrintf_showmessage("driver should use VIDEO_HAS_SHADOWS");

	/* prepare shadow draw table */
	gfx_drawmode_table[0] = DRAWMODE_NONE;
	for (i = 1;i < 15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	gfx_drawmode_table[15] = DRAWMODE_SHADOW;

	K053247_dx = dx;
	K053247_dy = dy;
	K053247_memory_region = gfx_memory_region;
	K053247_gfx = Machine->gfx[gfx_index];
	K053247_callback = callback;
	K053246_OBJCHA_line = CLEAR_LINE;
	K053247_ram = auto_malloc(0x1000);
	if (!K053247_ram) return 1;

	memset(K053247_ram,  0, 0x1000);
	memset(K053246_regs, 0, 8);

	state_save_register_UINT16("K053246", 0, "memory",    K053247_ram,  0x800);
	state_save_register_UINT8 ("K053246", 0, "registers", K053246_regs, 8);
	state_save_register_int   ("K053246", 0, "objcha",    &K053246_OBJCHA_line);

	return 0;
}

READ16_HANDLER( K053247_word_r )
{
	return K053247_ram[offset];
}

WRITE16_HANDLER( K053247_word_w )
{
	COMBINE_DATA(K053247_ram + offset);
}

READ_HANDLER( K053247_r )
{
	if(offset & 1)
		return K053247_ram[offset>>1] & 0xff;
	else
		return (K053247_ram[offset>>1]>>8) & 0xff;
}

WRITE_HANDLER( K053247_w )
{
	if(offset & 1)
		K053247_ram[offset>>1] = (K053247_ram[offset>>1] & 0xff00) | data;
	else
		K053247_ram[offset>>1] = (K053247_ram[offset>>1] & 0x00ff) | (data<<8);
}

READ_HANDLER( K053246_r )
{
	if (K053246_OBJCHA_line == ASSERT_LINE)
	{
		int addr;


		addr = (K053246_regs[6] << 17) | (K053246_regs[7] << 9) | (K053246_regs[4] << 1) | ((offset & 1) ^ 1);
		addr &= memory_region_length(K053247_memory_region)-1;

#if 0
	usrintf_showmessage("%04x: offset %02x addr %06x",activecpu_get_pc(),offset,addr);
#endif

		return memory_region(K053247_memory_region)[addr];
	}
	else
	{
logerror("%04x: read from unknown 053246 address %x\n",activecpu_get_pc(),offset);
		return 0;
	}
}

WRITE_HANDLER( K053246_w )
{
	K053246_regs[offset] = data;
#ifdef MAME_DEBUG
	if (offset == 5 && (data & 0xc8))
		usrintf_showmessage("053246 reg 05 = %02x",data);
#endif
}

READ16_HANDLER( K053246_word_r )
{
	return K053246_r(offset*2 + 1) | (K053246_r(offset*2) << 8);
}

WRITE16_HANDLER( K053246_word_w )
{
	if (ACCESSING_MSB)
		K053246_w(offset*2,(data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K053246_w(offset*2 + 1,data & 0xff);
}

void K053246_set_OBJCHA_line(int state)
{
	K053246_OBJCHA_line = state;
}

int K053246_is_IRQ_enabled(void)
{
	return K053246_regs[5] & 0x10;
}

/*
 * Sprite Format
 * ------------------
 *
 * Word | Bit(s)           | Use
 * -----+-fedcba9876543210-+----------------
 *   0  | x--------------- | active (show this sprite)
 *   0  | -x-------------- | maintain aspect ratio (when set, zoom y acts on both axis)
 *   0  | --x------------- | flip y
 *   0  | ---x------------ | flip x
 *   0  | ----xxxx-------- | sprite size (see below)
 *   0  | --------xxxxxxxx | priority order
 *   1  | xxxxxxxxxxxxxxxx | sprite code
 *   2  | ------xxxxxxxxxx | y position
 *   3  | ------xxxxxxxxxx | x position
 *   4  | xxxxxxxxxxxxxxxx | zoom y (0x40 = normal, <0x40 = enlarge, >0x40 = reduce)
 *   5  | xxxxxxxxxxxxxxxx | zoom x (0x40 = normal, <0x40 = enlarge, >0x40 = reduce)
 *   6  | x--------------- | mirror y (top half is drawn as mirror image of the bottom)
 *   6  | -x-------------- | mirror x (right half is drawn as mirror image of the left)
 *   6  | -----x---------- | shadow
 *   6  | xxxxxxxxxxxxxxxx | "color", but depends on external connections
 *   7  | ---------------- |
 *
 * shadow enables transparent shadows. Note that it applies to pen 0x0f ONLY.
 * The rest of the sprite remains normal.
 */

void K053247_sprites_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
#define NUM_SPRITES 256
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	int flipscreenx = K053246_regs[5] & 0x01;
	int flipscreeny = K053246_regs[5] & 0x02;
	INT16 offx = (K053246_regs[0] << 8) | K053246_regs[1];
	INT16 offy = (K053246_regs[2] << 8) | K053246_regs[3];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

#if 0
	{
		static int count=0;
		if(++count == 5) {
			int show = 0;
			count = 0;
			if (keyboard_pressed(KEYCODE_I)) {
				K053247_dy--;
				show = 1;
			} else if (keyboard_pressed(KEYCODE_M)) {
				K053247_dy++;
				show = 1;
			}
			if (keyboard_pressed(KEYCODE_J)) {
				K053247_dx--;
				show = 1;
			} else if (keyboard_pressed(KEYCODE_K)) {
				K053247_dx++;
				show = 1;
			}
			if (keyboard_pressed(KEYCODE_O))
				show = 1;
			if(show)
				usrintf_showmessage("dx %d dy %d", K053247_dx, K053247_dy);
		}
	}
#endif
	/* prebuild a sorted table */
	for (offs = 0;offs < 0x800;offs += 8)
	{
//		if (K053247_ram[offs] & 0x8000)
		sortedlist[K053247_ram[offs] & 0x00ff] = offs;
	}

	for (pri_code = 0;pri_code < NUM_SPRITES;pri_code++)
	{
		int ox,oy,color,code,size,w,h,x,y,xa,ya,flipx,flipy,mirrorx,mirrory,shadow,zoomx,zoomy,pri;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		if ((K053247_ram[offs] & 0x8000) == 0) continue;

		code = K053247_ram[offs+1];
		color = K053247_ram[offs+6];
		pri = 0;

		(*K053247_callback)(&code,&color,&pri);

		size = (K053247_ram[offs] & 0x0f00) >> 8;

		w = 1 << (size & 0x03);
		h = 1 << ((size >> 2) & 0x03);

		/* the sprite can start at any point in the 8x8 grid. We have to */
		/* adjust the offsets to draw it correctly. Simpsons does this all the time. */
		xa = 0;
		ya = 0;
		if (code & 0x01) xa += 1;
		if (code & 0x02) ya += 1;
		if (code & 0x04) xa += 2;
		if (code & 0x08) ya += 2;
		if (code & 0x10) xa += 4;
		if (code & 0x20) ya += 4;
		code &= ~0x3f;


		/* zoom control:
		   0x40 = normal scale
		  <0x40 enlarge (0x20 = double size)
		  >0x40 reduce (0x80 = half size)
		*/
		zoomy = K053247_ram[offs+4];
		if (zoomy > 0x2000) continue;
		if (zoomy) zoomy = (0x400000+zoomy/2) / zoomy;
		else zoomy = 2 * 0x400000;
		if ((K053247_ram[offs] & 0x4000) == 0)
		{
			zoomx = K053247_ram[offs+5];
			if (zoomx > 0x2000) continue;
			if (zoomx) zoomx = (0x400000+zoomx/2) / zoomx;
			else zoomx = 2 * 0x400000;
		}
		else zoomx = zoomy;

		ox = K053247_ram[offs+3];
		oy = K053247_ram[offs+2];

		flipx = K053247_ram[offs] & 0x1000;
		flipy = K053247_ram[offs] & 0x2000;
		mirrorx = K053247_ram[offs+6] & 0x4000;
		mirrory = K053247_ram[offs+6] & 0x8000;
		shadow = K053247_ram[offs+6] & 0x0400;

		if (flipscreenx)
		{
			ox = -ox;
			if (!mirrorx) flipx = !flipx;
		}
		if (flipscreeny)
		{
			oy = -oy;
			if (!mirrory) flipy = !flipy;
		}

#if 0	// fixes overdriv, but breaks everything else
		ox = (K053247_dx + ox - offx) & 0xfff;
		if (ox >= 0x800) ox -= 0x1000;
		oy = (-(K053247_dy + oy + offy)) & 0xfff;
		if (oy >= 0x800) oy -= 0x1000;
#else
		ox = (K053247_dx + ox - offx) & 0x3ff;
		if (ox >= 0x300) ox -= 0x400;
		oy = (-(K053247_dy + oy + offy)) & 0x3ff;
		if (oy >= 0x280) oy -= 0x400;
#endif

		/* the coordinates given are for the *center* of the sprite */
		ox -= (zoomx * w) >> 13;
		oy -= (zoomy * h) >> 13;

		for (y = 0;y < h;y++)
		{
			int sx,sy,zw,zh;

			sy = oy + ((zoomy * y + (1<<11)) >> 12);
			zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

			for (x = 0;x < w;x++)
			{
				int c,fx,fy;

				sx = ox + ((zoomx * x + (1<<11)) >> 12);
				zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
				c = code;
				if (mirrorx)
				{
					if ((flipx == 0) ^ (2*x < w))
					{
						/* mirror left/right */
						c += xoffset[(w-1-x+xa)&7];
						fx = 1;
					}
					else
					{
						c += xoffset[(x+xa)&7];
						fx = 0;
					}
				}
				else
				{
					if (flipx) c += xoffset[(w-1-x+xa)&7];
					else c += xoffset[(x+xa)&7];
					fx = flipx;
				}
				if (mirrory)
				{
					if ((flipy == 0) ^ (2*y >= h))
					{
						/* mirror top/bottom */
						c += yoffset[(h-1-y+ya)&7];
						fy = 1;
					}
					else
					{
						c += yoffset[(y+ya)&7];
						fy = 0;
					}
				}
				else
				{
					if (flipy) c += yoffset[(h-1-y+ya)&7];
					else c += yoffset[(y+ya)&7];
					fy = flipy;
				}

				if (zoomx == 0x10000 && zoomy == 0x10000)
				{
					pdrawgfx(bitmap,K053247_gfx,
							c,
							color,
							fx,fy,
							sx,sy,
							cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,pri);
				}
				else
				{
					pdrawgfxzoom(bitmap,K053247_gfx,
							c,
							color,
							fx,fy,
							sx,sy,
							cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16,pri);
				}

				if (mirrory && h == 1)  /* Simpsons shadows */
				{
					if (zoomx == 0x10000 && zoomy == 0x10000)
					{
						pdrawgfx(bitmap,K053247_gfx,
								c,
								color,
								fx,!fy,
								sx,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,pri);
					}
					else
					{
						pdrawgfxzoom(bitmap,K053247_gfx,
								c,
								color,
								fx,!fy,
								sx,sy,
								cliprect,shadow ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN,0,
								(zw << 16) / 16,(zh << 16) / 16,pri);
					}
				}
			}
		}
	}
#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(K053247_ram, 0x1000, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
#undef NUM_SPRITES
}





/***************************************************************************/
/*                                                                         */
/*                                 051316                                  */
/*                                                                         */
/***************************************************************************/



#define MAX_K051316 3

static int K051316_memory_region[MAX_K051316];
static int K051316_gfxnum[MAX_K051316];
static int K051316_wraparound[MAX_K051316];
static int K051316_offset[MAX_K051316][2];
static int K051316_bpp[MAX_K051316];
static void (*K051316_callback[MAX_K051316])(int *code,int *color);
static unsigned char *K051316_ram[MAX_K051316];
static unsigned char K051316_ctrlram[MAX_K051316][16];
static struct tilemap *K051316_tilemap[MAX_K051316];

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void K051316_get_tile_info(int tile_index,int chip)
{
	int code = K051316_ram[chip][tile_index];
	int color = K051316_ram[chip][tile_index + 0x400];

	tile_info.flags = 0;

	(*K051316_callback[chip])(&code,&color);

	SET_TILE_INFO(
			K051316_gfxnum[chip],
			code,
			color,
			tile_info.flags)
}

static void K051316_get_tile_info0(int tile_index) { K051316_get_tile_info(tile_index,0); }
static void K051316_get_tile_info1(int tile_index) { K051316_get_tile_info(tile_index,1); }
static void K051316_get_tile_info2(int tile_index) { K051316_get_tile_info(tile_index,2); }


int K051316_vh_start(int chip, int gfx_memory_region,int bpp,
		int tilemap_type,int transparent_pen,
		void (*callback)(int *code,int *color))
{
	int gfx_index;
	static void (*get_tile_info[3])(int tile_index) = { K051316_get_tile_info0,K051316_get_tile_info1,K051316_get_tile_info2 };

	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	if (bpp == 4)
	{
		static struct GfxLayout charlayout =
		{
			16,16,
			0,				/* filled in later */
			4,
			{ 0, 1, 2, 3 },
			{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
					8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
			{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
					8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
			128*8
		};


		/* tweak the structure for the number of tiles we have */
		charlayout.total = memory_region_length(gfx_memory_region) / 128;

		/* decode the graphics */
		Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&charlayout);
	}
	else if (bpp == 7 || bpp == 8)
	{
		static struct GfxLayout charlayout =
		{
			16,16,
			0,				/* filled in later */
			0,				/* filled in later */
			{ 0 },			/* filled in later */
			{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
					8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
			{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
					8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
			256*8
		};
		int i;


		/* tweak the structure for the number of tiles we have */
		charlayout.total = memory_region_length(gfx_memory_region) / 256;
		charlayout.planes = bpp;
		if (bpp == 7) for (i = 0;i < 7;i++) charlayout.planeoffset[i] = i+1;
		else for (i = 0;i < 8;i++) charlayout.planeoffset[i] = i;

		/* decode the graphics */
		Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region),&charlayout);
	}
	else
	{
logerror("K051316_vh_start supports only 4, 7 and 8 bpp\n");
		return 1;
	}

	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / (1 << bpp);
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / (1 << bpp);
	}

	K051316_memory_region[chip] = gfx_memory_region;
	K051316_gfxnum[chip] = gfx_index;
	K051316_bpp[chip] = bpp;
	K051316_callback[chip] = callback;

	K051316_tilemap[chip] = tilemap_create(get_tile_info[chip],tilemap_scan_rows,tilemap_type,16,16,32,32);

	K051316_ram[chip] = auto_malloc(0x800);

	if (!K051316_ram[chip] || !K051316_tilemap[chip])
		return 1;

	tilemap_set_transparent_pen(K051316_tilemap[chip],transparent_pen);

	K051316_wraparound[chip] = 0;	/* default = no wraparound */
	K051316_offset[chip][0] = K051316_offset[chip][1] = 0;

	return 0;
}

int K051316_vh_start_0(int gfx_memory_region,int bpp,
		int tilemap_type,int transparent_pen,
		void (*callback)(int *code,int *color))
{
	return K051316_vh_start(0,gfx_memory_region,bpp,tilemap_type,transparent_pen,callback);
}

int K051316_vh_start_1(int gfx_memory_region,int bpp,
		int tilemap_type,int transparent_pen,
		void (*callback)(int *code,int *color))
{
	return K051316_vh_start(1,gfx_memory_region,bpp,tilemap_type,transparent_pen,callback);
}

int K051316_vh_start_2(int gfx_memory_region,int bpp,
		int tilemap_type,int transparent_pen,
		void (*callback)(int *code,int *color))
{
	return K051316_vh_start(2,gfx_memory_region,bpp,tilemap_type,transparent_pen,callback);
}


int K051316_r(int chip, int offset)
{
	return K051316_ram[chip][offset];
}

READ_HANDLER( K051316_0_r )
{
	return K051316_r(0, offset);
}

READ_HANDLER( K051316_1_r )
{
	return K051316_r(1, offset);
}

READ_HANDLER( K051316_2_r )
{
	return K051316_r(2, offset);
}


void K051316_w(int chip,int offset,int data)
{
	if (K051316_ram[chip][offset] != data)
	{
		K051316_ram[chip][offset] = data;
		tilemap_mark_tile_dirty(K051316_tilemap[chip],offset & 0x3ff);
	}
}

WRITE_HANDLER( K051316_0_w )
{
	K051316_w(0,offset,data);
}

WRITE_HANDLER( K051316_1_w )
{
	K051316_w(1,offset,data);
}

WRITE_HANDLER( K051316_2_w )
{
	K051316_w(2,offset,data);
}


int K051316_rom_r(int chip, int offset)
{
	if ((K051316_ctrlram[chip][0x0e] & 0x01) == 0)
	{
		int addr;

		addr = offset + (K051316_ctrlram[chip][0x0c] << 11) + (K051316_ctrlram[chip][0x0d] << 19);
		if (K051316_bpp[chip] <= 4) addr /= 2;
		addr &= memory_region_length(K051316_memory_region[chip])-1;

#if 0
	usrintf_showmessage("%04x: offset %04x addr %04x",activecpu_get_pc(),offset,addr);
#endif

		return memory_region(K051316_memory_region[chip])[addr];
	}
	else
	{
logerror("%04x: read 051316 ROM offset %04x but reg 0x0c bit 0 not clear\n",activecpu_get_pc(),offset);
		return 0;
	}
}

READ_HANDLER( K051316_rom_0_r )
{
	return K051316_rom_r(0,offset);
}

READ_HANDLER( K051316_rom_1_r )
{
	return K051316_rom_r(1,offset);
}

READ_HANDLER( K051316_rom_2_r )
{
	return K051316_rom_r(2,offset);
}



void K051316_ctrl_w(int chip,int offset,int data)
{
	K051316_ctrlram[chip][offset] = data;
//if (offset >= 0x0c) logerror("%04x: write %02x to 051316 reg %x\n",activecpu_get_pc(),data,offset);
}

WRITE_HANDLER( K051316_ctrl_0_w )
{
	K051316_ctrl_w(0,offset,data);
}

WRITE_HANDLER( K051316_ctrl_1_w )
{
	K051316_ctrl_w(1,offset,data);
}

WRITE_HANDLER( K051316_ctrl_2_w )
{
	K051316_ctrl_w(2,offset,data);
}

void K051316_wraparound_enable(int chip, int status)
{
	K051316_wraparound[chip] = status;
}

void K051316_set_offset(int chip, int xoffs, int yoffs)
{
	K051316_offset[chip][0] = xoffs;
	K051316_offset[chip][1] = yoffs;
}


void K051316_zoom_draw(int chip, struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,UINT32 priority)
{
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;

	startx = 256 * ((INT16)(256 * K051316_ctrlram[chip][0x00] + K051316_ctrlram[chip][0x01]));
	incxx  =        (INT16)(256 * K051316_ctrlram[chip][0x02] + K051316_ctrlram[chip][0x03]);
	incyx  =        (INT16)(256 * K051316_ctrlram[chip][0x04] + K051316_ctrlram[chip][0x05]);
	starty = 256 * ((INT16)(256 * K051316_ctrlram[chip][0x06] + K051316_ctrlram[chip][0x07]));
	incxy  =        (INT16)(256 * K051316_ctrlram[chip][0x08] + K051316_ctrlram[chip][0x09]);
	incyy  =        (INT16)(256 * K051316_ctrlram[chip][0x0a] + K051316_ctrlram[chip][0x0b]);

	startx -= (16 + K051316_offset[chip][1]) * incyx;
	starty -= (16 + K051316_offset[chip][1]) * incyy;

	startx -= (89 + K051316_offset[chip][0]) * incxx;
	starty -= (89 + K051316_offset[chip][0]) * incxy;

	tilemap_draw_roz(bitmap,cliprect,K051316_tilemap[chip],startx << 5,starty << 5,
			incxx << 5,incxy << 5,incyx << 5,incyy << 5,
			K051316_wraparound[chip],
			flags,priority);

#if 0
	usrintf_showmessage("%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x",
			K051316_ctrlram[chip][0x00],
			K051316_ctrlram[chip][0x01],
			K051316_ctrlram[chip][0x02],
			K051316_ctrlram[chip][0x03],
			K051316_ctrlram[chip][0x04],
			K051316_ctrlram[chip][0x05],
			K051316_ctrlram[chip][0x06],
			K051316_ctrlram[chip][0x07],
			K051316_ctrlram[chip][0x08],
			K051316_ctrlram[chip][0x09],
			K051316_ctrlram[chip][0x0a],
			K051316_ctrlram[chip][0x0b],
			K051316_ctrlram[chip][0x0c],	/* bank for ROM testing */
			K051316_ctrlram[chip][0x0d],
			K051316_ctrlram[chip][0x0e],	/* 0 = test ROMs */
			K051316_ctrlram[chip][0x0f]);
#endif
}

void K051316_zoom_draw_0(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,UINT32 priority)
{
	K051316_zoom_draw(0,bitmap,cliprect,flags,priority);
}

void K051316_zoom_draw_1(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,UINT32 priority)
{
	K051316_zoom_draw(1,bitmap,cliprect,flags,priority);
}

void K051316_zoom_draw_2(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,UINT32 priority)
{
	K051316_zoom_draw(2,bitmap,cliprect,flags,priority);
}




/***************************************************************************/
/*                                                                         */
/*                                 053936                                  */
/*                                                                         */
/***************************************************************************/


#define K053936_MAX_CHIPS 2

data16_t *K053936_0_ctrl,*K053936_0_linectrl;
data16_t *K053936_1_ctrl,*K053936_1_linectrl;
static int K053936_offset[K053936_MAX_CHIPS][2];
static int K053936_wraparound[K053936_MAX_CHIPS];


static void K053936_zoom_draw(int chip,data16_t *ctrl,data16_t *linectrl,struct mame_bitmap *bitmap,const struct rectangle *cliprect,struct tilemap *tilemap,int flags,UINT32 priority)
{
	if (ctrl[0x07] & 0x0040)	/* "super" mode */
	{
		UINT32 startx,starty;
		int incxx,incxy;
		struct rectangle my_clip;
		int y,maxy;

		if (ctrl[0x07] & 0x0002)	/* not sure but fixes glfgreat */
		{
			my_clip.min_x = ctrl[0x08] + K053936_offset[chip][0]+2;
			my_clip.max_x = ctrl[0x09] + K053936_offset[chip][0]+2 - 1;
			if (my_clip.min_x < cliprect->min_x)
				my_clip.min_x = cliprect->min_x;
			if (my_clip.max_x > cliprect->max_x)
				my_clip.max_x = cliprect->max_x;

			y = ctrl[0x0a] + K053936_offset[chip][1]-2;
			if (y < cliprect->min_y)
				y = cliprect->min_y;
			maxy = ctrl[0x0b] + K053936_offset[chip][1]-2 - 1;
			if (maxy > cliprect->max_y)
				maxy = cliprect->max_y;
		}
		else
		{
			my_clip.min_x = cliprect->min_x;
			my_clip.max_x = cliprect->max_x;

			y = cliprect->min_y;
			maxy = cliprect->max_y;
		}

if (!keyboard_pressed(KEYCODE_Z))
		while (y <= maxy)
		{
			my_clip.min_y = my_clip.max_y = y;

			startx = 256 * (INT16)(linectrl[4*((y - K053936_offset[chip][1]) & 0x1ff) + 0] + ctrl[0x00]);
			starty = 256 * (INT16)(linectrl[4*((y - K053936_offset[chip][1]) & 0x1ff) + 1] + ctrl[0x01]);
			incxx  =       (INT16)(linectrl[4*((y - K053936_offset[chip][1]) & 0x1ff) + 2]);
			incxy  =       (INT16)(linectrl[4*((y - K053936_offset[chip][1]) & 0x1ff) + 3]);

			if (ctrl[0x06] & 0x8000) incxx *= 256;
			if (ctrl[0x06] & 0x0080) incxy *= 256;

			startx -= K053936_offset[chip][0] * incxx;
			starty -= K053936_offset[chip][0] * incxy;

			tilemap_draw_roz(bitmap,&my_clip,tilemap,startx << 5,starty << 5,
					incxx << 5,incxy << 5,0,0,
					K053936_wraparound[chip],
					flags,priority);

			y++;
		}
	}
	else	/* "simple" mode */
	{
		UINT32 startx,starty;
		int incxx,incxy,incyx,incyy;

		startx = 256 * (INT16)(ctrl[0x00]);
		starty = 256 * (INT16)(ctrl[0x01]);
		incyx  =       (INT16)(ctrl[0x02]);
		incxx  =       (INT16)(ctrl[0x03]);
		incyy  =       (INT16)(ctrl[0x04]);
		incxy  =       (INT16)(ctrl[0x05]);

		if (ctrl[0x06] & 0x4000) { incyx *= 256; incxx *= 256; }
		if (ctrl[0x06] & 0x0040) { incyy *= 256; incxy *= 256; }

		startx -= K053936_offset[chip][1] * incyx;
		starty -= K053936_offset[chip][1] * incyy;

		startx -= K053936_offset[chip][0] * incxx;
		starty -= K053936_offset[chip][0] * incxy;

		tilemap_draw_roz(bitmap,cliprect,tilemap,startx << 5,starty << 5,
				incxx << 5,incxy << 5,incyx << 5,incyy << 5,
				K053936_wraparound[chip],
				flags,priority);
	}

#if 0
if (keyboard_pressed(KEYCODE_D))
	usrintf_showmessage("%04x %04x %04x %04x\n%04x %04x %04x %04x\n%04x %04x %04x %04x\n%04x %04x %04x %04x",
			ctrl[0x00],
			ctrl[0x01],
			ctrl[0x02],
			ctrl[0x03],
			ctrl[0x04],
			ctrl[0x05],
			ctrl[0x06],
			ctrl[0x07],
			ctrl[0x08],
			ctrl[0x09],
			ctrl[0x0a],
			ctrl[0x0b],
			ctrl[0x0c],
			ctrl[0x0d],
			ctrl[0x0e],
			ctrl[0x0f]);
#endif
}


void K053936_0_zoom_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,struct tilemap *tilemap,int flags,UINT32 priority)
{
	K053936_zoom_draw(0,K053936_0_ctrl,K053936_0_linectrl,bitmap,cliprect,tilemap,flags,priority);
}

void K053936_1_zoom_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,struct tilemap *tilemap,int flags,UINT32 priority)
{
	K053936_zoom_draw(1,K053936_1_ctrl,K053936_1_linectrl,bitmap,cliprect,tilemap,flags,priority);
}


void K053936_wraparound_enable(int chip, int status)
{
	K053936_wraparound[chip] = status;
}


void K053936_set_offset(int chip, int xoffs, int yoffs)
{
	K053936_offset[chip][0] = xoffs;
	K053936_offset[chip][1] = yoffs;
}





/***************************************************************************/
/*                                                                         */
/*                                 053251                                  */
/*                                                                         */
/***************************************************************************/


static unsigned char K053251_ram[16];
static int K053251_palette_index[5];
static struct tilemap *K053251_tilemaps[5];
static int K053251_tilemaps_set;

static void K053251_reset_indexes(void)
{
	K053251_palette_index[0] = 32 * ((K053251_ram[9] >> 0) & 0x03);
	K053251_palette_index[1] = 32 * ((K053251_ram[9] >> 2) & 0x03);
	K053251_palette_index[2] = 32 * ((K053251_ram[9] >> 4) & 0x03);
	K053251_palette_index[3] = 16 * ((K053251_ram[10] >> 0) & 0x07);
	K053251_palette_index[4] = 16 * ((K053251_ram[10] >> 3) & 0x07);
}

int K053251_vh_start(void)
{
	K053251_set_tilemaps(NULL,NULL,NULL,NULL,NULL);

	state_save_register_UINT8("K053251", 0, "registers", K053251_ram, 16);
	state_save_register_func_postload(K053251_reset_indexes);

	return 0;
}

void K053251_set_tilemaps(struct tilemap *ci0,struct tilemap *ci1,struct tilemap *ci2,struct tilemap *ci3,struct tilemap *ci4)
{
	K053251_tilemaps[0] = ci0;
	K053251_tilemaps[1] = ci1;
	K053251_tilemaps[2] = ci2;
	K053251_tilemaps[3] = ci3;
	K053251_tilemaps[4] = ci4;

	K053251_tilemaps_set = (ci0 || ci1 || ci2 || ci3 || ci4) ? 1 : 0;
}

WRITE_HANDLER( K053251_w )
{
	int i,newind;

	data &= 0x3f;

	if (K053251_ram[offset] != data)
	{
		K053251_ram[offset] = data;
		if (offset == 9)
		{
			/* palette base index */
			for (i = 0;i < 3;i++)
			{
				newind = 32 * ((data >> 2*i) & 0x03);
				if (K053251_palette_index[i] != newind)
				{
					K053251_palette_index[i] = newind;
					if (K053251_tilemaps[i])
						tilemap_mark_all_tiles_dirty(K053251_tilemaps[i]);

				}
			}

			if (!K053251_tilemaps_set)
				tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}
		else if (offset == 10)
		{
			/* palette base index */
			for (i = 0;i < 2;i++)
			{
				newind = 16 * ((data >> 3*i) & 0x07);
				if (K053251_palette_index[3+i] != newind)
				{
					K053251_palette_index[3+i] = newind;
					if (K053251_tilemaps[3+i])
						tilemap_mark_all_tiles_dirty(K053251_tilemaps[3+i]);

				}
			}

			if (!K053251_tilemaps_set)
				tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}
	}
}

WRITE16_HANDLER( K053251_lsb_w )
{
	if (ACCESSING_LSB)
		K053251_w(offset, data & 0xff);
}

WRITE16_HANDLER( K053251_msb_w )
{
	if (ACCESSING_MSB)
		K053251_w(offset, (data >> 8) & 0xff);
}

int K053251_get_priority(int ci)
{
	return K053251_ram[ci];
}

int K053251_get_palette_index(int ci)
{
	return K053251_palette_index[ci];
}




/***************************************************************************/
/*                                                                         */
/*                                 054000                                  */
/*                                                                         */
/***************************************************************************/


static unsigned char K054000_ram[0x20];

static WRITE_HANDLER( collision_w )
{
}

WRITE_HANDLER( K054000_w )
{
#if VERBOSE
logerror("%04x: write %02x to 054000 address %02x\n",activecpu_get_pc(),data,offset);
#endif

	K054000_ram[offset] = data;
}

READ_HANDLER( K054000_r )
{
	int Acx,Acy,Aax,Aay;
	int Bcx,Bcy,Bax,Bay;


#if VERBOSE
logerror("%04x: read 054000 address %02x\n",activecpu_get_pc(),offset);
#endif

	if (offset != 0x18) return 0;


	Acx = (K054000_ram[0x01] << 16) | (K054000_ram[0x02] << 8) | K054000_ram[0x03];
	Acy = (K054000_ram[0x09] << 16) | (K054000_ram[0x0a] << 8) | K054000_ram[0x0b];
/* TODO: this is a hack to make thndrx2 pass the startup check. It is certainly wrong. */
if (K054000_ram[0x04] == 0xff) Acx+=3;
if (K054000_ram[0x0c] == 0xff) Acy+=3;
	Aax = K054000_ram[0x06] + 1;
	Aay = K054000_ram[0x07] + 1;

	Bcx = (K054000_ram[0x15] << 16) | (K054000_ram[0x16] << 8) | K054000_ram[0x17];
	Bcy = (K054000_ram[0x11] << 16) | (K054000_ram[0x12] << 8) | K054000_ram[0x13];
	Bax = K054000_ram[0x0e] + 1;
	Bay = K054000_ram[0x0f] + 1;

	if (Acx + Aax < Bcx - Bax)
		return 1;

	if (Bcx + Bax < Acx - Aax)
		return 1;

	if (Acy + Aay < Bcy - Bay)
		return 1;

	if (Bcy + Bay < Acy - Aay)
		return 1;

	return 0;
}

READ16_HANDLER( K054000_lsb_r )
{
	return K054000_r(offset);
}

WRITE16_HANDLER( K054000_lsb_w )
{
	if (ACCESSING_LSB)
		K054000_w(offset, data & 0xff);
}




/***************************************************************************/
/*                                                                         */
/*                                 051733                                  */
/*                                                                         */
/***************************************************************************/



static unsigned char K051733_ram[0x20];

WRITE_HANDLER( K051733_w )
{
#if VERBOSE
logerror("%04x: write %02x to 051733 address %02x\n",activecpu_get_pc(),data,offset);
#endif

	K051733_ram[offset] = data;
}


static int int_sqrt(UINT32 op)
{
	UINT32 i,step;

	i = 0x8000;
	step = 0x4000;
	while (step)
	{
		if (i*i == op) return i;
		else if (i*i > op) i -= step;
		else i += step;
		step >>= 1;
	}
	return i;
}

READ_HANDLER( K051733_r )
{
	int op1 = (K051733_ram[0x00] << 8) | K051733_ram[0x01];
	int op2 = (K051733_ram[0x02] << 8) | K051733_ram[0x03];
	int op3 = (K051733_ram[0x04] << 8) | K051733_ram[0x05];

	int rad = (K051733_ram[0x06] << 8) | K051733_ram[0x07];
	int yobj1c = (K051733_ram[0x08] << 8) | K051733_ram[0x09];
	int xobj1c = (K051733_ram[0x0a] << 8) | K051733_ram[0x0b];
	int yobj2c = (K051733_ram[0x0c] << 8) | K051733_ram[0x0d];
	int xobj2c = (K051733_ram[0x0e] << 8) | K051733_ram[0x0f];

#if VERBOSE
logerror("%04x: read 051733 address %02x\n",activecpu_get_pc(),offset);
#endif

	switch(offset){
		case 0x00:
			if (op2) return	(op1 / op2) >> 8;
			else return 0xff;
		case 0x01:
			if (op2) return	(op1 / op2) & 0xff;
			else return 0xff;

		/* this is completely unverified */
		case 0x02:
			if (op2) return	(op1 % op2) >> 8;
			else return 0xff;
		case 0x03:
			if (op2) return	(op1 % op2) & 0xff;
			else return 0xff;

		case 0x04:
			return int_sqrt(op3<<16) >> 8;

		case 0x05:
			return int_sqrt(op3<<16) & 0xff;

		case 0x07:{
			if (xobj1c + rad < xobj2c)
				return 0x80;

			if (xobj2c + rad < xobj1c)
				return 0x80;

			if (yobj1c + rad < yobj2c)
				return 0x80;

			if (yobj2c + rad < yobj1c)
				return 0x80;

			return 0;
		}
		default:
			return K051733_ram[offset];
	}
}





/***************************************************************************/
/*                                                                         */
/*                                 054157                                  */
/*                                                                         */
/***************************************************************************/



static struct tilemap *K054157_tilemap[4], *K054157_cur_tilemap;
static struct tilemap *K054157_tilemapb[4], *K054157_tilemaps[4];

static data16_t K054157_regs[0x20], K054157_regsb[4];
static void (*K054157_linescroll_updater[4])(int layer);

static int K054157_cur_rombank, K054157_romnbbanks;
static int K054157_uses_tile_banks, K054157_cur_tile_bank;
static int K054157_gfxnum, K054157_memory_region;
static int K054157_cur_offset;
static data16_t *K054157_rambase, *K054157_cur_spbase, *K054157_cur_rambase;
static data8_t *K054157_rombase;
static data16_t *K054157_rambasel[8];
static int K054157_tilemapl[8], K054157_offsetl[8];

static void (*K054157_callback)(int, int *, int *);

INLINE void K054157_get_tile_info(int tile_index,int layer)
{
	data16_t *addr;
	int attr, code;
	data16_t *lbase = K054157_rambase + 0x2000*layer;
	if(tile_index < 64*32)
		addr = lbase + (tile_index<<1);
	else
		addr = lbase + (tile_index<<1) + 0x1000 - 64*32*2;

	attr = addr[0];
	code = addr[1];
	tile_info.flags = 0;

	(*K054157_callback)(layer, &code, &attr);
	SET_TILE_INFO(K054157_gfxnum,
			code,
			attr,
			tile_info.flags)
}

static void K054157_get_tile_info0(int tile_index) { K054157_get_tile_info(tile_index,0); }
static void K054157_get_tile_info1(int tile_index) { K054157_get_tile_info(tile_index,1); }
static void K054157_get_tile_info2(int tile_index) { K054157_get_tile_info(tile_index,2); }
static void K054157_get_tile_info3(int tile_index) { K054157_get_tile_info(tile_index,3); }


static void K054157_lsu_1_256(int layer)
{
	int y;
	int basey = K054157_regs[0x10|layer];

	data16_t *baseram = K054157_cur_spbase + layer*0x400;
	for(y=0; y<256; y++) {
		int offset = (((basey + y) & 0x1ff) << 1) | 1;
		tilemap_set_scrollx(K054157_tilemap[layer], y, baseram[offset]);
	}
}

static void K054157_lsu_1_512(int layer)
{
	int y;
	int basey = K054157_regs[0x10|layer];

	data16_t *baseram = K054157_cur_spbase + layer*0x400;
	for(y=0; y<512; y++) {
		int offset = (((basey + y) & 0x1ff) << 1) | 1;
		tilemap_set_scrollx(K054157_tilemap[layer], y, baseram[offset]);
	}
}

static void K054157_lsu_8_256(int layer)
{
	int y;
	int basey = K054157_regs[0x10|layer];

	data16_t *baseram = K054157_cur_spbase + layer*0x400;
	for(y=0; y<256; y++) {
		int offset = (((basey + y) & 0x1f8) << 1) | 1;
		tilemap_set_scrollx(K054157_tilemap[layer], y, baseram[offset]);
	}
}

static void K054157_lsu_8_512(int layer)
{
	int y;
	int basey = K054157_regs[0x10|layer];

	data16_t *baseram = K054157_cur_spbase + layer*0x400;
	for(y=0; y<512; y++) {
		int offset = (((basey + y) & 0x1f8) << 1) | 1;
		tilemap_set_scrollx(K054157_tilemap[layer], y, baseram[offset]);
	}
}

static void K054157_lsu_full(int layer)
{
	tilemap_set_scrollx(K054157_tilemap[layer], 0, K054157_regs[0x14|layer]);
}

static void K054157_reset_linescroll(void)
{
	int layer;
	int mode = K054157_regs[5];
	for(layer=0; layer < 4; layer++) {
		int lmode = (mode >> (layer << 1)) & 3;
		switch(lmode) {
		case 0:
			if(K054157_tilemap[layer] == K054157_tilemaps[layer]) {
				K054157_linescroll_updater[layer] = K054157_lsu_1_256;
				tilemap_set_scroll_rows(K054157_tilemap[layer], 256);
			} else {
				K054157_linescroll_updater[layer] = K054157_lsu_1_512;
				tilemap_set_scroll_rows(K054157_tilemap[layer], 512);
			}
			break;
		case 1:
			logerror("LS mode layer %d unknown (%d)\n", layer, lmode);
			goto rhaaa_lovely;
		case 2:
			if(K054157_tilemap[layer] == K054157_tilemaps[layer]) {
				K054157_linescroll_updater[layer] = K054157_lsu_8_256;
				tilemap_set_scroll_rows(K054157_tilemap[layer], 256);
			} else {
				K054157_linescroll_updater[layer] = K054157_lsu_8_512;
				tilemap_set_scroll_rows(K054157_tilemap[layer], 512);
			}
			break;
		case 3:
		rhaaa_lovely:
			K054157_linescroll_updater[layer] = K054157_lsu_full;
			tilemap_set_scroll_rows(K054157_tilemap[layer], 1);
			break;
		}
	}
}

static void K054157_change_tilemap(int layer)
{
	int flip = 0;
	if(K054157_regs[8|layer] & 1) {
		tilemap_set_enable(K054157_tilemapb[layer], 1);
		tilemap_set_enable(K054157_tilemaps[layer], 0);
		K054157_tilemap[layer] = K054157_tilemapb[layer];
	} else {
		tilemap_set_enable(K054157_tilemapb[layer], 0);
		tilemap_set_enable(K054157_tilemaps[layer], 1);
		K054157_tilemap[layer] = K054157_tilemaps[layer];
	}
	tilemap_mark_all_tiles_dirty(K054157_tilemap[layer]);

	if(K054157_regs[0] & 0x20)
		flip |= TILEMAP_FLIPY;
	if(K054157_regs[0] & 0x10)
		flip |= TILEMAP_FLIPX;

	tilemap_set_flip(K054157_tilemap[layer], flip);

	K054157_reset_linescroll();
}

static void K054157_reset_tilemaps(void)
{
	int i;
	// To avoid crashes in K054157_reset_linescroll()
	for(i=0; i<4; i++)
		K054157_tilemap[i] = K054157_tilemaps[i];
	for(i=0; i<4; i++)
		K054157_change_tilemap(i);
}

static void K054157_change_flip(void)
{
	int flip = 0;

	if(K054157_regs[0] & 0x20)
		flip |= TILEMAP_FLIPY;
	if(K054157_regs[0] & 0x10)
		flip |= TILEMAP_FLIPX;
	tilemap_set_flip(K054157_tilemap[0], flip);
	tilemap_set_flip(K054157_tilemap[1], flip);
	tilemap_set_flip(K054157_tilemap[2], flip);
	tilemap_set_flip(K054157_tilemap[3], flip);
}

static void K054157_change_rambank(void)
{
	int bank = ((K054157_regs[0x19]>>2) & 6) | (K054157_regs[0x19] & 1);

	K054157_cur_rambase = K054157_rambasel[bank];
	K054157_cur_tilemap = K054157_tilemap[K054157_tilemapl[bank]];
	K054157_cur_offset  = K054157_offsetl[bank];
}

static void K054157_change_splayer(void)
{
	int bank = ((K054157_regs[0x18]>>2) & 6) | (K054157_regs[0x18] & 1);
	K054157_cur_spbase = K054157_rambasel[bank];
}

static void K054157_change_rombank(void)
{
	int bank;

	if (K054157_uses_tile_banks)	/* asterix */
		bank = (K054157_regs[0x1a] >> 8) | (K054157_regs[0x1b] << 4) | (K054157_cur_tile_bank << 6);
	else	/* everything else */
		bank = K054157_regs[0x1a] | (K054157_regs[0x1b] << 16);

	K054157_cur_rombank = bank % K054157_romnbbanks;
//usrintf_showmessage("%04x: %04x %04x %04x",activecpu_get_pc(),K054157_regs[0x1a],K054157_regs[0x1b],K054157_cur_rombank);
}

int K054157_vh_start(int gfx_memory_region, int big, int (*scrolld)[4][2], int plane0,int plane1,int plane2,int plane3, void (*callback)(int, int *, int *))
{
	int gfx_index;
	int i;
	static struct GfxLayout charlayout =
	{
		8, 8,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
		{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },
		8*8*4
	};

	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	charlayout.total = memory_region_length(gfx_memory_region) / (8*4);
	charlayout.planeoffset[0] = plane0;
	charlayout.planeoffset[1] = plane1;
	charlayout.planeoffset[2] = plane2;
	charlayout.planeoffset[3] = plane3;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(gfx_memory_region), &charlayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[gfx_index]->colortable = Machine->pens;
		Machine->gfx[gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	K054157_memory_region = gfx_memory_region;
	K054157_gfxnum = gfx_index;
	K054157_callback = callback;

	K054157_rombase = memory_region(gfx_memory_region);
	K054157_romnbbanks = memory_region_length(gfx_memory_region)/0x2000;
	K054157_cur_rombank = 0;
	K054157_uses_tile_banks = 0;

	K054157_tilemapb[0] = tilemap_create(K054157_get_tile_info0, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	K054157_tilemapb[1] = tilemap_create(K054157_get_tile_info1, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	K054157_tilemapb[2] = tilemap_create(K054157_get_tile_info2, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	K054157_tilemapb[3] = tilemap_create(K054157_get_tile_info3, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	K054157_tilemaps[0] = tilemap_create(K054157_get_tile_info0, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	K054157_tilemaps[1] = tilemap_create(K054157_get_tile_info1, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	K054157_tilemaps[2] = tilemap_create(K054157_get_tile_info2, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	K054157_tilemaps[3] = tilemap_create(K054157_get_tile_info3, tilemap_scan_rows,
										 TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	K054157_rambase = auto_malloc(0x14000);

	if(!K054157_rambase
	   || !K054157_tilemapb[0] || !K054157_tilemapb[1] || !K054157_tilemapb[2] || !K054157_tilemapb[3]
	   || !K054157_tilemaps[0] || !K054157_tilemaps[1] || !K054157_tilemaps[2] || !K054157_tilemaps[3])
		return 1;

	if(big) {
		K054157_rambasel[0] = K054157_rambase + 0x2000;
		K054157_rambasel[1] = K054157_rambase + 0x6000;
		K054157_rambasel[2] = K054157_rambase + 0x3000;
		K054157_rambasel[3] = K054157_rambase + 0x7000;
		K054157_rambasel[4] = K054157_rambase + 0x4000;
		K054157_rambasel[5] = K054157_rambase + 0x0000;
		K054157_rambasel[6] = K054157_rambase + 0x5000;
		K054157_rambasel[7] = K054157_rambase + 0x1000;
	} else {
		K054157_rambasel[0] = K054157_rambase + 0x0000;
		K054157_rambasel[1] = K054157_rambase + 0x2000;
		K054157_rambasel[2] = K054157_rambase + 0x4000;
		K054157_rambasel[3] = K054157_rambase + 0x6000;
		K054157_rambasel[4] = 0;
		K054157_rambasel[5] = 0;
		K054157_rambasel[6] = 0;
		K054157_rambasel[7] = 0;
	}

	for(i=0; i<8; i++) {
		if(K054157_rambasel[i]) {
			int delta = K054157_rambasel[i] - K054157_rambase;
			K054157_tilemapl[i] = delta >> 13;
			K054157_offsetl [i] = (delta & 0x1000) ? 64*32 : 0;
		} else {
			K054157_tilemapl[i] = 0;
			K054157_offsetl [i] = 0;
		}
	}

	memset(K054157_rambase, 0, 0x10000);
	memset(K054157_regs, 0, 0x40);
	memset(K054157_regsb, 0, 8);

	for(i=0; i<4; i++) {
		tilemap_set_transparent_pen(K054157_tilemapb[i],0);
		tilemap_set_scrolldx(K054157_tilemapb[i], -scrolld[0][i][0], -scrolld[1][i][0]);
		tilemap_set_scrolldy(K054157_tilemapb[i], -scrolld[0][i][1], -scrolld[1][i][1]);

		tilemap_set_transparent_pen(K054157_tilemaps[i],0);
		tilemap_set_scrolldx(K054157_tilemaps[i], -scrolld[0][i][0], -scrolld[1][i][0]);
		tilemap_set_scrolldy(K054157_tilemaps[i], -scrolld[0][i][1], -scrolld[1][i][1]);
	}

	K054157_reset_tilemaps();
	K054157_change_rambank();
	K054157_change_rombank();
	K054157_change_splayer();

	state_save_register_UINT16("K054157", 0, "memory",      K054157_rambase, 0x8000);
	state_save_register_UINT16("K054157", 0, "registers",   K054157_regs,    0x20);
	state_save_register_UINT16("K054157", 0, "registers b", K054157_regsb,   0x4);

	state_save_register_func_postload(K054157_reset_tilemaps);
	state_save_register_func_postload(K054157_change_rambank);
	state_save_register_func_postload(K054157_change_rombank);

	return 0;
}


READ16_HANDLER( K054157_ram_word_r )
{
	return K054157_cur_rambase[offset];
}

READ16_HANDLER( K054157_ram_half_word_r )
{
	return K054157_cur_rambase[((offset << 1) & 0xffe) | ((offset >> 11) ^ 1)];
}

READ16_HANDLER( K054157_rom_word_r )
{
	int addr = 0x2000*K054157_cur_rombank + 2*offset;

#if 0
	usrintf_showmessage("%04x: addr %06x",activecpu_get_pc(),addr);
#endif

	return K054157_rombase[addr+1] | (K054157_rombase[addr] << 8);
}

WRITE16_HANDLER( K054157_ram_word_w )
{
	data16_t *adr = K054157_cur_rambase + offset;
	data16_t old = *adr;

	COMBINE_DATA(adr);
	if(*adr != old && K054157_cur_tilemap)
		tilemap_mark_tile_dirty(K054157_cur_tilemap, offset/2 + K054157_cur_offset);
}

WRITE16_HANDLER( K054157_ram_half_word_w )
{
	data16_t *adr = K054157_cur_rambase + (((offset << 1) & 0xffe) | 1);
	data16_t old = *adr;

	COMBINE_DATA(adr);
	if(*adr != old)
		tilemap_mark_tile_dirty(K054157_cur_tilemap, (offset & 0x7ff) + K054157_cur_offset);
}

WRITE16_HANDLER( K054157_word_w )
{
	UINT16 old = K054157_regs[offset];
	COMBINE_DATA (K054157_regs + offset);

	if(K054157_regs[offset] != old)
	{
		switch(offset) {
		case 0x00:
			if((K054157_regs[0] & 0x30) != (old & 0x30))
				K054157_change_flip();
			break;
		case 0x05:
			K054157_reset_linescroll();
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
			if((K054157_regs[offset] & 1) ^ (K054157_tilemap[offset & 3] == K054157_tilemapb[offset & 3]))
				K054157_change_tilemap(offset & 3);
			break;
		case 0x18:
			K054157_change_splayer();
			break;
		case 0x19:
			K054157_change_rambank();
			break;
		case 0x1a:
		case 0x1b:
			K054157_change_rombank();
			break;
		case 0x1c:
			tilemap_mark_all_tiles_dirty(K054157_tilemapb[0]);
			tilemap_mark_all_tiles_dirty(K054157_tilemapb[1]);
			tilemap_mark_all_tiles_dirty(K054157_tilemapb[2]);
			tilemap_mark_all_tiles_dirty(K054157_tilemapb[3]);
			tilemap_mark_all_tiles_dirty(K054157_tilemaps[0]);
			tilemap_mark_all_tiles_dirty(K054157_tilemaps[1]);
			tilemap_mark_all_tiles_dirty(K054157_tilemaps[2]);
			tilemap_mark_all_tiles_dirty(K054157_tilemaps[3]);
			break;
		}
	}
}

WRITE16_HANDLER( K054157_b_word_w )
{
	COMBINE_DATA (K054157_regsb + offset);
}

void K054157_tilemap_update(void)
{
	int layer;

	for(layer=0; layer<4; layer++)
	{
		K054157_linescroll_updater[layer](layer);
		tilemap_set_scrolly(K054157_tilemap[layer], 0, K054157_regs[0x10|layer]);
	}
}

void K054157_tilemap_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int num, int flags, UINT32 priority)
{
	tilemap_draw(bitmap,cliprect, K054157_tilemap[num], flags, priority);
}

int K054157_is_IRQ_enabled(void)
{
	return K054157_regs[3] & 1;
}

int K054157_get_lookup(int bits)
{
	int res;

	res = (K054157_regs[0x1c] >> (bits << 2)) & 0x0f;

	if (K054157_uses_tile_banks)	/* Asterix */
		res |= K054157_cur_tile_bank << 4;

	return res;
}

void K054157_set_tile_bank(int bank)
{
	K054157_uses_tile_banks = 1;

	if (K054157_cur_tile_bank != bank)
	{
		K054157_cur_tile_bank = bank;

		tilemap_mark_all_tiles_dirty(K054157_tilemap[0]);
		tilemap_mark_all_tiles_dirty(K054157_tilemap[1]);
		tilemap_mark_all_tiles_dirty(K054157_tilemap[2]);
		tilemap_mark_all_tiles_dirty(K054157_tilemap[3]);
	}

	K054157_change_rombank();
}
