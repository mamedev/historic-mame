/******************************************************************************

  driver.c

  The list of all available drivers. Drivers have to be included here to be
  recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the DRIVER() macro. The first one
  declares external references to the drivers; the second one builds an array
  storing all the drivers.

******************************************************************************/

#include "driver.h"

#ifdef TINY_COMPILE
extern struct GameDriver TINY_NAME;

const struct GameDriver *drivers[] =
{
	&TINY_NAME,
	0	/* end of array */
};

#else

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern struct GameDriver NAME##_driver;
#include "driver.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &NAME##_driver,
const struct GameDriver *drivers[] =
{
#include "driver.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

#ifndef NEOMAME

	/* "Pacman hardware" games */
	DRIVER( pacman )	/* (c) 1980 Namco */
	DRIVER( pacmanjp )	/* (c) 1980 Namco */
	DRIVER( pacmanm )	/* (c) 1980 Midway */
	DRIVER( npacmod )	/* (c) 1981 Namco */
	DRIVER( pacmod )	/* (c) 1981 Midway */
	DRIVER( hangly )	/* hack */
	DRIVER( hangly2 )	/* hack */
	DRIVER( puckman )	/* hack */
	DRIVER( pacheart )	/* hack */
	DRIVER( piranha )	/* hack */
	DRIVER( pacplus )
	DRIVER( mspacman )	/* (c) 1981 Midway (but it's a bootleg) */	/* made by Gencomp */
	DRIVER( mspacatk )	/* hack */
	DRIVER( pacgal )	/* hack */
	DRIVER( maketrax )	/* (c) 1981 Williams, high score table says KRL (fur Kural) */
	DRIVER( crush )		/* (c) 1981 Kural Samno Electric Ltd */
	DRIVER( crush2 )	/* (c) 1981 Kural Esco Electric Ltd - bootleg? */
	DRIVER( crush3 )	/* Kural Electric Ltd - bootleg? */
	DRIVER( mbrush )	/* 1981 bootleg */
	DRIVER( eyes )		/* (c) 1982 Digitrex Techstar + "Rockola presents" */
	DRIVER( eyes2 )		/* (c) 1982 Techstar + "Rockola presents" */
	DRIVER( mrtnt )		/* (c) 1983 Telko */
	DRIVER( ponpoko )	/* (c) 1982 Sigma Ent. Inc. */
	DRIVER( ponpokov )	/* (c) 1982 Sigma Ent. Inc. + Venture Line license */
	DRIVER( lizwiz )	/* (c) 1985 Techstar + "Sunn presents" */
	DRIVER( theglob )	/* (c) 1983 Epos Corporation */
	DRIVER( beastf )	/* (c) 1984 Epos Corporation */
	DRIVER( jumpshot )
	DRIVER( vanvan )	/* (c) 1983 Karateco (bootleg?) */
	DRIVER( vanvanb )	/* bootleg */
	DRIVER( alibaba )	/* (c) 1982 Sega */
	DRIVER( pengo )		/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo2 )	/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo2u )	/* 834-0386 (c) 1982 Sega */
	DRIVER( penta )		/* bootleg */
	DRIVER( jrpacman )	/* (c) 1983 Midway */

	/* "Galaxian hardware" games */
	DRIVER( galaxian )	/* (c) Namco */
	DRIVER( galmidw )	/* (c) Midway */
	DRIVER( superg )	/* hack */
	DRIVER( galaxb )	/* bootleg */
	DRIVER( galapx )	/* hack */
	DRIVER( galap1 )	/* hack */
	DRIVER( galap4 )	/* hack */
	DRIVER( galturbo )	/* hack */
	DRIVER( pisces )	/* ? */
	DRIVER( uniwars )	/* (c) Irem */
	DRIVER( gteikoku )	/* (c) Irem */
	DRIVER( spacbatt )	/* bootleg */
	DRIVER( warofbug )	/* (c) 1981 Armenia */
	DRIVER( redufo )	/* ? */
	DRIVER( pacmanbl )	/* bootleg */
	DRIVER( devilfsg )	/* (c) 1984 Vision / Artic (bootleg?) */
	DRIVER( zigzag )	/* (c) 1982 LAX */
	DRIVER( zigzag2 )	/* (c) 1982 LAX */
	DRIVER( jumpbug )	/* (c) 1981 Rock-ola */
	DRIVER( jumpbugb )	/* (c) 1981 Sega */
	DRIVER( levers )	/* (c) 1983 Rock-ola */
	DRIVER( azurian )	/* (c) 1982 Rait Electronics Ltd */
	DRIVER( mooncrgx )	/* bootleg */
	DRIVER( mooncrst )	/* (c) 1980 Nichibutsu */
	DRIVER( mooncrsg )	/* (c) 1980 Gremlin */
	DRIVER( smooncrs )	/* Gremlin */
	DRIVER( mooncrsb )	/* bootleg */
	DRIVER( mooncrs2 )	/* bootleg */
	DRIVER( fantazia )	/* bootleg */
	DRIVER( eagle )		/* (c) Centuri */
	DRIVER( eagle2 )	/* (c) Centuri */
	DRIVER( moonqsr )	/* (c) 1980 Nichibutsu */
	DRIVER( checkman )	/* (c) 1982 Zilec-Zenitone */
	DRIVER( moonal2 )	/* Nichibutsu */
	DRIVER( moonal2b )	/* Nichibutsu */
	DRIVER( kingball )	/* (c) 1980 Namco */
	DRIVER( kingbalj )	/* (c) 1980 Namco */

	/* "Scramble hardware" (and variations) games */
	DRIVER( scramble )	/* GX387 (c) 1981 Konami */
	DRIVER( scrambls )	/* GX387 (c) 1981 Stern */
	DRIVER( scramblb )	/* bootleg */
	DRIVER( atlantis )	/* (c) 1981 Comsoft */
	DRIVER( atlants2 )	/* (c) 1981 Comsoft */
	DRIVER( theend )	/* (c) 1980 Stern */
	DRIVER( ckongs )	/* bootleg */
	DRIVER( froggers )	/* bootleg */
	DRIVER( amidars )	/* (c) 1982 Konami */
	DRIVER( triplep )	/* (c) 1982 KKI */
	DRIVER( mariner )	/* (c) 1981 Amenip */
	DRIVER( mars )		/* (c) 1981 Artic */
	DRIVER( devilfsh )	/* (c) 1982 Artic */
	DRIVER( newsin7 )	/* (c) 1983 ATW USA, Inc. */
	DRIVER( hotshock )	/* (c) 1982 E.G. Felaco */
	DRIVER( scobra )	/* GX316 (c) 1981 Konami */
	DRIVER( scobras )	/* GX316 (c) 1981 Stern */
	DRIVER( scobrab )	/* GX316 (c) 1981 Karateco (bootleg?) */
	DRIVER( stratgyx )	/* GX306 (c) 1981 Stern */
	DRIVER( stratgyb )	/* GX306 bootleg (of the Konami version?) */
	DRIVER( armorcar )	/* (c) 1981 Stern */
	DRIVER( armorca2 )	/* (c) 1981 Stern */
	DRIVER( moonwar2 )	/* (c) 1981 Stern */
	DRIVER( monwar2a )	/* (c) 1981 Stern */
	DRIVER( spdcoin )	/* (c) 1984 Stern */
	DRIVER( darkplnt )	/* (c) 1982 Stern */
	DRIVER( tazmania )	/* (c) 1982 Stern */
	DRIVER( tazmani2 )	/* (c) 1982 Stern */
	DRIVER( calipso )	/* (c) 1982 Tago */
	DRIVER( anteater )	/* (c) 1982 Tago */
	DRIVER( rescue )	/* (c) 1982 Stern */
	DRIVER( minefld )	/* (c) 1983 Stern */
	DRIVER( losttomb )	/* (c) 1982 Stern */
	DRIVER( losttmbh )	/* (c) 1982 Stern */
	DRIVER( superbon )	/* bootleg */
	DRIVER( hustler )	/* GX343 (c) 1981 Konami */
	DRIVER( billiard )	/* bootleg */
	DRIVER( hustlerb )	/* bootleg */
	DRIVER( frogger )	/* 834-0068 (c) 1981 Sega */
	DRIVER( frogsega )	/* (c) 1981 Sega */
	DRIVER( frogger2 )	/* 800-3110 (c) 1981 Sega */
	DRIVER( amidar )	/* GX337 (c) 1981 Konami */
	DRIVER( amidaru )	/* GX337 (c) 1982 Konami + Stern license */
	DRIVER( amidaro )	/* GX337 (c) 1982 Konami + Olympia license */
	DRIVER( amigo )		/* bootleg */
	DRIVER( turtles )	/* (c) 1981 Stern */
	DRIVER( turpin )	/* (c) 1981 Sega */
	DRIVER( k600 )		/* GX353 (c) 1981 Konami */
	DRIVER( flyboy )	/* (c) 1982 Kaneko (bootleg?) */
	DRIVER( fastfred )	/* (c) 1982 Atari */
	DRIVER( jumpcoas )	/* (c) 1983 Kaneko */

	/* "Crazy Climber hardware" games */
	DRIVER( cclimber )	/* (c) 1980 Nichibutsu */
	DRIVER( cclimbrj )	/* (c) 1980 Nichibutsu */
	DRIVER( ccboot )	/* bootleg */
	DRIVER( ccboot2 )	/* bootleg */
	DRIVER( ckong )		/* (c) 1981 Falcon */
	DRIVER( ckonga )	/* (c) 1981 Falcon */
	DRIVER( ckongjeu )	/* bootleg */
	DRIVER( ckongo )	/* bootleg */
	DRIVER( ckongalc )	/* bootleg */
	DRIVER( monkeyd )	/* bootleg */
	DRIVER( rpatrolb )	/* bootleg */
	DRIVER( silvland )	/* Falcon */
	DRIVER( yamato )	/* (c) 1983 Sega */
	DRIVER( yamato2 )	/* (c) 1983 Sega */
	DRIVER( swimmer )	/* (c) 1982 Tehkan */
	DRIVER( swimmera )	/* (c) 1982 Tehkan */
	DRIVER( guzzler )	/* (c) 1983 Tehkan */

	/* Nichibutsu games */
	DRIVER( friskyt )	/* (c) 1981 */
	DRIVER( radrad )	/* (c) 1982 Nichibutsu USA */
	DRIVER( seicross )	/* (c) 1984 + Alice */
	DRIVER( sectrzon )	/* (c) 1984 + Alice */
	DRIVER( wiping )	/* (c) 1982 */
	DRIVER( rugrats )	/* (c) 1983 */
	DRIVER( cop01 )		/* (c) 1985 */
	DRIVER( cop01a )	/* (c) 1985 */
	DRIVER( terracre )	/* (c) 1985 */
	DRIVER( terracrb )	/* (c) 1985 */
	DRIVER( terracra )	/* (c) 1985 */
	DRIVER( galivan )	/* (c) 1985 */
	DRIVER( galivan2 )	/* (c) 1985 */
	DRIVER( dangar )	/* (c) 1986 */
	DRIVER( dangar2 )	/* (c) 1986 */
	DRIVER( dangarb )	/* bootleg */
	DRIVER( terraf )	/* (c) 1987 */
	DRIVER( terrafu )	/* (c) 1987 Nichibutsu USA */
	DRIVER( armedf )	/* (c) 1988 */

	/* "Phoenix hardware" (and variations) games */
	DRIVER( phoenix )	/* (c) 1980 Amstar */
	DRIVER( phoenixt )	/* (c) 1980 Taito */
	DRIVER( phoenix3 )	/* bootleg */
	DRIVER( phoenixc )	/* bootleg */
	DRIVER( pleiads )	/* (c) 1981 Tehkan */
	DRIVER( pleiadbl )	/* bootleg */
	DRIVER( pleiadce )	/* (c) 1981 Centuri + Tehkan */
	DRIVER( naughtyb )	/* (c) 1982 Jaleco */
	DRIVER( naughtya )	/* bootleg */
	DRIVER( naughtyc )	/* (c) 1982 Jaleco + Cinematronics */
	DRIVER( popflame )	/* (c) 1982 Jaleco */
	DRIVER( popflama )	/* (c) 1982 Jaleco */

	/* Namco games */
	DRIVER( warpwarp )	/* (c) 1981 Namco - different hardware */
	DRIVER( warpwarr )	/* (c) 1981 Rock-ola - different hardware */
						/* the high score table says "NAMCO" */
	DRIVER( warpwar2 )	/* (c) 1981 Rock-ola - different hardware */
						/* the high score table says "NAMCO" */
	DRIVER( rallyx )	/* (c) 1980 Namco */
	DRIVER( rallyxm )	/* (c) 1980 Midway */
	DRIVER( nrallyx )	/* (c) 1981 Namco */
	DRIVER( jungler )	/* GX327 (c) 1981 Konami */
	DRIVER( junglers )	/* GX327 (c) 1981 Stern */
	DRIVER( locomotn )	/* GX359 (c) 1982 Konami + Centuri license */
	DRIVER( cottong )	/* bootleg */
	DRIVER( commsega )	/* (c) 1983 Sega */
	/* the following ones all have a custom I/O chip */
	DRIVER( bosco )		/* (c) 1981 */
	DRIVER( boscomd )	/* (c) 1981 Midway */
	DRIVER( boscomd2 )	/* (c) 1981 Midway */
	DRIVER( galaga )	/* (c) 1981 */
	DRIVER( galagamw )	/* (c) 1981 Midway */
	DRIVER( galagads )	/* hack */
	DRIVER( gallag )	/* bootleg */
	DRIVER( galagab2 )	/* bootleg */
	DRIVER( galaga84 )	/* hack */
	DRIVER( nebulbee )	/* hack */
	DRIVER( digdug )	/* (c) 1982 */
	DRIVER( digdugb )	/* (c) 1982 */
	DRIVER( digdugat )	/* (c) 1982 Atari */
	DRIVER( dzigzag )	/* bootleg */
	DRIVER( xevious )	/* (c) 1982 */
	DRIVER( xeviousa )	/* (c) 1982 + Atari license */
	DRIVER( xevios )	/* bootleg */
	DRIVER( sxevious )	/* (c) 1984 */
	DRIVER( superpac )	/* (c) 1982 */
	DRIVER( superpcm )	/* (c) 1982 Midway */
	DRIVER( pacnpal )	/* (c) 1983 */
	DRIVER( phozon )	/* (c) 1983 */
	DRIVER( mappy )		/* (c) 1983 */
	DRIVER( mappyjp )	/* (c) 1983 */
	DRIVER( digdug2 )	/* (c) 1985 */
	DRIVER( digdug2a )	/* (c) 1985 */
	DRIVER( todruaga )	/* (c) 1984 */
	DRIVER( todruagb )	/* (c) 1984 */
	DRIVER( motos )		/* (c) 1985 */
	DRIVER( grobda )	/* (c) 1984 */
	DRIVER( grobda2 )	/* (c) 1984 */
	DRIVER( grobda3 )	/* (c) 1984 */
	DRIVER( gaplus )	/* (c) 1984 */
	DRIVER( gaplusa )	/* (c) 1984 */
	DRIVER( galaga3 )	/* (c) 1984 */
	DRIVER( galaga3a )	/* (c) 1984 */
	/* no custom I/O in the following, HD63701 (or compatible) microcontroller instead */
	DRIVER( pacland )	/* (c) 1984 */
	DRIVER( pacland2 )	/* (c) 1984 */
	DRIVER( pacland3 )	/* (c) 1984 */
	DRIVER( paclandm )	/* (c) 1984 Midway */
//	DRIVER( skykid )

	/* Namco System 86 games */
	/* 85.4  Hopping MAPPY (sequel to MAPPY) */
	/* 86.5  Sky Kid DX (sequel to Sky Kid) */
	DRIVER( roishtar )	/* (c) 1986 */
	DRIVER( genpeitd )	/* (c) 1986 */
	DRIVER( rthunder )	/* (c) 1986 */
	DRIVER( rthundrb )	/* (c) 1986 */
	DRIVER( wndrmomo )	/* (c) 1987 */

	/* Namco System 1 games */
//	DRIVER( blazer )
//	DRIVER( ws90 )
//	DRIVER( dspirits )
//	DRIVER( splatter )
//	DRIVER( galaga88 )
//	DRIVER( pacmania )
//	DRIVER( alice )
//	DRIVER( shadowld )

	/* Namco System 2 games */
//	DRIVER( cosmogng )
//	DRIVER( assault )
//	DRIVER( phelios )
//	DRIVER( rthun2 )
//	DRIVER( metlhawk )

/*
>Namco games
>-----------------------------------------------------
>I will finish this mail by correcting Namco system board history
>in driver.c.
>
>-----------------------------------------------------
>Libble Rabble board
>83.12 Libble Rabble (The first Japanese game that uses M68K)
>86.4  Toy Pop
>
>-----------------------------------------------------
>PAC-LAND board
>84.8  PAC-LAND
>85.1  Dragon Buster
>
>-----------------------------------------------------
>Metro-Cross board
>85.5  Metro-Cross
>85.7  Baraduke
>
>-----------------------------------------------------
>System I Games
>87.4  [Youkai Douchuuki] (Shadow Land) - This game has no custom chip protection
>87.6  Dragon Spirit (OLD version)
>87.9  Blazer
>87.?? Dragon Spirit (NEW version)
>87.9  Quester
>87.11 PAC-MANIA
>87.12 Galaga'88
>88.3  Pro-[Yakyuu](baseball) World Stadium
>88.5  [CHOU-ZETU-RINN-JINN] [BERABOW]-MAN.
>88.7  Marchen Maze (Alice in Wonderland)
>88.8  [BAKU-TOTU-KIJYUU-TEI] (sequel to Baraduke)
>88.10 Pro Tennis World Court
>88.11 Splatter House
>88.12 Face Off
>89.2  Rompers
>89.3  Blast Off (sequel to Bosconian)
>89.1  Pro-[Yakyuu] World Stadium '89
>89.12 Dangerous Seed
>90.7  PRO-[YAKYUU] WORLD-STADIUM'90.
>90.10 Pistol[DAIMYOU NO BOUKEN] (sequel to Berabow-man)
>90.11 [SOUKO-BAN] DX
>91.12 Tank Force
>
>-----------------------------------------------------
>System II GAMES
>
>88.4  Assault
>88.9  Ordyne
>88.10 Metal Hawk (dual-system2)
>88.12 [MIRAI]-NINJA
>89.2  Phelios
>89.4  WALKURE [NO DEN-SETU](The legend of Valkyrie)
>89.6  Dirt Fox
>89.9  Finest Hour
>89.11 Burning Force
>90.2  Marvel Land
>90.5  [Kyuukai Douchuuki] (baseball game. uses character of Youkai
>Douchuuki)
>90.12 Dragon Saber
>91.3  Rolling Thunder 2
>91.3  Steel Gunner
>91.9  Super World Stadium
>92.3  Steel Gunner 2
>92.3  Cosmo Gangs the Video
>92.9  Super World Stadium '92 [GEKITOU-HEN]
>92.10 F/A


-----------------------------------------------------
NAMCO SYSTEM BOARD GAME HISTORY?(TABLE SIZE GAME FRAME)
-----------------------------------------------------

(NAMCO SYSTEM I)

87.4  [YOU-KAI-DOU-CHUU-KI]                      "YD"    NONE
87.6  DragonSpirit(OLD VERSION)                  "DS"    136
87.7  Blazer                                     "BZ"    144
87.?? DragonSpirit(NEW VERSION)                  "DS2"   136
87.9  Quester                                    "QS"    'on sub board(A)'
87.?? Quester(SPECIAL EDITION)                   "QS2"   'on sub board(A)'
87.11 PAC-MANIA                                  "PN"    151
??.?? PAC-MANIA(?)(it's rare in japan)           "PN2"   (151?)
87.12 Galaga'88                                  "G8"    153
88.3  '88 PRO-[YAKYUU] WORLD-STADIUM.            (WS?)   (154?)
88.5  [CHOU-ZETU-RINN-JINN] [BERABOW]-MAN.       "BM"    'on sub board(B)'
88.7  MELHEN MAZE(Alice in Wonderland)           "MM"    152
88.8  [BAKU-TOTU-KIJYUU-TEI]                     "BK"    155
88.10 pro-tennis WORLD COAT                      "WC"    143
88.11 splatter house                             "SH"    181
88.12 FAIS OFF(spell-mistake?)                   "FO"    'on sub board(C)'
89.2  ROMPERS                                    "RP"    182
89.3  BLAST-OFF                                  "BO"    183
89.1  PRO-[YAKYUU] WORLD-STADIUM'89.             "W9"    184
89.12 DANGERUS SEED                              "DR"    308
90.7  PRO-[YAKYUU] WORLD-STADIUM'90.             "W90"   310
90.10 [PISUTORU-DAIMYOU NO BOUKEN]               "PD"    309
90.11 [SOUKO-BAN] DX                             "SB"    311
91.12 TANK-FOURCE                                "TF"    185

-----------------------------------------------------

About system I sub board:
sub board used game always need sub board.
if sub bord nothing then not starting game.
because, key custum chip on sub board.
(diffalent type key custum chip on mother board.)

-----------------------------------------------------

'sub board(A)'= for sencer interface bord.
it having changed jp that used format taito/namco.

-----------------------------------------------------

'sub board(B)'= two toggle switch bord.
it sub board the inter face that 'two toggle switch'.
( == in japan called 'BERABOW-SWITCH'.)

<push switch side view>

  =~~~~~=       =~~~~~=
   |   |         |   |
   +---+         +---+
    | |           |||

NORMAL-SW     BERABOW-SW
(two-pins)    (tree-pins)
(GND?,sw1)     (GND?,sw1,sw2)

It abnormal switch was can feel player pushed power.
(It power in proportion to the push speed.)
It used the game controls, 1 stick & 2 it botton game.
Passaged over-time from sw1 triggerd to sw2 triggerd then
min panch/kick,and short time then max panch/kick feeled the game.

-----------------------------------------------------

'sub board(C)'= can 4 players sub board.
it subboard on 3 player & 4 player input lines.

-----------------------------------------------------
( into '[' to ']' mark is 'NI-HO-N-GO'.(japannese languege) )
-----------------------------------------------------
System 2 GAMES

88.4  ASSULT
88.9  ORDYNE(spell-mistake?)
88.10 METAL HAWK(dual-system2)
88.12 [MIRAI]-NINJA
89.2  Phelious(spell-mistake?)
89.4  WALKURE [NO DEN-SETU](the legend of WALKURE)
89.6  (??durt-fox)
89.9  FINEST HOUR
89.11 BURNING FORCE
90.2  MARBEL-LAND
90.5  (??KYUU-KAI-DOU-CHUU-KI)
90.12 DRAGON-SAVER
91.3  ROLLING-THUNDER2
91.3  (??STEEL-GUNNER)
91.9  (??SUPER-WORLD-STUDIUM)
92.3  (??STEEL-GUNNER2)
92.3  COSMO-GANGS THE VIDEO
92.9  (??SUPER-WORLD-STUDIUM'92 [GEKITOU-HEN])

*/

	/* Universal games */
	DRIVER( cosmicg )	/* (c) 1979 */
	DRIVER( cosmica )	/* (c) [1979] */
	DRIVER( panic )		/* (c) 1980 */
	DRIVER( panica )	/* (c) 1980 */
	DRIVER( panicger )	/* (c) 1980 */
	DRIVER( magspot2 )	/* (c) [1980?] */
	DRIVER( devzone )	/* (c) [1980?] */
	DRIVER( nomnlnd )	/* (c) [1980?] */
	DRIVER( nomnlndg )	/* (c) [1980?] + Gottlieb */
	DRIVER( cheekyms )	/* (c) [1980?] */
	DRIVER( ladybug )	/* (c) 1981 */
	DRIVER( ladybugb )	/* bootleg */
	DRIVER( snapjack )	/* (c) */
	DRIVER( cavenger )	/* (c) 1981 */
	DRIVER( mrdo )		/* (c) 1982 */
	DRIVER( mrdot )		/* (c) 1982 + Taito license */
	DRIVER( mrdofix )	/* (c) 1982 + Taito license */
	DRIVER( mrlo )		/* bootleg */
	DRIVER( mrdu )		/* bootleg */
	DRIVER( mrdoy )		/* bootleg */
	DRIVER( yankeedo )	/* bootleg */
	DRIVER( docastle )	/* (c) 1983 */
	DRIVER( docastl2 )	/* (c) 1983 */
	DRIVER( dounicorn )	/* (c) 1983 */
	DRIVER( dorunrun )	/* (c) 1984 */
	DRIVER( dorunru2 )	/* (c) 1984 */
	DRIVER( dorunruc )	/* (c) 1984 */
	DRIVER( spiero )	/* (c) 1987 */
	DRIVER( dowild )	/* (c) 1984 */
	DRIVER( jjack )		/* (c) 1984 */
	DRIVER( kickridr )	/* (c) 1984 */

	/* Nintendo games */
	DRIVER( radarscp )	/* (c) 1980 Nintendo */
	DRIVER( dkong )		/* (c) 1981 Nintendo of America */
	DRIVER( dkongjp )	/* (c) 1981 Nintendo */
	DRIVER( dkongjr )	/* (c) 1982 Nintendo of America */
	DRIVER( dkngjrjp )	/* no copyright notice */
	DRIVER( dkjrjp )	/* (c) 1982 Nintendo */
	DRIVER( dkjrbl )	/* (c) 1982 Nintendo of America */
	DRIVER( dkong3 )	/* (c) 1983 Nintendo of America */
	DRIVER( mario )		/* (c) 1983 Nintendo of America */
	DRIVER( masao )		/* bootleg */
//	DRIVER( hunchy )	/* hacked Donkey Kong board */
//	DRIVER( herocast )
	DRIVER( popeye )
	DRIVER( popeye2 )
	DRIVER( popeyebl )	/* bootleg */
	DRIVER( punchout )	/* (c) 1984 */
	DRIVER( spnchout )	/* (c) 1984 */
	DRIVER( armwrest )	/* (c) 1985 */

	/* Midway 8080 b/w games */
	DRIVER( seawolf )	/* 596 [1976] */
	DRIVER( gunfight )	/* 597 [1975] */
	/* 603 - Top Gun [1976] */
	DRIVER( tornbase )	/* 605 [1976] */
	DRIVER( zzzap )		/* 610 [1976] */
	DRIVER( maze )		/* 611 [1976] */
	DRIVER( boothill )	/* 612 [1977] */
	DRIVER( checkmat )	/* 615 [1977] */
	/* 618 - Road Runner [1977] */
	/* 618 - Desert Gun [1977] */
	DRIVER( dplay )		/* 619 [1977] */
	DRIVER( lagunar )	/* 622 [1977] */
	DRIVER( gmissile )	/* 623 [1977] */
	DRIVER( m4 )		/* 626 [1977] */
	DRIVER( clowns )	/* 630 [1978] */
	/* 640 - Space Walk [1978] */
	DRIVER( einnings )	/* 642 [1978] Midway */
	/* 643 - Shuffleboard [1978] */
	DRIVER( dogpatch )	/* 644 [1977] */
	DRIVER( spcenctr )	/* 645 (c) 1980 Midway */
	DRIVER( phantom2 )	/* 652 [1979] */
	DRIVER( midwbowl )	/* 730 [1978] Midway */
	DRIVER( invaders )	/* 739 [1979] */
	DRIVER( blueshrk )	/* 742 [1978] */
	/* 851 - Space Invaders II cocktail */
	DRIVER( invdelux )	/* 852 [1980] Midway */
	DRIVER( invadpt2 )	/* 852 [1980] Taito */
	/* 870 - Space Invaders Deluxe cocktail */
	DRIVER( earthinv )
	DRIVER( spaceatt )
	DRIVER( sinvemag )
	DRIVER( invrvnge )
	DRIVER( invrvnga )
	DRIVER( galxwars )
	DRIVER( lrescue )	/* (c) 1979 Taito */
	DRIVER( grescue )	/* bootleg? */
	DRIVER( desterth )	/* bootleg */
	DRIVER( cosmicmo )	/* Universal */
	DRIVER( rollingc )	/* Nichibutsu */
	DRIVER( bandido )	/* (c) Exidy */
	DRIVER( ozmawars )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( solfight )	/* bootleg */
	DRIVER( spaceph )	/* Zilec Games */
	DRIVER( schaser )	/* Taito */
	DRIVER( lupin3 )	/* (c) 1980 Taito */
	DRIVER( helifire )	/* (c) Nintendo */
	DRIVER( helifira )	/* (c) Nintendo */
	DRIVER( spacefev )
	DRIVER( sfeverbw )
	DRIVER( astlaser )
	DRIVER( intruder )
	DRIVER( polaris )	/* (c) 1980 Taito */
	DRIVER( polarisa )	/* (c) 1980 Taito */
	DRIVER( ballbomb )	/* (c) 1980 Taito */
	DRIVER( m79amb )
	DRIVER( alieninv2 )
	DRIVER( sitv )
	DRIVER( sicv )
	DRIVER( sisv )

	/* "Midway" Z80 b/w games */
	DRIVER( astinvad )	/* (c) 1980 Stern */
	DRIVER( kamikaze )	/* Leijac Corporation */
	DRIVER( spaceint )	/* [1980] Shoei */

	/* Meadows S2650 games */
	DRIVER( lazercmd )	/* [1976?] */
	DRIVER( deadeye )	/* [1978?] */
	DRIVER( gypsyjug )	/* [1978?] */
	DRIVER( medlanes )	/* [1977?] */

	/* Midway "Astrocade" games */
	DRIVER( wow )		/* (c) 1980 */
	DRIVER( robby )		/* (c) 1981 */
	DRIVER( gorf )		/* (c) 1981 */
	DRIVER( gorfpgm1 )	/* (c) 1981 */
	DRIVER( seawolf2 )
	DRIVER( spacezap )	/* (c) 1980 */
	DRIVER( ebases )

	/* Bally Midway MCR games */
	/* MCR1 */
	DRIVER( solarfox )	/* (c) 1981 */
	DRIVER( kick )		/* (c) 1981 */
	DRIVER( kicka )		/* bootleg? */
	/* MCR2 */
	DRIVER( shollow )	/* (c) 1981 */
	DRIVER( shollow2 )	/* (c) 1981 */
	DRIVER( tron )		/* (c) 1982 */
	DRIVER( tron2 )		/* (c) 1982 */
	DRIVER( kroozr )	/* (c) 1982 */
	DRIVER( domino )	/* (c) 1982 */
	DRIVER( wacko )		/* (c) 1982 */
	DRIVER( twotiger )	/* (c) 1984 */
	/* MCR2 + MCR3 sprites */
	DRIVER( journey )	/* (c) 1983 */
	/* MCR3 */
	DRIVER( tapper )	/* (c) 1983 */
	DRIVER( sutapper )	/* (c) 1983 */
	DRIVER( rbtapper )	/* (c) 1984 */
	DRIVER( dotron )	/* (c) 1983 */
	DRIVER( dotrone )	/* (c) 1983 */
	DRIVER( destderb )	/* (c) 1984 */
	DRIVER( timber )	/* (c) 1984 */
	DRIVER( spyhunt )	/* (c) 1983 */
	DRIVER( crater )	/* (c) 1984 */
	DRIVER( sarge )		/* (c) 1985 */
	DRIVER( rampage )	/* (c) 1986 */
	DRIVER( rampage2 )	/* (c) 1986 */
//	DRIVER( powerdrv )
	DRIVER( maxrpm )	/* (c) 1986 */
	/* MCR 68000 */
	DRIVER( xenophob )	/* (c) 1987 */
	/* Power Drive - similar to Xenophobe? */
/* other possible MCR games:
Black Belt
Shoot the Bull
Special Force
MotorDome
Six Flags (?)
*/

	/* Irem games */
	/* trivia: IREM means "International Rental Electronics Machines" */
	DRIVER( skychut )	/* (c) [1980] */
	DRIVER( mpatrol )	/* (c) 1982 */
	DRIVER( mpatrolw )	/* (c) 1982 + Williams license */
	DRIVER( mranger )	/* bootleg */
	DRIVER( troangel )	/* (c) 1983 */
	DRIVER( yard )		/* (c) 1983 */
	DRIVER( vsyard )	/* (c) 1983/1984 */
	DRIVER( vsyard2 )	/* (c) 1983/1984 */
	DRIVER( travrusa )	/* (c) 1983 */
	DRIVER( motorace )	/* (c) 1983 Williams license */
	/* M62 */
	DRIVER( kungfum )	/* (c) 1984 */
	DRIVER( kungfud )	/* (c) 1984 + Data East license */
	DRIVER( kungfub )	/* bootleg */
	DRIVER( kungfub2 )	/* bootleg */
	DRIVER( battroad )	/* (c) 1984 */
	DRIVER( ldrun )		/* (c) 1984 licensed from Broderbund */
	DRIVER( ldruna )	/* (c) 1984 licensed from Broderbund */
	DRIVER( ldrun2 )	/* (c) 1984 licensed from Broderbund */
	DRIVER( ldrun3 )	/* (c) 1985 licensed from Broderbund */
	DRIVER( ldrun4 )	/* (c) 1986 licensed from Broderbund */
	DRIVER( lotlot )	/* (c) 1985 licensed from Tokuma Shoten */
	DRIVER( kidniki )	/* (c) 1986 + Data East USA license */
	DRIVER( spelunkr )	/* (c) 1985 licensed from Broderbund */
	DRIVER( spelunk2 )	/* (c) 1986 licensed from Broderbund */

	DRIVER( vigilant )	/* (c) 1988 */
	DRIVER( vigilntu )	/* (c) 1988 */
	DRIVER( vigilntj )	/* (c) 1988 */
	/* M97 */
//	DRIVER( riskchal )
//	DRIVER( shisen2 )
//	DRIVER( quizf1 )

	/* Gottlieb/Mylstar games (Gottlieb became Mylstar in 1983) */
	DRIVER( reactor )	/* GV-100 (c) 1982 Gottlieb */
	DRIVER( mplanets )	/* GV-102 (c) 1983 Gottlieb */
	DRIVER( qbert )		/* GV-103 (c) 1982 Gottlieb */
	DRIVER( qbertjp )	/* GV-103 (c) 1982 Gottlieb + Konami license */
	DRIVER( sqbert )	/* (c) 1983 Mylstar - never released */
	DRIVER( krull )		/* GV-105 (c) 1983 Gottlieb */
	DRIVER( mach3 )		/* GV-109 (c) 1983 Mylstar */
	DRIVER( usvsthem )	/* GV-??? (c) 198? Mylstar */
	DRIVER( stooges )	/* GV-113 (c) 1984 Mylstar */
	DRIVER( qbertqub )	/* GV-119 (c) 1983 Mylstar */
	DRIVER( curvebal )	/* GV-134 (c) 1984 Mylstar */

	/* older Taito games */
	DRIVER( crbaloon )	/* (c) 1980 Taito Corporation */
	DRIVER( crbalon2 )	/* (c) 1980 Taito Corporation */

	/* Taito "Qix hardware" games */
	DRIVER( qix )		/* (c) 1981 Taito America Corporation */
	DRIVER( qixa )		/* (c) 1981 Taito America Corporation */
	DRIVER( qixb )		/* (c) 1981 Taito America Corporation */
	DRIVER( qix2 )		/* (c) 1981 Taito America Corporation */
	DRIVER( sdungeon )	/* (c) 1981 Taito America Corporation */
	DRIVER( elecyoyo )	/* (c) 1982 Taito America Corporation */
	DRIVER( elecyoy2 )	/* (c) 1982 Taito America Corporation */
	DRIVER( kram )		/* (c) 1982 Taito America Corporation */
	DRIVER( kram2 )		/* (c) 1982 Taito America Corporation */
	DRIVER( zookeep )	/* (c) 1982 Taito America Corporation */
	DRIVER( zookeep2 )	/* (c) 1982 Taito America Corporation */
	DRIVER( zookeep3 )	/* (c) 1982 Taito America Corporation */

	/* Taito SJ System games */
	DRIVER( spaceskr )	/* (c) 1981 Taito Corporation */
	DRIVER( junglek )	/* (c) 1982 Taito Corporation */
	DRIVER( jungleh )	/* (c) 1982 Taito America Corporation */
	DRIVER( alpine )	/* (c) 1982 Taito Corporation */
	DRIVER( alpinea )	/* (c) 1982 Taito Corporation */
	DRIVER( timetunl )	/* (c) 1982 Taito Corporation */
	DRIVER( wwestern )	/* (c) 1982 Taito Corporation */
	DRIVER( frontlin )	/* (c) 1982 Taito Corporation */
	DRIVER( elevator )	/* (c) 1983 Taito Corporation */
	DRIVER( elevatob )	/* bootleg */
	DRIVER( tinstar )	/* (c) 1983 Taito Corporation */
	DRIVER( waterski )	/* (c) 1983 Taito Corporation */
	DRIVER( bioatack )	/* (c) 1983 Taito Corporation + Fox Video Games license */
	DRIVER( sfposeid )	/* 1984 */

	/* other Taito games */
	DRIVER( bking2 )	/* (c) 1983 Taito Corporation */
	DRIVER( gsword )	/* (c) 1984 Taito Corporation */
	DRIVER( lkage )		/* (c) 1984 Taito Corporation */
	DRIVER( lkageb )	/* bootleg */
	DRIVER( retofinv )	/* (c) 1985 Taito Corporation */
	DRIVER( retofin1 )	/* bootleg */
	DRIVER( retofin2 )	/* bootleg */
	DRIVER( gladiatr )	/* (c) 1986 Taito America Corporation (US) */
	DRIVER( ogonsiro )	/* (c) 1986 Taito Corporation (Japan) */
//	DRIVER( gcastle )	/* bootleg */ temporarily removed since a complete dump is not available
	DRIVER( bublbobl )	/* (c) 1986 Taito Corporation */
	DRIVER( bublbobr )	/* (c) 1986 Taito America Corporation */
	DRIVER( boblbobl )	/* bootleg */
	DRIVER( sboblbob )	/* bootleg */
	DRIVER( tokio )		/* 1986 */
	DRIVER( tokiob )	/* bootleg */
//	DRIVER( mexico86 )
//	DRIVER( kicknrun )
	DRIVER( rastan )	/* (c) 1987 Taito Corporation Japan (World) */
	DRIVER( rastanu )	/* (c) 1987 Taito America Corporation (US) */
	DRIVER( rastanu2 )	/* (c) 1987 Taito America Corporation (US) */
	DRIVER( rastsaga )	/* (c) 1987 Taito Corporation (Japan)*/
	DRIVER( rainbow )	/* (c) 1987 Taito Corporation */
	DRIVER( rainbowe )	/* (c) 1988 Taito Corporation */
	DRIVER( jumping )	/* bootleg */
	DRIVER( arkanoid )	/* (c) 1986 Taito Corporation Japan (World) */
	DRIVER( arknoidu )	/* (c) 1986 Taito America Corporation + Romstar license (US) */
	DRIVER( arknoidj )	/* (c) 1986 Taito Corporation (Japan) */
	DRIVER( arkbl2 )	/* bootleg */
	DRIVER( arkatayt )	/* bootleg */
	DRIVER( arkangc )	/* bootleg */
	DRIVER( superqix )	/* 1987 */
	DRIVER( sqixbl )	/* bootleg? but (c) 1987 */
	DRIVER( extrmatn )	/* (c) 1987 World Games */
	DRIVER( arkanoi2 )	/* (c) 1987 Taito Corporation Japan (World) */
	DRIVER( ark2us )	/* (c) 1987 Taito America Corporation + Romstar license */
	DRIVER( tnzs )		/* (c) 1988 Taito Corporation (Japan) (new logo) */
	DRIVER( tnzsb )		/* bootleg but Taito Corporation Japan (World) (new logo) */
	DRIVER( tnzs2 )		/* (c) 1988 Taito Corporation Japan (World) (old logo) */
	DRIVER( insectx )	/* (c) 1989 Taito Corporation Japan (World) */
	DRIVER( superman )	/* (c) 1988 Taito Corporation */

	/* Taito F2 games */
	DRIVER( ssi )		/* (c) 1990 Taito Corporation Japan (World) */
	/* Majestic 12 (c) 1990 Taito America Corporation (US) */
	DRIVER( majest12 )	/* (c) 1990 Taito Corporation (Japan) */
	DRIVER( liquidk )	/* (c) 1990 Taito Corporation Japan (World) */
	DRIVER( liquidku )	/* (c) 1990 Taito America Corporation (US) */
	DRIVER( mizubaku )	/* (c) 1990 Taito Corporation (Japan) */
	DRIVER( growl )		/* (c) 1990 Taito Corporation Japan (World) */
	/* Growl (c) 1990 Taito America Corporation (US) */
	/* Runark (c) 1990 Taito Corporation (Japan) */

	/* Toaplan games */
	DRIVER( tigerh )	/* GX-551 [not a Konami board!] */
	DRIVER( tigerh2 )	/* GX-551 [not a Konami board!] */
	DRIVER( tigerhb1 )	/* bootleg but (c) 1985 Taito Corporation */
	DRIVER( tigerhb2 )	/* bootleg but (c) 1985 Taito Corporation */
	DRIVER( slapfigh )	/* TP-??? */
	DRIVER( slapbtjp )	/* bootleg but (c) 1986 Taito Corporation */
	DRIVER( slapbtuk )	/* bootleg but (c) 1986 Taito Corporation */
	DRIVER( alcon )		/* TP-??? */
	DRIVER( getstar )	/* TP-??? bootleg but (c) 1986 Taito Corporation */

	DRIVER( fshark )	/* TP-007 (c) 1987 Taito Corporation (World) */
	DRIVER( skyshark )	/* TP-007 (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( hishouza )	/* TP-007 (c) 1987 Taito Corporation (Japan) */
	DRIVER( fsharkbt )	/* bootleg */
	DRIVER( wardner )	/* TP-??? (c) 1987 Taito Corporation Japan (World) */
	DRIVER( pyros )		/* TP-??? (c) 1987 Taito America Corporation (US) */
	DRIVER( wardnerj )	/* TP-??? (c) 1987 Taito Corporation Japan (Japan) */
	DRIVER( twincobr )	/* TP-011 (c) 1987 Taito Corporation (World) */
	DRIVER( twincobu )	/* TP-011 (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( ktiger )	/* TP-011 (c) 1987 Taito Corporation (Japan) */

	DRIVER( rallybik )	/* TP-012 (c) 1988 Taito */
	DRIVER( truxton )	/* TP-013B (c) 1988 Taito */
	DRIVER( hellfire )	/* TP-??? (c) 1989 Toaplan + Taito license */
	DRIVER( zerowing )	/* TP-015 (c) 1989 Toaplan */
	DRIVER( demonwld )	/* TP-016 (c) 1989 Toaplan + Taito license */
	DRIVER( outzone )	/* TP-018 (c) 1990 Toaplan */
	DRIVER( vimana )	/* TP-019 (c) 1991 Toaplan (+ Tecmo license when set to Japan) */
	DRIVER( vimana2 )	/* TP-019 (c) 1991 Toaplan (+ Tecmo license when set to Japan)  */

	DRIVER( snowbros )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbroa )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbroj )	/* MIN16-02 (c) 1990 Toaplan */

/*
Toa Plan's board list
(translated from http://www.aianet.ne.jp/~eisetu/rom/rom_toha.html)

Title              ROMno.   Remark(1)   Remark(2)
--------------------------------------------------
Tiger Heli           A47      GX-551
Hishouzame           B02      TP-007
Kyukyoku Tiger       B30      TP-011
Dash Yarou           B45      TP-012
Tatsujin             B65      TP-013B   M6100649A
Zero Wing            O15      TP-015
Horror Story         O16      TP-016
Same!Same!Same!      O17      TP-017
Out Zone                      TP-018
Vimana                        TP-019
Teki Paki            O20      TP-020
Ghox               TP-21      TP-021
Dogyuun                       TP-022
Tatsujin Oh                   TP-024    *1
Fixeight                      TP-026
V-V                           TP-027

*1 There is a doubt this game uses TP-024 board and TP-025 romsets.

   86 Mahjong Sisters                                 Kit 2P 8W+2B     HC    Mahjong TP-
   88 Dash                                            Kit 2P 8W+2B                   TP-
   89 Fire Shark                                      Kit 2P 8W+2B     VC    Shooter TP-017
   89 Twin Hawk                                       Kit 2P 8W+2B     VC    Shooter TP-
   91 Whoopie                                         Kit 2P 8W+2B     HC    Action
   92 Teki Paki                                       Kit 2P                         TP-020
   92 Ghox                                            Kit 2P Paddle+1B VC    Action  TP-021
10/92 Dogyuun                                         Kit 2P 8W+2B     VC    Shooter TP-022
92/93 Knuckle Bash                 Atari Games        Kit 2P 8W+2B     HC    Action  TP-023
10/92 Tatsujin II/Truxton II       Taito              Kit 2P 8W+2B     VC    Shooter TP-029(?)
10/92 Truxton II/Tatsujin II       Taito              Kit 2P 8W+2B     VC    Shooter TP-029(?)
      Pipi & Bipi                                                                    TP-025
   92 Fix Eight                                       Kit 2P 8W+2B     VC    Action  TP-026
12/92 V  -  V (5)/Grind Stormer                       Kit 2P 8W+2B     VC    Shooter TP-027
 1/93 Grind Stormer/V - V (Five)                      Kit 2P 8W+2B     VC    Shooter TP-027
 2/94 Batsugun                                        Kit 2P 8W+2B     VC            TP-
 4/94 Snow Bros. 2                                    Kit 2P 8W+2B     HC    Action  TP-
*/

	/* Williams games */
	DRIVER( robotron )	/* (c) 1982 */
	DRIVER( robotryo )	/* (c) 1982 */
	DRIVER( stargate )	/* (c) 1981 */
	DRIVER( joust )		/* (c) 1982 */
	DRIVER( joustwr )	/* (c) 1982 */
	DRIVER( joustr )	/* (c) 1982 */
	DRIVER( sinistar )	/* (c) 1982 */
	DRIVER( sinista1 )	/* (c) 1982 */
	DRIVER( sinista2 )	/* (c) 1982 */
	DRIVER( bubbles )	/* (c) 1982 */
	DRIVER( bubblesr )	/* (c) 1982 */
	DRIVER( defender )	/* (c) 1980 */
	DRIVER( defendg )	/* (c) 1980 */
	DRIVER( splat )		/* (c) 1982 */
	DRIVER( blaster )	/* (c) 1983 */
	DRIVER( colony7 )	/* (c) 1981 Taito */
	DRIVER( colony7a )	/* (c) 1981 Taito */
	DRIVER( lottofun )	/* (c) 1987 H.A.R. Management */
	DRIVER( defcmnd )	/* bootleg */
	DRIVER( defence )	/* bootleg */
	DRIVER( mysticm )	/* (c) 1983 */
	DRIVER( tshoot )	/* (c) 1984 */
	DRIVER( inferno )	/* (c) 1984 */
	DRIVER( joust2 )	/* (c) 1986 */

	/* Capcom games */
	/* The following is a COMPLETE list of the Capcom games up to 1997, as shown on */
	/* their web site. The list is sorted by production date. Some titles are in */
	/* quotes, because I couldn't find the English name (might not have been exported). */
	/* The name in quotes is the title Capcom used for the html page. */
	DRIVER( vulgus )	/*  5/1984 (c) 1984 */
	DRIVER( vulgus2 )	/*  5/1984 (c) 1984 */
	DRIVER( vulgusj )	/*  5/1984 (c) 1984 */
	DRIVER( sonson )	/*  7/1984 (c) 1984 */
	DRIVER( higemaru )	/*  9/1984 (c) 1984 */
	DRIVER( c1942 )		/* 12/1984 (c) 1984 */
	DRIVER( c1942a )	/* 12/1984 (c) 1984 */
	DRIVER( c1942b )	/* 12/1984 (c) 1984 */
	DRIVER( exedexes )	/*  2/1985 (c) 1985 */
	DRIVER( savgbees )	/*  2/1985 (c) 1985 + Memetron license */
	DRIVER( commando )	/*  5/1985 (c) 1985 (World) */
	DRIVER( commandu )	/*  5/1985 (c) 1985 + Data East license (US) */
	DRIVER( commandj )	/*  5/1985 (c) 1985 (Japan) */
	DRIVER( spaceinv )	/* bootleg */
	DRIVER( gng )		/*  9/1985 (c) 1985 */
	DRIVER( gnga )		/*  9/1985 (c) 1985 */
	DRIVER( gngt )		/*  9/1985 (c) 1985 */
	DRIVER( makaimur )	/*  9/1985 (c) 1985 */
	DRIVER( makaimuc )	/*  9/1985 (c) 1985 */
	DRIVER( makaimug )	/*  9/1985 (c) 1985 */
	DRIVER( diamond )	/* (c) 1989 KH Video (NOT A CAPCOM GAME but runs on GnG hardware) */
	DRIVER( gunsmoke )	/* 11/1985 (c) 1985 (World) */
	DRIVER( gunsmrom )	/* 11/1985 (c) 1985 + Romstar (US) */
	DRIVER( gunsmoka )	/* 11/1985 (c) 1985 (US) */
	DRIVER( gunsmokj )	/* 11/1985 (c) 1985 (Japan) */
	DRIVER( sectionz )	/* 12/1985 (c) 1985 */
	DRIVER( sctionza )	/* 12/1985 (c) 1985 */
	DRIVER( trojan )	/*  4/1986 (c) 1986 + Romstar */
	DRIVER( trojanj )	/*  4/1986 (c) 1986 */
	DRIVER( srumbler )	/*  9/1986 (c) 1986 */	/* aka Rush'n Crash */
	DRIVER( srumblr2 )	/*  9/1986 (c) 1986 */
	DRIVER( lwings )	/* 11/1986 (c) 1986 */
	DRIVER( lwings2 )	/* 11/1986 (c) 1986 */
	DRIVER( lwingsjp )	/* 11/1986 (c) 1986 */
	DRIVER( sidearms )	/* 12/1986 (c) 1986 (World) */
	DRIVER( sidearmr )	/* 12/1986 (c) 1986 + Romstar license (US) */
	DRIVER( sidearjp )	/* 12/1986 (c) 1986 (Japan) */
	DRIVER( turtship )	/* (c) 1988 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( dyger )		/* (c) 1989 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( avengers )	/*  2/1987 (c) 1987 (US) */
	DRIVER( avenger2 )	/*  2/1987 (c) 1987 (US) */
	DRIVER( bionicc )	/*  3/1987 (c) 1987 (US) */
	DRIVER( bionicc2 )	/*  3/1987 (c) 1987 (US) */
	DRIVER( topsecrt )	/*  3/1987 (c) 1987 (Japan) */
	DRIVER( c1943 )		/*  6/1987 (c) 1987 (US) */
	DRIVER( c1943j )	/*  6/1987 (c) 1987 (Japan) */
	DRIVER( blktiger )	/*  8/1987 (c) 1987 (US) */
	DRIVER( bktigerb )	/* bootleg */
	DRIVER( blkdrgon )	/*  8/1987 (c) 1987 (Japan) */
	DRIVER( blkdrgnb )	/* bootleg, hacked to say Black Tiger */
	DRIVER( sf1 )		/*  8/1987 (c) 1987 (World) */
	DRIVER( sf1us )		/*  8/1987 (c) 1987 (US) */
	DRIVER( sf1jp )		/*  8/1987 (c) 1987 (Japan) */
	DRIVER( tigeroad )	/* 11/1987 (c) 1987 + Romstar (US) */
	DRIVER( f1dream )	/*  4/1988 (c) 1988 + Romstar */
	DRIVER( f1dreamb )	/* bootleg */
	DRIVER( c1943kai )	/*  6/1988 (c) 1987 (Japan) */
	DRIVER( lastduel )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lstduela )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lstduelb )	/* bootleg */
	DRIVER( madgear )	/*  2/1989 (c) 1989 (US) */
	DRIVER( ledstorm )	/*  2/1989 (c) 1989 (US) */
	/*  3/1989 Dokaben (baseball) - see below among "Mitchell" games */
	/*  8/1989 Dokaben 2 (baseball) - see below among "Mitchell" games */
	/* 10/1989 Capcom Baseball - see below among "Mitchell" games */
	/* 11/1989 Capcom World - see below among "Mitchell" games */
	/*  3/1990 Adventure Quiz 2 Hatena no Dai-Bouken - see below among "Mitchell" games */
	/*  1/1991 Quiz Tonosama no Yabou - see below among "Mitchell" games */
	/*  4/1991 Ashita Tenki ni Naare (golf) - see below among "Mitchell" games */
	/*  5/1991 Ataxx - see below among "Leland" games */
	/*  6/1991 Quiz Sangokushi - see below among "Mitchell" games */
	/* 10/1991 Block Block - see below among "Mitchell" games */
	/*  6/1995 Street Fighter - the Movie - see below among "Incredible Technologies" games */
	/* 11/1995 Battle Arena Toshinden 2 (3D, not CPS) */
	/*  7/1996 Star Gladiator (3D, not CPS) */
	/* 12/1996 Street Fighter EX */
	/*  4/1997 Street Fighter EX Plus */

	/* Capcom CPS1 games */
	DRIVER( forgottn )	/*  7/1988 (c) 1988 (US) */	/* aka Lost Worlds */
	DRIVER( ghouls )	/* 12/1988 (c) 1988 */
	DRIVER( ghoulsj )	/* 12/1988 (c) 1988 */
	DRIVER( strider )	/*  3/1989 (c) 1989 */
	DRIVER( striderj )	/*  3/1989 (c) 1989 */
	DRIVER( dwj )		/*  4/1989 (c) 1989 */
	DRIVER( willow )	/*  6/1989 (c) 1989 (Japan) */
	DRIVER( willowj )	/*  6/1989 (c) 1989 (Japan) */
	DRIVER( unsquad )	/*  8/1989 (c) 1989 */
	DRIVER( area88 )	/*  8/1989 (c) 1989 */
	DRIVER( ffight )	/* 12/1989 (c) (World) */
	DRIVER( ffightu )	/* 12/1989 (c) (US)    */
	DRIVER( ffightj )	/* 12/1989 (c) (Japan) */
	DRIVER( c1941 )		/*  2/1990 (c) 1990 (World) */
	DRIVER( c1941j )	/*  2/1990 (c) 1990 (Japan) */
	DRIVER( mercs )		/*  3/ 2/1990 (c) 1990 (World) */
	DRIVER( mercsu )	/*  3/ 2/1990 (c) 1990 (US)    */
	DRIVER( mercsj )	/*  3/ 2/1990 (c) 1990 (Japan) */
	DRIVER( mtwins )	/*  6/19/1990 (c) 1990 (World) */
	DRIVER( chikij )	/*  6/19/1990 (c) 1990 (Japan) */
	DRIVER( msword )	/*  7/25/1990 (c) 1990 (World) */
	DRIVER( mswordu )	/*  7/25/1990 (c) 1990 (US)    */
	DRIVER( mswordj )	/*  6/23/1990 (c) 1990 (Japan) */
	DRIVER( cawing )	/* 10/12/1990 (c) 1990 (World) */
	DRIVER( cawingj )	/* 10/12/1990 (c) 1990 (Japan) */
	DRIVER( nemo )		/* 11/30/1990 (c) 1990 (World) */
	DRIVER( nemoj )		/* 11/20/1990 (c) 1990 (Japan) */
	DRIVER( sf2 )		/*  2/14/1991 (c) 1991 (World) */
	DRIVER( sf2a )		/*  2/ 6/1991 (c) 1991 (US)    */
	DRIVER( sf2b )		/*  2/14/1991 (c) 1991 (US)    */
	DRIVER( sf2e )		/*  2/28/1991 (c) 1991 (US)    */
	DRIVER( sf2j )		/* 12/10/1991 (c) 1991 (Japan) */
	DRIVER( sf2jb )		/*  2/14/1991 (c) 1991 (Japan) */
	DRIVER( c3wonders )	/*  5/20/1991 (c) 1991 (US) */
	DRIVER( c3wonderj )	/*  5/20/1991 (c) 1991 (Japan) */
	DRIVER( kod )		/*  7/11/1991 (c) 1991 (World) */
	DRIVER( kodj )		/*  8/ 5/1991 (c) 1991 (Japan) */
	DRIVER( kodb )		/* bootleg */
	DRIVER( captcomm )	/* 10/14/1991 (c) 1991 (World) */
	DRIVER( captcomu )	/*  9/28/1991 (c) 1991 (US)    */
	DRIVER( captcomj )	/* 12/ 2/1991 (c) 1991 (Japan) */
	DRIVER( knights )	/* 11/27/1991 (c) 1991 (World) */
	DRIVER( knightsj )	/* 11/27/1991 (c) 1991 (Japan) */
	DRIVER( sf2ce )		/*  3/13/1992 (c) 1992 (World) */
	DRIVER( sf2cea )	/*  3/13/1992 (c) 1992 (US)    */
	DRIVER( sf2ceb )	/*  5/13/1992 (c) 1992 (US)    */
	DRIVER( sf2cej )	/*  5/13/1992 (c) 1992 (Japan) */
	DRIVER( sf2red )	/* hack */
	DRIVER( sf2accp2 )	/* hack */
//	DRIVER( sf2rb )		/* hack */
	DRIVER( varth )		/*  6/12/1992 (c) 1992 (World) */
	DRIVER( varthj )	/*  7/14/1992 (c) 1992 (Japan) */
	DRIVER( cworld2j )	/*  6/11/1992 (QUIZ 5) (c) 1992 (Japan) */
	DRIVER( wof )		/* 10/ 2/1992 (c) 1992 (World) (CPS1 + QSound) */
	DRIVER( wofj )		/* 10/31/1992 (c) 1992 (Japan) (CPS1 + QSound) */
	DRIVER( sf2t )		/* 12/ 9/1992 (c) 1992 (US)    */
	DRIVER( sf2tj )		/* 12/ 9/1992 (c) 1992 (Japan) */
	DRIVER( dino )		/*  2/ 1/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( dinoj )		/*  2/ 1/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( punisher )	/*  4/22/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( punishrj )	/*  4/22/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( slammast )	/*  7/13/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( mbomberj )	/*  7/13/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( mbombrd )	/* 12/ 6/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( mbombrdj )	/* 12/ 6/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( pnickj )	/*  6/ 8/1994 (c) 1994 + Compile license (Japan) not listed on Capcom's site */
	DRIVER( qad )		/*  7/ 1/1992 (c) 1992 (US)    */
	DRIVER( qadj )		/*  9/21/1994 (c) 1994 (Japan) */
	DRIVER( qtono2 )	/*  1/23/1995 (c) 1995 (Japan) */
	DRIVER( pang3 )		/*  5/11/1995 (c) 1995 Mitchell (Japan) not listed on Capcom's site */
	DRIVER( megaman )	/* 10/ 6/1995 (c) 1995 (Asia)  */
	DRIVER( rockmanj )	/*  9/22/1995 (c) 1995 (Japan) */
	DRIVER( sfzch )		/* 10/20/1995 (c) 1995 (Japan) (CPS Changer) */

	/* Capcom CPS2 games */
	/* list completed with Chris Mullins' FAQ */
	/* http://members.aol.com/CMull11217/private/index.htm */
	/* 10/1993 Super Street Fighter II */
	/*  1/1994 Dungeons & Dragons - Tower of Doom */
	/*  3/1994 Super Street Fighter II Turbo / Super Street Fighter II X */
	/*  5/1994 Alien vs Predator */
	/*  6/1994 Eco Fighters / Ultimate Ecology */
	/*  7/1994 Dark Stalkers / Vampire */
	/*  9/1994 Saturday Night Slam Masters 2 - Ring of Destruction / Super Muscle Bomber */
	/* 10/1994 Armored Warriors / Powered Gear */
	/* 12/1994 X-Men - Children of the Atom */
	/*  3/1995 Night Warriors - Dark Stalkers Revenge / Vampire Hunter */
	/*  4/1995 Cyberbots */
	/*  6/1995 Street Fighter Alpha / Street Fighter Zero */
	/* 11/1995 Marvel Super Heroes */
	/*  1/1996 19XX: The Battle Against Destiny */
	/*  2/1996 Dungeons & Dragons - Shadow Over Mystara */
	/*  3/1996 Street Fighter Alpha 2 / Street Fighter Zero 2 */
	/*  6/1996 Super Puzzle Fighter II / Turbo Super Puzzle Fighter II X */
	/*  7/1996 Rockman 2 - The Power Fighters */
	/*  8/1996 Street Fighter Zero 2 Alpha */
	/*  9/1996 Quiz Naneiro Dreams */
	/*  9/1996 X-Men vs. Street Fighter */
	/* 1996 Dark Stalkers 3 - Jedah's Damnation / Vampire Savior */
	/* 1997 Battle Circuit */
	/* 1997 Marvel Super Heroes vs Street Fighter */
	/* 1997 Super Gem Fighter / Pocket Fighter */
	/* 1997 Vampire Hunter 2 */
	/* 1997 Vampire Savior 2 */
	/* 1998 Marvel vs Capcom */
	/* 1998 Street Fighter Alpha 3 / Street Fighter Zero 3 */
	/* 1999 Giga Wing */
	/* another unlisted puzzle game: Gulum Pa! */

	/* Capcom CPS3 games */
	/* 10/1996 Warzard */
	/*  2/1997 Street Fighter III - New Generation */
	/* ???? Jojo's Bizarre Adventure */
	/* ???? Street Fighter 3: Second Impact ~giant attack~ */
	/* ???? Street Fighter 3: Third Strike ~fight to the finish~ */

	/* Mitchell games */
	DRIVER( mgakuen )	/* (c) 1988 Yuga */
	DRIVER( mgakuen2 )	/* (c) 1989 Face */
	DRIVER( pkladies )	/* (c) 1989 Mitchell */
//	DRIVER( pkladisb )
	DRIVER( pang )		/* (c) 1989 Mitchell (World) */
	DRIVER( pangb )		/* bootleg */
	DRIVER( bbros )		/* (c) 1989 Capcom (US) not listed on Capcom's site */
	/*  3/1989 Dokaben (baseball) */
	/*  8/1989 Dokaben 2 (baseball) */
	/* 10/1989 Capcom Baseball */
	DRIVER( cworld )	/* 11/1989 (QUIZ 1) (c) 1989 Capcom */
	DRIVER( hatena )	/*  2/28/1990 (QUIZ 2) (c) 1990 Capcom (Japan) */
	DRIVER( spang )		/*  9/14/1990 (c) 1990 Mitchell (World) */
	DRIVER( sbbros )	/* 10/ 1/1990 (c) 1990 Mitchell + Capcom (US) not listed on Capcom's site */
	DRIVER( marukin )	/* 10/17/1990 (c) 1990 Yuga (Japan) */
	DRIVER( qtono1 )	/* 12/25/1990 (QUIZ 3) (c) 1991 Capcom (Japan) */
	/*  4/1991 Ashita Tenki ni Naare (golf) */
	DRIVER( qsangoku )	/*  6/ 7/1991 (QUIZ 4) (c) 1991 Capcom (Japan) */
	DRIVER( block )		/*  9/10/1991 (c) 1991 Capcom (World) */
	DRIVER( blockj )	/*  9/10/1991 (c) 1991 Capcom (Japan) */
	DRIVER( blockbl )	/* bootleg */

	/* Incredible Technologies games */
	DRIVER( capbowl )	/* (c) 1988 Incredible Technologies */
	DRIVER( capbowl2 )	/* (c) 1988 Incredible Technologies */
	DRIVER( clbowl )	/* (c) 1989 Incredible Technologies */
	DRIVER( bowlrama )	/* (c) 1991 P & P Marketing */
/*
The Incredible Technologies game list
http://www.itsgames.com/it/CorporateProfile/corporateprofile_main.htm

ShuffleShot - (Incredible Technologies, Inc.)
Peter Jacobsen's Golden Tee '97 - (Incredible Technologies, Inc.)
World Class Bowling - (Incredible Technologies, Inc.)
Peter Jacobsen's Golden Tee 3D Golf - (Incredible Technologies, Inc.)
Street Fighter - "The Movie" (Capcom)
PAIRS - (Strata)
BloodStorm - (Strata)
Driver's Edge - (Strata)
NFL Hard Yardage - (Strata)
Time Killers - (Strata)
Neck 'n' Neck - (Bundra Games)
Ninja Clowns - (Strata)
Rim Rockin' Basketball - (Strata)
Arlington Horse Racing - (Strata)
Dyno Bop - (Grand Products)
Poker Dice - (Strata)
Peggle - (Strata)
Slick Shot - (Grand Products)
Golden Tee Golf II - (Strata)
Hot Shots Tennis - (Strata)
Strata Bowling - (Strata)
Golden Tee Golf I - (Strata)
Capcom Bowling - (Strata)
*/

	/* Leland games */
//	DRIVER( mayhem )	/* (c) 1985 Cinematronics */
//	DRIVER( wseries )	/* (c) 1985 Cinematronics Inc. */
//	DRIVER( dangerz )	/* (c) 1986 Cinematronics USA Inc. */
//	DRIVER( basebal2 )	/* (c) 1987 Cinematronics Inc. */
//	DRIVER( dblplay )	/* (c) 1987 Tradewest / The Leland Corp. */
//	DRIVER( teamqb )	/* (c) 1988 Leland Corp. */
//	DRIVER( strkzone )	/* (c) 1988 The Leland Corporation */
//	DRIVER( offroad )	/* (c) 1989 Leland Corp. */
//	DRIVER( offroadt )
//	DRIVER( pigout )	/* (c) 1990 The Leland Corporation */
//	DRIVER( pigoutj )	/* (c) 1990 The Leland Corporation */
//	DRIVER( redlin2p )
//	DRIVER( viper )
//	DRIVER( aafb )
//	DRIVER( aafb2p )
//	DRIVER( alleymas )
//	DRIVER( ataxx )

	/* Gremlin 8080 games */
	/* the numbers listed are the range of ROM part numbers */
	DRIVER( blockade )	/* 1-4 [1977 Gremlin] */
	DRIVER( comotion )	/* 5-7 [1977 Gremlin] */
	DRIVER( hustle )	/* 16-21 [1977 Gremlin] */
	DRIVER( blasto )	/* [1978 Gremlin] */

	/* Gremlin/Sega "VIC dual game board" games */
	/* the numbers listed are the range of ROM part numbers */
	DRIVER( depthch )	/* 50-55 [1977 Gremlin?] */
	DRIVER( safari )	/* 57-66 [1977 Gremlin?] */
	DRIVER( frogs )		/* 112-119 [1978 Gremlin?] */
	DRIVER( sspaceat )	/* 155-162 (c) */
	DRIVER( sspacatc )	/* 139-146 (c) */
	DRIVER( headon )	/* 163-167/192-193 (c) Gremlin */
	DRIVER( headonb )	/* 163-167/192-193 (c) Gremlin */
	DRIVER( headon2 )	/* ???-??? (c) 1979 Sega */
	/* ???-??? Fortress */
	/* ???-??? Gee Bee */
	/* 255-270  Head On 2 / Deep Scan */
	DRIVER( invho2 )	/* 271-286 (c) 1979 Sega */
	DRIVER( samurai )	/* 289-302 + upgrades (c) 1980 Sega */
	DRIVER( invinco )	/* 310-318 (c) 1979 Sega */
	DRIVER( invds )		/* 367-382 (c) 1979 Sega */
	DRIVER( tranqgun )	/* 413-428 (c) 1980 Sega */
	/* 450-465  Tranquilizer Gun (different version?) */
	/* ???-??? Car Hunt / Deep Scan */
	DRIVER( spacetrk )	/* 630-645 (c) 1980 Sega */
	DRIVER( sptrekct )	/* (c) 1980 Sega */
	DRIVER( carnival )	/* 651-666 (c) 1980 Sega */
	DRIVER( carnvckt )	/* 501-516 (c) 1980 Sega */
	DRIVER( digger )	/* 684-691 no copyright notice */
	DRIVER( pulsar )	/* 790-805 (c) 1981 Sega */
	DRIVER( heiankyo )	/* (c) [1979?] Denki Onkyo */

	/* Sega G-80 vector games */
	DRIVER( spacfury )	/* (c) 1981 */
	DRIVER( spacfura )	/* no copyright notice */
	DRIVER( zektor )	/* (c) 1982 */
	DRIVER( tacscan )	/* (c) */
	DRIVER( elim2 )		/* (c) 1981 Gremlin */
	DRIVER( elim2a )	/* (c) 1981 Gremlin */
	DRIVER( elim4 )		/* (c) 1981 Gremlin */
	DRIVER( startrek )	/* (c) 1982 */

	/* Sega G-80 raster games */
	DRIVER( astrob )	/* (c) 1981 */
	DRIVER( astrob1 )	/* (c) 1981 */
	DRIVER( s005 )		/* (c) 1981 */
	DRIVER( monsterb )	/* (c) 1982 */
	DRIVER( spaceod )	/* (c) 1981 */
	DRIVER( pignewt )	/* (c) 1983 */
	DRIVER( pignewta )	/* (c) 1983 */
	DRIVER( sindbadm )	/* 834-5244 (c) 1983 Sega */

	/* Sega "Zaxxon hardware" games */
	DRIVER( zaxxon )	/* (c) 1982 */
	DRIVER( zaxxon2 )	/* (c) 1982 */
	DRIVER( zaxxonb )	/* bootleg */
	DRIVER( szaxxon )	/* (c) 1982 */
	DRIVER( futspy )	/* (c) 1984 */
	DRIVER( congo )		/* 605-5167 (c) 1983 */
	DRIVER( tiptop )	/* 605-5167 (c) 1983 */

	/* Sega System 1 / System 2 games */
	DRIVER( starjack )	/* 834-5191 (c) 1983 (S1) */
	DRIVER( starjacs )	/* (c) 1983 Stern (S1) */
	DRIVER( regulus )	/* 834-5328 (c) 1983 (S1) */
	DRIVER( regulusu )	/* 834-5328 (c) 1983 (S1) */
	DRIVER( upndown )	/* (c) 1983 (S1) */
	DRIVER( mrviking )	/* 834-5383 (c) 1984 (S1) */
	DRIVER( swat )		/* 834-5388 (c) 1984 Coreland / Sega (S1) */
	DRIVER( flicky )	/* (c) 1984 (S1) */
	DRIVER( flicky2 )	/* (c) 1984 (S1) */
	/* Water Match (S1) */
	DRIVER( bullfgtj )	/* 834-5478 (c) 1984 Sega / Coreland (S1) */
	DRIVER( pitfall2 )	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision (S1) */
	DRIVER( pitfallu )	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision (S1) */
	DRIVER( seganinj )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( seganinu )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprinces )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprincsu )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprincsb )	/* bootleg? (S1) */
	DRIVER( imsorry )	/* 834-5707 (c) 1985 Coreland / Sega (S1) */
	DRIVER( imsorryj )	/* 834-5707 (c) 1985 Coreland / Sega (S1) */
	DRIVER( teddybb )	/* 834-5712 (c) 1985 (S1) */
	DRIVER( hvymetal )	/* 834-5745 (c) 1985 (S2?) */
	DRIVER( myhero )	/* 834-5755 (c) 1985 (S1) */
	DRIVER( myheroj )	/* 834-5755 (c) 1985 Coreland / Sega (S1) */
	DRIVER( myherok )	/* 834-5755 (c) 1985 Coreland / Sega (S1) */
	DRIVER( shtngmst )	/* 834-5719/5720 (c) 1985 (S2) */
	DRIVER( chplft )	/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin (S2) */
	DRIVER( chplftb )	/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin (S2) */
	DRIVER( chplftbl )	/* bootleg (S2) */
	DRIVER( fdwarrio )	/* 834-5918 (c) 1985 Coreland / Sega (S1) */
	DRIVER( brain )		/* (c) 1986 Coreland / Sega (S2?) */
	DRIVER( wboy )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboy2 )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboy3 )
	DRIVER( wboy4 )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboyu )		/* 834-5753 (? maybe a conversion) (c) 1986 + Escape license (S1) */
	DRIVER( wboy4u )	/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wbdeluxe )	/* (c) 1986 + Escape license (S1) */
	DRIVER( gardia )	/* 834-6119 (S2?) */
	DRIVER( blockgal )	/* 834-6303 (S1) */
	DRIVER( tokisens )	/* (c) 1987 (from a bootleg board) (S2) */
	DRIVER( dakkochn )	/* 836-6483? (S2) */
	DRIVER( ufosensi )	/* 834-6659 (S2) */
	DRIVER( wbml )		/* bootleg (S2?) */
/*
other System 1 / System 2 games:

WarBall
Rafflesia
Sanrin Sanchan
DokiDoki Penguin Land *not confirmed
*/

	/* other Sega 8-bit games */
	DRIVER( turbo )		/* (c) 1981 Sega */
	DRIVER( turboa )	/* (c) 1981 Sega */
	DRIVER( turbob )	/* (c) 1981 Sega */
//	DRIVER( kopunch )	/* 834-0103 (c) 1981 Sega */
	DRIVER( suprloco )	/* (c) 1982 Sega */
	DRIVER( champbas )	/* (c) 1983 Sega */
	DRIVER( champbb2 )
	DRIVER( appoooh )	/* (c) 1984 Sega */
	DRIVER( bankp )		/* (c) 1984 Sega */

	/* Sega System 16 games */
	DRIVER( alexkidd )	/* (c) 1986 (but bootleg) */
	DRIVER( aliensyn )	/* (c) 1987 */
	DRIVER( altbeast )	/* (c) 1988 */
	DRIVER( astormbl )
	DRIVER( aurail )
	DRIVER( dduxbl )
	DRIVER( eswatbl )	/* (c) 1989 (but bootleg) */
	DRIVER( fantzone )
	DRIVER( fpointbl )
	DRIVER( goldnaxe )	/* (c) 1989 */
	DRIVER( hwchamp )
	DRIVER( mjleague )	/* (c) 1985 */
	DRIVER( passshtb )	/* bootleg */
	DRIVER( quartet2 )
	DRIVER( sdi )		/* (c) 1987 */
	DRIVER( shinobi )	/* (c) 1987 */
	DRIVER( tetrisbl )	/* (c) 1988 (but bootleg) */
	DRIVER( timscanr )
	DRIVER( tturfbl )
	DRIVER( wb3bl )
	DRIVER( wrestwar )	/* (c) 1989 */

	/* Data East "Burger Time hardware" games */
	DRIVER( lnc )		/* (c) 1981 */
	DRIVER( zoar )		/* (c) 1982 */
	DRIVER( btime )		/* (c) 1982 */
	DRIVER( btime2 )	/* (c) 1982 */
	DRIVER( btimem )	/* (c) 1982 + Midway */
	DRIVER( wtennis )	/* bootleg 1982 */
	DRIVER( brubber )	/* (c) 1982 */
	DRIVER( bnj )		/* (c) 1982 + Midway */
	DRIVER( caractn )	/* bootleg */
	DRIVER( disco )		/* (c) 1982 */
	DRIVER( mmonkey )	/* (c) 1982 Technos Japan + Roller Tron */
	DRIVER( scregg )	/* TA-0001 (c) 1983 Technos Japan */
	DRIVER( eggs )		/* TA-0002 (c) 1983 Universal USA */
	DRIVER( tagteam )	/* TA-0007 (c) 1983 + Technos Japan license */
	/* cassette system */
//	DRIVER( decocass )
	DRIVER( cookrace )	/* bootleg */

	/* other Data East games */
	DRIVER( astrof )	/* (c) [1980?] */
	DRIVER( astrof2 )	/* (c) [1980?] */
	DRIVER( astrof3 )	/* (c) [1980?] */
	DRIVER( tomahawk )	/* (c) [1980?] */
	DRIVER( tomahaw5 )	/* (c) [1980?] */
	DRIVER( kchamp )	/* (c) 1984 Data East USA (US) */
	DRIVER( kchampvs )	/* (c) 1984 Data East USA (US) */
	DRIVER( karatedo )	/* (c) 1984 Data East Corporation (Japan) */
	DRIVER( firetrap )	/* (c) 1986 */
	DRIVER( firetpbl )	/* bootleg */
	DRIVER( brkthru )	/* (c) 1986 Data East USA (US) */
	DRIVER( brkthruj )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( darwin )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( shootout )	/* (c) 1985 Data East USA (US) */
	DRIVER( sidepckt )	/* (c) 1986 Data East Corporation (World?) */
	DRIVER( exprraid )	/* (c) 1986 Data East USA (US) */
	DRIVER( wexpress )	/* (c) 1986 Data East Corporation (World?) */
	DRIVER( wexpresb )	/* bootleg */
	DRIVER( pcktgal )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( pcktgalb )	/* bootleg */
	DRIVER( pcktgal2 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3i )	/* (c) 1990 Data East Corporation + I-Vics license */
	DRIVER( actfancr )	/* (c) 1989 Data East Corporation */
	DRIVER( actfancj )	/* (c) 1989 Data East Corporation */

	/* Data East 8-bit games */
	DRIVER( lastmiss )	/* (c) 1986 Data East USA (US) */
	DRIVER( lastmss2 )	/* (c) 1986 Data East USA (US) */
	DRIVER( shackled )	/* (c) 1986 Data East USA (US) */
	DRIVER( breywood )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( csilver )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( ghostb )	/* (c) 1987 Data East USA (US) */
	DRIVER( ghostb3 )	/* (c) 1987 Data East USA (US) */
	DRIVER( meikyuh )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( srdarwin )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( gondo )		/* (c) 1987 Data East USA (US) */
	DRIVER( makyosen )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( cobracom )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( oscar )		/* (c) 1988 Data East USA (US) */
	DRIVER( oscarj )		/* (c) 1987 Data East Corporation (Japan) */

	/* Data East 16-bit games */
	DRIVER( karnov )	/* (c) 1987 Data East USA (US) */
	DRIVER( karnovj )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( chelnov )	/* (c) 1988 Data East USA (US) */
	DRIVER( chelnovj )	/* (c) 1988 Data East Corporation (Japan) */
	/* the following ones all run on similar hardware */
	DRIVER( hbarrel )	/* (c) 1987 Data East USA (US) */
	DRIVER( hbarrelj )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( baddudes )	/* (c) 1988 Data East USA (US) */
	DRIVER( drgninja )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( robocopp )	/* bootleg */
	DRIVER( hippodrm )	/* (c) 1989 Data East USA (US) */
	DRIVER( ffantasy )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( midres )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( midresu )	/* (c) 1989 Data East USA (US) */
	DRIVER( slyspy )	/* (c) 1989 Data East USA (US) */
	DRIVER( slyspy2 )	/* (c) 1989 Data East USA (US) */
	DRIVER( bouldash )	/* (c) 1990 Data East Corporation */
	/* evolution of the hardware */
	DRIVER( darkseal )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( darksea1 )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( gatedoom )	/* (c) 1990 Data East Corporation (US) */
	DRIVER( gatedom1 )	/* (c) 1990 Data East Corporation (US) */
	DRIVER( supbtime )	/* (c) 1990 Data East Corporation (Japan) */
	DRIVER( cninja )	/* (c) 1991 Data East Corporation (World) */
	DRIVER( cninja0 )	/* (c) 1991 Data East Corporation (World) */
	DRIVER( cninjau )	/* (c) 1991 Data East Corporation (US) */
	DRIVER( joemac )	/* (c) 1991 Data East Corporation (Japan) */
	DRIVER( stoneage )	/* bootleg */
	DRIVER( tumblep )	/* 1991 bootleg */
	DRIVER( tumblep2 )	/* 1991 bootleg */

	/* Tehkan / Tecmo games (Tehkan became Tecmo in 1986) */
	DRIVER( senjyo )	/* (c) 1983 Tehkan */
	DRIVER( starforc )	/* (c) 1984 Tehkan */
	DRIVER( starfore )	/* (c) 1984 Tehkan */
	DRIVER( megaforc )	/* (c) 1985 Tehkan + Video Ware license */
	DRIVER( bombjack )	/* (c) 1984 Tehkan */
	DRIVER( bombjac2 )	/* (c) 1984 Tehkan */
	DRIVER( pbaction )	/* (c) 1985 Tehkan */
	DRIVER( pbactio2 )	/* (c) 1985 Tehkan */
	DRIVER( pontoon )	/* 6011 - (c) 1985 Tehkan */
	DRIVER( tehkanwc )	/* (c) 1985 Tehkan */
	DRIVER( gridiron )	/* (c) 1985 Tehkan */
	DRIVER( teedoff )	/* 6102 - (c) 1986 Tecmo */
	DRIVER( solomon )	/* (c) 1986 Tecmo */
	DRIVER( rygar )		/* 6002 - (c) 1986 Tecmo */
	DRIVER( rygar2 )	/* 6002 - (c) 1986 Tecmo */
	DRIVER( rygarj )	/* 6002 - (c) 1986 Tecmo */
	DRIVER( gemini )	/* (c) 1987 Tecmo */
	DRIVER( silkworm )	/* 6217 - (c) 1988 Tecmo */
	DRIVER( silkwrm2 )	/* 6217 - (c) 1988 Tecmo */
	DRIVER( gaiden )	/* 6215 - (c) 1988 Tecmo */
	DRIVER( shadoww )	/* 6215 - (c) 1988 Tecmo */
	DRIVER( tknight )	/* (c) 1989 Tecmo */
	DRIVER( wildfang )	/* (c) 1989 Tecmo */
	DRIVER( wc90 )		/* (c) 1989 Tecmo */
	DRIVER( wc90b )		/* bootleg */

/* Other Tehkan games:
6009 Tank Busters
*/

	/* Konami bitmap games */
	DRIVER( tutankhm )	/* GX350 (c) 1982 Konami */
	DRIVER( tutankst )	/* GX350 (c) 1982 Stern */
	DRIVER( junofrst )	/* GX310 (c) 1983 Konami */

	/* Konami games */
	DRIVER( pooyan )	/* GX320 (c) 1982 */
	DRIVER( pooyans )	/* GX320 (c) 1982 Stern */
	DRIVER( pootan )	/* bootleg */
	DRIVER( timeplt )	/* GX393 (c) 1982 */
	DRIVER( timepltc )	/* GX393 (c) 1982 + Centuri license*/
	DRIVER( spaceplt )	/* bootleg */
	DRIVER( megazone )	/* GX319 (c) 1983 + Kosuka */
	DRIVER( rocnrope )	/* GX364 (c) 1983 + Kosuka */
	DRIVER( ropeman )	/* bootleg */
	DRIVER( gyruss )	/* GX347 (c) 1983 */
	DRIVER( gyrussce )	/* GX347 (c) 1983 + Centuri license */
	DRIVER( venus )		/* bootleg */
	DRIVER( trackfld )	/* GX361 (c) 1983 */
	DRIVER( trackflc )	/* GX361 (c) 1983 + Centuri license */
	DRIVER( hyprolym )	/* GX361 (c) 1983 */
	DRIVER( hyprolyb )	/* bootleg */
	DRIVER( circusc )	/* GX380 (c) 1984 */
	DRIVER( circusc2 )	/* GX380 (c) 1984 */
	DRIVER( circuscc )	/* GX380 (c) 1984 + Centuri license */
	DRIVER( tp84 )		/* GX388 (c) 1984 */
	DRIVER( tp84a )		/* GX388 (c) 1984 */
	DRIVER( hyperspt )	/* GX330 (c) 1984 + Centuri */
	DRIVER( sbasketb )	/* GX405 (c) 1984 */
	DRIVER( mikie )		/* GX469 (c) 1984 */
	DRIVER( mikiej )	/* GX469 (c) 1984 */
	DRIVER( mikiehs )	/* GX469 (c) 1984 */
	DRIVER( roadf )		/* GX461 (c) 1984 */
	DRIVER( roadf2 )	/* GX461 (c) 1984 */
	DRIVER( yiear )		/* GX407 (c) 1985 */
	DRIVER( yiear2 )	/* GX407 (c) 1985 */
	DRIVER( kicker )	/* GX477 (c) 1985 */
	DRIVER( shaolins )	/* GX477 (c) 1985 */
	DRIVER( pingpong )	/* GX555 (c) 1985 */
	DRIVER( gberet )	/* GX577 (c) 1985 */
	DRIVER( rushatck )	/* GX577 (c) 1985 */
	DRIVER( gberetb )	/* bootleg on different hardware */
	DRIVER( jailbrek )	/* GX507 (c) 1986 */
	DRIVER( ironhors )	/* GX560 (c) 1986 */
	DRIVER( farwest )
	DRIVER( jackal )	/* GX631 (c) 1986 */
	DRIVER( topgunr )	/* GX631 (c) 1986 */
	DRIVER( topgunbl )	/* bootleg */
	DRIVER( ddrible )	/* GX690 (c) 1986 */
	DRIVER( contra )	/* GX633 (c) 1987 */
	DRIVER( contrab )	/* bootleg */
	DRIVER( gryzorb )	/* bootleg */
	DRIVER( mainevt )	/* GX799 (c) 1988 */
	DRIVER( mainevt2 )	/* GX799 (c) 1988 */
	DRIVER( devstors )	/* GX890 (c) 1988 */
	DRIVER( combasc )	/* GX611 (c) 1988 */
	DRIVER( combascb )	/* bootleg */

	/* Konami "Nemesis hardware" games */
	DRIVER( nemesis )	/* GX456 (c) 1985 */
	DRIVER( nemesuk )	/* GX456 (c) 1985 */
	DRIVER( konamigt )	/* GX561 (c) 1985 */
	DRIVER( salamand )	/* GX587 (c) 1986 */
	DRIVER( lifefrce )	/* GX587 (c) 1986 */
	DRIVER( lifefrcj )	/* GX587 (c) 1986 */
	/* GX400 BIOS based games */
	DRIVER( rf2 )		/* GX561 (c) 1985 */
	DRIVER( twinbee )	/* GX412 (c) 1985 */
	DRIVER( gradius )	/* GX456 (c) 1985 */
	DRIVER( gwarrior )	/* GX578 (c) 1985 */

	/* Konami "TMNT hardware" games */
	DRIVER( tmnt )		/* GX963 (c) 1989 */
	DRIVER( tmntj )		/* GX963 (c) 1989 */
	DRIVER( tmht2p )	/* GX963 (c) 1989 */
	DRIVER( tmnt2pj )	/* GX963 (c) 1990 */
	DRIVER( punkshot )	/* GX907 (c) 1990 */
	DRIVER( punksht2 )	/* GX907 (c) 1990 */

	/* Exidy games */
	DRIVER( sidetrac )	/* (c) 1979 */
	DRIVER( targ )		/* (c) 1980 */
	DRIVER( spectar )	/* (c) 1980 */
	DRIVER( spectar1 )	/* (c) 1980 */
	DRIVER( venture )	/* (c) 1981 */
	DRIVER( venture2 )	/* (c) 1981 */
	DRIVER( venture4 )	/* (c) 1981 */
	DRIVER( mtrap )		/* (c) 1981 */
	DRIVER( mtrap3 )	/* (c) 1981 */
	DRIVER( mtrap4 )	/* (c) 1981 */
	DRIVER( pepper2 )	/* (c) 1982 */
	DRIVER( hardhat )	/* (c) 1982 */
	DRIVER( fax )		/* (c) 1983 */
	DRIVER( circus )	/* no copyright notice [1977?] */
	DRIVER( robotbwl )	/* no copyright notice */
	DRIVER( crash )		/* Exidy [1979?] */
	DRIVER( ripcord )	/* Exidy [1977?] */
	DRIVER( starfire )	/* Exidy [1979?] */
	DRIVER( fireone )	/* (c) 1979 Exidy */

	/* Exidy 440 games */
	DRIVER( crossbow )	/* (c) 1983 */
	DRIVER( cheyenne )	/* (c) 1984 */
	DRIVER( combat )	/* (c) 1985 */
	DRIVER( cracksht )	/* (c) 1985 */
	DRIVER( claypign )	/* (c) 1986 */
	DRIVER( chiller )	/* (c) 1986 */
	DRIVER( topsecex )	/* (c) 1986 */
	DRIVER( hitnmiss )	/* (c) 1987 */
	DRIVER( hitnmis2 )	/* (c) 1987 */
	DRIVER( whodunit )	/* (c) 1988 */
	DRIVER( showdown )	/* (c) 1988 */

	/* Atari vector games */
	DRIVER( asteroid )	/* (c) 1979 */
	DRIVER( asteroi1 )	/* no copyright notice */
	DRIVER( astdelux )	/* (c) 1980 */
	DRIVER( astdelu1 )	/* (c) 1980 */
	DRIVER( bwidow )	/* (c) 1982 */
	DRIVER( bzone )		/* (c) 1980 */
	DRIVER( bzone2 )	/* (c) 1980 */
	DRIVER( gravitar )	/* (c) 1982 */
	DRIVER( gravitr2 )	/* (c) 1982 */
	DRIVER( llander )	/* no copyright notice */
	DRIVER( llander1 )	/* no copyright notice */
	DRIVER( redbaron )	/* (c) 1980 */
	DRIVER( spacduel )	/* (c) 1980 */
	DRIVER( tempest )	/* (c) 1980 */
	DRIVER( tempest1 )	/* (c) 1980 */
	DRIVER( tempest2 )	/* (c) 1980 */
	DRIVER( temptube )	/* hack */
	DRIVER( starwars )	/* (c) 1983 */
	DRIVER( starwar1 )	/* (c) 1983 */
	DRIVER( esb )		/* (c) 1985 */
	DRIVER( mhavoc )	/* (c) 1983 */
	DRIVER( mhavoc2 )	/* (c) 1983 */
	DRIVER( mhavocp )	/* (c) 1983 */
	DRIVER( mhavocrv )	/* hack */
	DRIVER( quantum )	/* (c) 1982 */	/* made by Gencomp */
	DRIVER( quantum1 )	/* (c) 1982 */	/* made by Gencomp */
	DRIVER( quantump )	/* (c) 1982 */	/* made by Gencomp */

	/* Atari "Centipede hardware" games */
	DRIVER( warlord )	/* (c) 1980 */
	DRIVER( centiped )	/* (c) 1980 */
	DRIVER( centipd2 )	/* (c) 1980 */
	DRIVER( centipdb )	/* bootleg */
	DRIVER( milliped )	/* (c) 1982 */
	DRIVER( qwakprot )	/* (c) 1982 */

	/* Atari "Kangaroo hardware" games */
	DRIVER( kangaroo )	/* (c) 1982 */
	DRIVER( kangarob )	/* bootleg */
	DRIVER( arabian )	/* (c) 1983 Sun Electronics */
	DRIVER( arabiana )	/* (c) 1983 */

	/* Atari "Missile Command hardware" games */
	DRIVER( missile )	/* (c) 1980 */
	DRIVER( missile2 )	/* (c) 1980 */
	DRIVER( suprmatk )	/* (c) 1980 + (c) 1981 Gencomp */

	/* Atari b/w games */
	DRIVER( sprint1 )	/* no copyright notice */
	DRIVER( sprint2 )	/* no copyright notice */
	DRIVER( sbrkout )	/* no copyright notice */
	DRIVER( dominos )	/* no copyright notice */
	DRIVER( nitedrvr )	/* no copyright notice [1976] */
	DRIVER( bsktball )	/* no copyright notice */
	DRIVER( copsnrob )	/* [1976] */
	DRIVER( avalnche )	/* no copyright notice [1978] */
	DRIVER( subs )		/* no copyright notice [1976] */
	DRIVER( atarifb )	/* no copyright notice [1978] */
	DRIVER( atarifb1 )	/* no copyright notice [1978] */
	DRIVER( atarifb4 )	/* no copyright notice [1979] */
	DRIVER( abaseb )	/* no copyright notice [1979] */
	DRIVER( abaseb2 )	/* no copyright notice [1979] */
	DRIVER( canyon )	/* no copyright notice [1977] */
	DRIVER( canbprot )	/* no copyright notice [1977] */
	DRIVER( skydiver )	/* no copyright notice [1977] */

	/* misc Atari games */
//	DRIVER( polepos )
	DRIVER( foodf )		/* (c) 1982 */	/* made by Gencomp */
	DRIVER( liberatr )	/* (c) 1982 */
	DRIVER( ccastles )	/* (c) 1983 */
	DRIVER( ccastle2 )	/* (c) 1983 */
	DRIVER( cloak )		/* (c) 1983 */
	DRIVER( cloud9 )	/* (c) 1983 */
	DRIVER( jedi )		/* (c) 1984 */

	/* Atari System 1 games */
	DRIVER( marble )	/* (c) 1984 */
	DRIVER( marble2 )	/* (c) 1984 */
	DRIVER( marblea )	/* (c) 1984 */
	DRIVER( peterpak )	/* (c) 1984 */
	DRIVER( indytemp )	/* (c) 1985 */
	DRIVER( indytem2 )	/* (c) 1985 */
	DRIVER( indytem3 )	/* (c) 1985 */
	DRIVER( roadrunn )	/* (c) 1985 */
	DRIVER( roadblst )	/* (c) 1986, 1987 */

	/* Atari System 2 games */
	DRIVER( paperboy )	/* (c) 1984 */
	DRIVER( ssprint )	/* (c) 1986 */
	DRIVER( csprint )	/* (c) 1986 */
	DRIVER( a720 )		/* (c) 1986 */
	DRIVER( a720b )		/* (c) 1986 */
	DRIVER( apb )		/* (c) 1987 */
	DRIVER( apb2 )		/* (c) 1987 */

	/* later Atari games */
	DRIVER( gauntlet )	/* (c) 1985 */
	DRIVER( gauntir1 )	/* (c) 1985 */
	DRIVER( gauntir2 )	/* (c) 1985 */
	DRIVER( gaunt2p )	/* (c) 1985 */
	DRIVER( gaunt2 )	/* (c) 1986 */
	DRIVER( vindctr2 )	/* (c) 1988 */
	DRIVER( atetris )	/* (c) 1988 */
	DRIVER( atetrisa )	/* (c) 1988 */
	DRIVER( atetrisb )	/* bootleg */
	DRIVER( atetcktl )	/* (c) 1989 */
	DRIVER( atetckt2 )	/* (c) 1989 */
	DRIVER( toobin )	/* (c) 1988 */
	DRIVER( vindictr )	/* (c) 1988 */
	DRIVER( klax )		/* (c) 1989 */
	DRIVER( klax2 )		/* (c) 1989 */
	DRIVER( klax3 )		/* (c) 1989 */
	DRIVER( klaxj )		/* (c) 1989 (Japan) */
	DRIVER( blstroid )	/* (c) 1987 */
	DRIVER( blstroi2 )	/* (c) 1987 */
	DRIVER( xybots )	/* (c) 1987 */
	DRIVER( eprom )		/* (c) 1989 */
	DRIVER( eprom2 )	/* (c) 1989 */
	DRIVER( skullxbo )	/* (c) 1989 */
	DRIVER( skullxb2 )	/* (c) 1989 */
	DRIVER( badlands )	/* (c) 1989 */
	DRIVER( cyberb2p )	/* (c) 1989 */
	DRIVER( rampart )	/* (c) 1990 */
	DRIVER( ramprt2p )	/* (c) 1990 */
	DRIVER( shuuz )		/* (c) 1990 */
	DRIVER( shuuz2 )	/* (c) 1990 */

	/* SNK / Rock-ola games */
	DRIVER( sasuke )	/* [1980] Shin Nihon Kikaku (SNK) */
	DRIVER( satansat )	/* (c) 1981 SNK */
	DRIVER( zarzon )	/* (c) 1981 Taito, gameplay says SNK */
	DRIVER( vanguard )	/* (c) 1981 SNK */
	DRIVER( vangrdce )	/* (c) 1981 SNK + Centuri */
	DRIVER( fantasy )	/* (c) 1981 Rock-ola */
	DRIVER( pballoon )	/* (c) 1982 SNK */
	DRIVER( nibbler )	/* (c) 1982 Rock-ola */
	DRIVER( nibblera )	/* (c) 1982 Rock-ola */

	/* later SNK games */
	DRIVER( tnk3 )		/* (c) 1985 */
	DRIVER( tnk3j )		/* (c) 1985 */
	DRIVER( aso )		/* (c) 1985 */
	DRIVER( athena )	/* (c) 1986 */
	DRIVER( fitegolf )	/* (c) 1988 */
	DRIVER( ikari )		/* (c) 1986 */
	DRIVER( ikarijp )	/* (c) 1986 */
	DRIVER( ikarijpb )	/* bootleg */
	DRIVER( victroad )	/* (c) 1986 */
	DRIVER( dogosoke )	/* (c) 1986 */
	DRIVER( gwar )		/* (c) 1987 */
	DRIVER( bermudat )	/* (c) 1987 */
	DRIVER( psychos )	/* (c) 1987 */
	DRIVER( psychosa )	/* (c) 1987 */
	DRIVER( chopper )	/* (c) 1988 */
	DRIVER( legofair )	/* (c) 1988 */
//	DRIVER( ftsoccer )	/* (c) 1988 */
	DRIVER( tdfever )	/* (c) 1987 */
	DRIVER( tdfeverj )	/* (c) 1987 */
	DRIVER( pow )		/* (c) 1988 */
	DRIVER( powj )		/* (c) 1988 */
	DRIVER( searchar )	/* (c) 1989 */
	DRIVER( streetsm )	/* (c) 1989 */
	DRIVER( streets2 )	/* (c) 1989 */
	DRIVER( streetsj )	/* (c) 1989 */
	DRIVER( prehisle )	/* (c) 1989 */
	DRIVER( prehislu )	/* (c) 1989 */
	DRIVER( prehislj )	/* (c) 1989 */

	/* Technos games */
	DRIVER( mystston )	/* TA-0010 (c) 1984 */
	DRIVER( matmania )	/* TA-0015 (c) 1985 + Taito America license */
	DRIVER( excthour )	/* TA-0015 (c) 1985 + Taito license */
	DRIVER( maniach )	/* TA-???? (c) 1986 + Taito America license */
	DRIVER( maniach2 )	/* TA-???? (c) 1986 + Taito America license */
	DRIVER( renegade )	/* TA-0018 (c) 1986 + Taito America license */
	DRIVER( kuniokub )	/* TA-0018 bootleg */
	DRIVER( xsleena )	/* TA-0019 (c) 1986 */
	DRIVER( xsleenab )	/* bootleg */
	DRIVER( solarwar )	/* TA-0019 (c) 1986 Taito + Memetron license */
	DRIVER( battlane )	/* TA-???? (c) 1986 + Taito license */
	DRIVER( battlan2 )	/* TA-???? (c) 1986 + Taito license */
	DRIVER( battlan3 )	/* TA-???? (c) 1986 + Taito license */
	DRIVER( ddragon )
	DRIVER( ddragonb )	/* TA-0021 bootleg */
	/* TA-0022 Super Dodge Ball */
	/* TA-0023 China Gate */
	/* TA-0024 WWF Superstars */
	/* TA-0025 Champ V'Ball */
	DRIVER( ddragon2 )	/* TA-0026 (c) 1988 */
	/* TA-0028 Combatribes */
	DRIVER( blockout )	/* TA-0029 (c) 1989 + California Dreams */
	DRIVER( blckout2 )	/* TA-0029 (c) 1989 + California Dreams */
	/* TA-0030 Double Dragon 3 */
	/* TA-0031 WWF Wrestlefest */

	/* Stern "Berzerk hardware" games */
	DRIVER( berzerk )	/* (c) 1980 */
	DRIVER( berzerk1 )	/* (c) 1980 */
	DRIVER( frenzy )	/* (c) 1982 */

	/* GamePlan games */
	DRIVER( megatack )	/* (c) 1980 Centuri */
	DRIVER( killcom )	/* (c) 1980 Centuri */
	DRIVER( challeng )	/* (c) 1981 Centuri */
	DRIVER( kaos )		/* (c) 1981 */

	/* "stratovox hardware" games */
	DRIVER( route16 )	/* (c) 1981 Tehkan/Sun + Centuri license */
	DRIVER( route16b )	/* bootleg */
	DRIVER( stratvox )	/* Taito */
	DRIVER( stratvxb )	/* bootleg */
	DRIVER( speakres )	/* no copyright notice */

	/* Zaccaria games */
	DRIVER( monymony )	/* (c) 1983 */
	DRIVER( jackrabt )	/* (c) 1984 */
	DRIVER( jackrab2 )	/* (c) 1984 */
	DRIVER( jackrabs )	/* (c) 1984 */

	/* UPL games */
	DRIVER( nova2001 )	/* (c) [1984?] + Universal license */
	DRIVER( pkunwar )	/* [1985?] */
	DRIVER( pkunwarj )	/* [1985?] */
	DRIVER( ninjakd2 )	/* (c) 1987 */
	DRIVER( ninjak2a )	/* (c) 1987 */
	DRIVER( ninjak2b )	/* (c) 1987 */
	DRIVER( rdaction )	/* (c) 1987 + World Games license */
	DRIVER( mnight )	/* (c) 1987 distributed by Kawakus */
/*
other UPL games:

   83 Mouser                              Kit 2P              Action
8/87 Mission XX                          Kit 2P  8W+2B   VC  Shooter
   88 Aquaria                             Kit 2P  8W+2B
   89 Ochichi Mahjong                     Kit 2P  8W+2B   HC  Mahjong
9/89 Omega Fighter        American Sammy Kit 2P  8W+2B   HC  Shooter
12/89 Task Force Harrier   American Sammy Kit 2P  8W+2B   VC  Shooter
   90 Atomic Robo-Kid      American Sammy Kit 2P  8W+2B   HC  Shooter
   90 Mustang - U.S.A.A.F./Fire Mustang   Kit 2P  8W+2B   HC  Shooter
   91 Acrobat Mission               Taito Kit 2P  8W+2B   VC  Shooter
   91 Bio Ship Paladin/Spaceship Gomera   Kit 2P  8W+2B   HC  Shooter
   91 Black Heart                         Kit 2P  8W+2B   HC  Shooter
   91 Spaceship Gomera/Bio Ship Paladin   Kit 2P  8W+2B   HC  Shooter
   91 Van Dyke Fantasy                    Kit 2P  8W+2B
2/92 Strahl                              Kit 2P  8W+3B

board numbers:
Atomic Robo Kid     UPL-88013
Omega Fighter       UPL-89016
Task Force Harrier  UPL-89050
USAAF Mustang       UPL-90058
Bio Ship Paladin    UPL-90062

*/

	/* Williams/Midway TMS34010 games */
	DRIVER( narc )		/* (c) 1988 Williams */
	DRIVER( trog )		/* (c) 1990 Midway */
	DRIVER( trog3 )		/* (c) 1990 Midway */
	DRIVER( trogp )		/* (c) 1990 Midway */
	DRIVER( smashtv )	/* (c) 1990 Williams */
	DRIVER( smashtv6 )	/* (c) 1990 Williams */
	DRIVER( smashtv5 )	/* (c) 1990 Williams */
	DRIVER( smashtv4 )	/* (c) 1990 Williams */
	DRIVER( hiimpact )	/* (c) 1990 Williams */
	DRIVER( shimpact )	/* (c) 1991 Midway */
	DRIVER( strkforc )	/* (c) 1991 Midway */
	DRIVER( mk )		/* (c) 1992 Midway */
	DRIVER( mkla1 )		/* (c) 1992 Midway */
	DRIVER( mkla2 )		/* (c) 1992 Midway */
	DRIVER( term2 )		/* (c) 1992 Midway */
	DRIVER( totcarn )	/* (c) 1992 Midway */
	DRIVER( totcarnp )	/* (c) 1992 Midway */
	DRIVER( mk2 )		/* (c) 1993 Midway */
	DRIVER( mk2r32 )	/* (c) 1993 Midway */
	DRIVER( mk2r14 )	/* (c) 1993 Midway */
	DRIVER( nbajam )	/* (c) 1993 Midway */

	/* Cinematronics raster games */
	DRIVER( jack )		/* (c) 1982 Cinematronics */
	DRIVER( jack2 )		/* (c) 1982 Cinematronics */
	DRIVER( jack3 )		/* (c) 1982 Cinematronics */
	DRIVER( treahunt )	/* (c) 1982 Hara Ind. */
	DRIVER( zzyzzyxx )	/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( zzyzzyx2 )	/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( brix )		/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( freeze )	/* Cinematronics */
	DRIVER( sucasino )	/* (c) 1982 Data Amusement */

	/* Cinematronics vector games */
	DRIVER( spacewar )
	DRIVER( barrier )
	DRIVER( starcas )	/* (c) 1980 */
	DRIVER( starcas1 )	/* (c) 1980 */
	DRIVER( tailg )
	DRIVER( ripoff )
	DRIVER( armora )
	DRIVER( wotw )
	DRIVER( warrior )
	DRIVER( starhawk )
	DRIVER( solarq )	/* (c) 1981 */
	DRIVER( boxingb )	/* (c) 1981 */
	DRIVER( speedfrk )
	DRIVER( sundance )
	DRIVER( demon )		/* (c) 1982 Rock-ola */

	/* "The Pit hardware" games */
	DRIVER( roundup )	/* (c) 1981 Amenip/Centuri */
	DRIVER( fitter )	/* (c) 1981 Taito */
	DRIVER( thepit )	/* (c) 1982 Centuri */
	DRIVER( intrepid )	/* (c) 1983 Nova Games Ltd. */
	DRIVER( intrepi2 )	/* (c) 1983 Nova Games Ltd. */
	DRIVER( portman )	/* (c) 1982 Nova Games Ltd. */
	DRIVER( suprmous )	/* (c) 1982 Taito */
	DRIVER( suprmou2 )	/* (c) 1982 Chu Co. Ltd. */
	DRIVER( machomou )	/* (c) 1982 Techstar */

	/* Valadon Automation games */
	DRIVER( bagman )	/* (c) 1982 */
	DRIVER( bagnard )	/* (c) 1982 */
	DRIVER( bagmans )	/* (c) 1982 + Stern license */
	DRIVER( bagmans2 )	/* (c) 1982 + Stern license */
	DRIVER( sbagman )	/* (c) 1984 */
	DRIVER( sbagmans )	/* (c) 1984 + Stern license */
	DRIVER( pickin )	/* (c) 1983 */

	/* Seibu Denshi / Seibu Kaihatsu games */
	DRIVER( stinger )	/* (c) 1983 Seibu Denshi */
	DRIVER( scion )		/* (c) 1984 Seibu Denshi */
	DRIVER( scionc )	/* (c) 1984 Seibu Denshi + Cinematronics license */
	DRIVER( wiz )		/* (c) 1985 Seibu Kaihatsu */

	/* Jaleco games */
	DRIVER( exerion )	/* (c) 1983 Jaleco */
	DRIVER( exeriont )	/* (c) 1983 Jaleco + Taito America license */
	DRIVER( exerionb )	/* bootleg */
	DRIVER( formatz )	/* (c) 1984 Jaleco */
	DRIVER( aeroboto )	/* (c) 1984 Williams */
	DRIVER( citycon )	/* (c) 1985 Jaleco */
	DRIVER( citycona )	/* (c) 1985 Jaleco */
	DRIVER( cruisin )	/* (c) 1985 Jaleco/Kitkorp */
	DRIVER( psychic5 )	/* (c) 1987 Jaleco */

	/* Jaleco Mega System 1 games */
	DRIVER( lomakaj )	/* (c) 1988 */
	DRIVER( p47 )		/* (c) 1988 */
	DRIVER( p47j )		/* (c) 1988 */
	DRIVER( street64 )	/* (c) 1991 */
	DRIVER( edf )		/* (c) 1991 */
	DRIVER( rodlandj )	/* (c) 1990 */
	DRIVER( avspirit )	/* (c) 1991 */
//	DRIVER( astyanax )
//	DRIVER( hachoo )
//	DRIVER( plusalph )
//	DRIVER( phantasm )
//	DRIVER( stdragon )

	/* Video System Co. games */
	DRIVER( pspikes )	/* (c) 1991 */
	DRIVER( turbofrc )	/* (c) 1991 */
	DRIVER( aerofgt )	/* (c) 1992 */
	DRIVER( aerofgtb )	/* (c) 1992 */
	DRIVER( aerofgtc )	/* (c) 1992 */

	/* Tad games */
	DRIVER( cabal )		/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabal2 )	/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabalbl )	/* bootleg */
	DRIVER( toki )		/* (c) 1989 Tad */
	DRIVER( toki2 )		/* (c) 1989 Tad */
	DRIVER( toki3 )		/* (c) 1989 Tad */
	DRIVER( tokiu )		/* (c) 1989 Tad + Fabtek license */
	DRIVER( tokib )		/* bootleg */

	/* Orca games */
	DRIVER( marineb )	/* (c) 1982 Orca */
	DRIVER( changes )	/* (c) 1982 Orca */
	DRIVER( springer )	/* (c) 1982 Orca */
	DRIVER( hoccer )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( hoccer2 )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( wanted )	/* (c) 1984 Sigma Ent. Inc. */
	DRIVER( funkybee )	/* (c) 1982 Orca */
	DRIVER( skylancr )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( zodiack )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( dogfight )	/* (c) 1983 Thunderbolt */
	DRIVER( moguchan )	/* (c) 1982 Orca + Eastern Commerce Inc. license (doesn't appear on screen) */
	DRIVER( espial )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */
	DRIVER( espiale )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */

	DRIVER( spacefb )	/* (c) [1980?] Nintendo */
	DRIVER( spacefbg )	/* 834-0031 (c) 1980 Gremlin */
	DRIVER( spacebrd )	/* bootleg */
	DRIVER( spacedem )	/* (c) 1980 Nintendo / Fortrek */
	DRIVER( blueprnt )	/* (c) 1982 Bally Midway (Zilec in ROM 3U, and the programmer names) */
	DRIVER( saturn )	/* (c) 1983 Jaleco (Zilec in ROM R6, and the programmer names) */
	DRIVER( omegrace )	/* (c) 1981 Midway */
	DRIVER( vastar )	/* (c) 1983 Sesame Japan */
	DRIVER( vastar2 )	/* (c) 1983 Sesame Japan */
	DRIVER( tankbatt )	/* (c) 1980 Namco */
	DRIVER( dday )		/* (c) 1982 Olympia */
	DRIVER( ddayc )		/* (c) 1982 Olympia + Centuri license */
	DRIVER( gundealr )	/* (c) 1990 Dooyong */
	DRIVER( gundeala )	/* (c) Dooyong */
	DRIVER( yamyam )	/* (c) 1990 Dooyong */
	DRIVER( wiseguy )	/* (c) 1990 Dooyong */
	DRIVER( leprechn )	/* (c) 1982 Tong Electronic */
	DRIVER( potogold )	/* (c) 1982 Tong Electronic */
	DRIVER( hexa )		/* D. R. Korea */
	DRIVER( redalert )	/* (c) 1981 Irem (GDI game) */
	DRIVER( irobot )
	DRIVER( spiders )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( spiders2 )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( stactics )	/* [1981 Sega] */
	DRIVER( goldstar )
	DRIVER( goldstbl )
	DRIVER( exterm )	/* (c) 1989 Premier Technology - a Gottlieb game */
	DRIVER( sharkatt )	/* (c) Pacific Novelty */
	DRIVER( kingofb )	/* (c) 1985 Woodplace Inc. */
	DRIVER( ringking )	/* (c) 1985 Data East USA */
	DRIVER( ringkin2 )
	DRIVER( ringkin3 )	/* (c) 1985 Data East USA */
	DRIVER( zerozone )	/* (c) 1993 Comad */
	DRIVER( exctsccr )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( exctscca )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( exctsccb )	/* bootleg */
	DRIVER( speedbal )	/* (c) 1987 Tecfri */
	DRIVER( sauro )		/* (c) 1987 Tecfri */
	DRIVER( galpanic )	/* (c) 1990 Kaneko */
	DRIVER( airbustr )	/* (c) 1990 Kaneko */
	DRIVER( ambush )	/* (c) 1983 Nippon Amuse Co-Ltd */
	DRIVER( starcrus )	/* [1977 Ramtek] */
	DRIVER( sichuan2 )	/* (c) 1989 Tamtex */
	DRIVER( shisen )	/* (c) 1989 Tamtex */
//	DRIVER( shanghai )	/* (c) 1988 Sun Electronics */
	DRIVER( goindol )	/* (c) 1987 Sun a Electronics */
	DRIVER( homo )		/* bootleg */
//	DRIVER( dlair )

#endif /* NEOMAME */

#ifndef NEOFREE

	/* Neo Geo games */
	/* the four digits number is the game ID stored at address 0x0108 of the program ROM */
	DRIVER( nam1975 )	/* 0001 (c) 1990 SNK */
	DRIVER( bstars )	/* 0002 (c) 1990 SNK */
	DRIVER( tpgolf )	/* 0003 (c) 1990 SNK */
	DRIVER( mahretsu )	/* 0004 (c) 1990 SNK */
	DRIVER( maglord )	/* 0005 (c) 1990 Alpha Denshi Co */
	DRIVER( maglordh )	/* 0005 (c) 1990 Alpha Denshi Co */
	DRIVER( ridhero )	/* 0006 (c) 1990 SNK */
	DRIVER( alpham2 )	/* 0007 (c) 1991 SNK */
	/* 0008 */
	DRIVER( ncombat )	/* 0009 (c) 1990 Alpha Denshi Co */
	DRIVER( cyberlip )	/* 0010 (c) 1990 SNK */
	DRIVER( superspy )	/* 0011 (c) 1990 SNK */
	/* 0012 */
	/* 0013 */
	DRIVER( mutnat )	/* 0014 (c) 1992 SNK */
	/* 0015 */
	DRIVER( kotm )		/* 0016 (c) 1991 SNK */
	DRIVER( sengoku )	/* 0017 (c) 1991 SNK */
	DRIVER( burningf )	/* 0018 (c) 1991 SNK */
	DRIVER( lbowling )	/* 0019 (c) 1990 SNK */
	DRIVER( gpilots )	/* 0020 (c) 1991 SNK */
	DRIVER( joyjoy )	/* 0021 (c) 1990 SNK */
	DRIVER( bjourney )	/* 0022 (c) 1990 Alpha Denshi Co */
	DRIVER( quizdais )	/* 0023 (c) 1991 SNK */
	DRIVER( lresort )	/* 0024 (c) 1992 SNK */
	DRIVER( eightman )	/* 0025 (c) 1991 SNK / Pallas */
	/* 0026 Fun Fun Brothers - prototype? */
	DRIVER( minasan )	/* 0027 (c) 1990 Monolith Corp. */
	/* 0028 */
	DRIVER( legendos )	/* 0029 (c) 1991 SNK */
	DRIVER( ttbb )		/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( socbrawl )	/* 0031 (c) 1991 SNK */
	DRIVER( roboarmy )	/* 0032 (c) 1991 SNK */
	DRIVER( fatfury1 )	/* 0033 (c) 1991 SNK */
	DRIVER( fbfrenzy )	/* 0034 (c) 1992 SNK */
	/* 0035 */
	DRIVER( bakatono )	/* 0036 (c) 1991 Monolith Corp. */
	DRIVER( crsword )	/* 0037 (c) 1991 Alpha Denshi Co */
	DRIVER( trally )	/* 0038 (c) 1991 Alpha Denshi Co */
	DRIVER( kotm2 )		/* 0039 (c) 1992 SNK */
	DRIVER( sengoku2 )	/* 0040 (c) 1993 SNK */
	DRIVER( bstars2 )	/* 0041 (c) 1992 SNK */
	DRIVER( quizdai2 )	/* 0042 (c) 1992 SNK */
	DRIVER( countb )	/* 0043 (c) 1993 SNK */
	DRIVER( aof )		/* 0044 (c) 1992 SNK */
	DRIVER( samsho )	/* 0045 (c) 1993 SNK */
	DRIVER( tophuntr )	/* 0046 (c) 1994 SNK */
	DRIVER( fatfury2 )	/* 0047 (c) 1992 SNK */
	DRIVER( janshin )	/* 0048 (c) 1994 Aicom */
	DRIVER( androdun )	/* 0049 (c) 1992 Visco */
	DRIVER( ncommand )	/* 0050 (c) 1992 Alpha Denshi Co */
	DRIVER( viewpoin )	/* 0051 (c) 1992 Sammy */
	DRIVER( ssideki )	/* 0052 (c) 1992 SNK */
	DRIVER( wh1 )		/* 0053 (c) 1992 Alpha Denshi Co */
	/* 0054 Crossed Swords 2 (CD only) */
	DRIVER( kof94 )		/* 0055 (c) 1994 SNK */
	DRIVER( aof2 )		/* 0056 (c) 1994 SNK */
	DRIVER( wh2 )		/* 0057 (c) 1993 ADK */
	DRIVER( fatfursp )	/* 0058 (c) 1993 SNK */
	DRIVER( savagere )	/* 0059 (c) 1995 SNK */
	DRIVER( fightfev )	/* 0060 (c) 1994 Viccom */
	DRIVER( ssideki2 )	/* 0061 (c) 1994 SNK */
	DRIVER( spinmast )	/* 0062 (c) 1993 Data East Corporation */
	DRIVER( samsho2 )	/* 0063 (c) 1994 SNK */
	DRIVER( wh2j )		/* 0064 (c) 1994 ADK / SNK */
	DRIVER( wjammers )	/* 0065 (c) 1994 Data East Corporation */
	DRIVER( karnovr )	/* 0066 (c) 1994 Data East Corporation */
	DRIVER( gururin )	/* 0067 (c) 1994 Face */
	DRIVER( pspikes2 )	/* 0068 (c) 1994 Video System Co. */
	DRIVER( fatfury3 )	/* 0069 (c) 1995 SNK */
	/* 0070 */
	/* 0071 */
	/* 0072 */
	DRIVER( panicbom )	/* 0073 (c) 1994 Eighting / Hudson */
	DRIVER( aodk )		/* 0074 (c) 1994 ADK / SNK */
	DRIVER( sonicwi2 )	/* 0075 (c) 1994 Video System Co. */
	DRIVER( zedblade )	/* 0076 (c) 1994 NMK */
	/* 0077 */
	DRIVER( galaxyfg )	/* 0078 (c) 1995 Sunsoft */
	DRIVER( strhoop )	/* 0079 (c) 1994 Data East Corporation */
	DRIVER( quizkof )	/* 0080 (c) 1995 Saurus */
	DRIVER( ssideki3 )	/* 0081 (c) 1995 SNK */
	DRIVER( doubledr )	/* 0082 (c) 1995 Technos */
	DRIVER( pbobble )	/* 0083 (c) 1994 Taito */
	DRIVER( kof95 )		/* 0084 (c) 1995 SNK */
	/* 0085 Shinsetsu Samurai Spirits Bushidoretsuden / Samurai Shodown RPG (CD only) */
	DRIVER( tws96 )		/* 0086 (c) 1996 Tecmo */
	DRIVER( samsho3 )	/* 0087 (c) 1995 SNK */
	DRIVER( stakwin )	/* 0088 (c) 1995 Saurus */
	DRIVER( pulstar )	/* 0089 (c) 1995 Aicom */
	DRIVER( whp )		/* 0090 (c) 1995 ADK / SNK */
	/* 0091 */
	DRIVER( kabukikl )	/* 0092 (c) 1995 Hudson */
	DRIVER( neobombe )	/* 0093 (c) 1997 Hudson */
	DRIVER( gowcaizr )	/* 0094 (c) 1995 Technos */
	DRIVER( rbff1 )		/* 0095 (c) 1995 SNK */
	DRIVER( aof3 )		/* 0096 (c) 1996 SNK */
	DRIVER( sonicwi3 )	/* 0097 (c) 1995 Video System Co. */
	/* 0098 Idol Mahjong - final romance 2 (CD only? not confirmed, MVS might exist) */
	/* 0099 */
	DRIVER( turfmast )	/* 0200 (c) 1996 Nazca */
	DRIVER( mslug )		/* 0201 (c) 1996 Nazca */
	DRIVER( puzzledp )	/* 0202 (c) 1995 Taito (Visco license) */
	/* 0203 Master of Syougi / Syougi No Tatsujin */
	/* 0204 ADK World (CD only) */
	/* 0205 Neo-Geo CD Special (CD only) */
	DRIVER( marukodq )	/* 0206 (c) 1995 Takara */
	DRIVER( neomrdo )	/* 0207 (c) 1996 Visco */
	DRIVER( sdodgeb )	/* 0208 (c) 1996 Technos */
	DRIVER( goalx3 )	/* 0209 (c) 1995 Visco */
	/* 0210 */
	/* 0211 Oshidashi Zintrick (CD only? not confirmed, MVS might exist) */
	DRIVER( overtop )	/* 0212 (c) 1996 ADK */
	DRIVER( neodrift )	/* 0213 (c) 1996 Visco */
	DRIVER( kof96 )		/* 0214 (c) 1996 SNK */
	DRIVER( ssideki4 )	/* 0215 (c) 1996 SNK */
	DRIVER( kizuna )	/* 0216 (c) 1996 SNK */
	DRIVER( ninjamas )	/* 0217 (c) 1996 ADK / SNK */
	DRIVER( ragnagrd )	/* 0218 (c) 1996 Saurus */
	/* 0219 Futsal - pleasure goal 5-on-5 street soccer */
	/* 0220 Choutetsu Brikin'ger - iron clad (CD only? not confirmed, MVS might exist) */
	DRIVER( magdrop2 )	/* 0221 (c) 1996 Data East Corporation */
	DRIVER( samsho4 )	/* 0222 (c) 1996 SNK */
	DRIVER( rbffspec )	/* 0223 (c) 1996 SNK */
	DRIVER( twinspri )	/* 0224 (c) 1996 ADK */
	DRIVER( wakuwak7 )	/* 0225 (c) 1996 Sunsoft */
	/* 0226 */
	DRIVER( stakwin2 )	/* 0227 (c) 1996 Saurus */
	/* 0228 */
	/* 0229 King of Fighters '96 CD Collection (CD only) */
	DRIVER( breakers )	/* 0230 (c) 1996 Visco */
	DRIVER( miexchng )	/* 0231 (c) 1997 Face */
	DRIVER( kof97 )		/* 0232 (c) 1997 SNK */
	DRIVER( magdrop3 )	/* 0233 (c) 1997 Data East Corporation */
	DRIVER( lastblad )	/* 0234 (c) 1997 SNK */
	DRIVER( puzzldpr )	/* 0235 (c) 1997 Taito (Visco license) */
	DRIVER( irrmaze )	/* 0236 (c) 1997 SNK / Saurus */
	DRIVER( popbounc )	/* 0237 (c) 1997 Video System Co. */
	DRIVER( shocktro )	/* 0238 (c) 1997 Saurus */
	DRIVER( blazstar )	/* 0239 (c) 1998 Yumekobo */
	DRIVER( rbff2 )		/* 0240 (c) 1998 SNK */
	DRIVER( mslug2 )	/* 0241 (c) 1998 SNK */
	DRIVER( kof98 )		/* 0242 (c) 1998 SNK */
	DRIVER( lastbld2 )	/* 0243 (c) 1998 SNK */
	DRIVER( neocup98 )	/* 0244 (c) 1998 SNK */
	DRIVER( breakrev )	/* 0245 (c) 1998 Visco */
	DRIVER( shocktr2 )	/* 0246 (c) 1998 Saurus */
	DRIVER( flipshot )	/* 0247 (c) 1998 Visco */
//	DRIVER( pbobble2 )	/* 0248 (c) 1999 Taito (SNK license) */
//	DRIVER( ctomaday )	/* 0249 (c) 1999 Visco */
//	DRIVER( mslugx )	/* 0250 (c) 1999 SNK */

#endif	/* NEOFREE */

#endif	/* DRIVER_RECURSIVE */

#endif	/* TINY_COMPILE */
