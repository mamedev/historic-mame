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
#define TESTDRIVER(NAME) extern struct GameDriver NAME##_driver;
#include "driver.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &NAME##_driver,
#define TESTDRIVER(NAME)
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
	DRIVER( hunchbks )	/* (c) 1983 Century */
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
TESTDRIVER( skykid )

	/* Namco System 86 games */
	/* 85.4  Hopping MAPPY (sequel to MAPPY) */
	/* 86.5  Sky Kid DX (sequel to Sky Kid) */
	DRIVER( roishtar )	/* (c) 1986 */
	DRIVER( genpeitd )	/* (c) 1986 */
	DRIVER( rthunder )	/* (c) 1986 */
	DRIVER( rthundrb )	/* (c) 1986 */
	DRIVER( wndrmomo )	/* (c) 1987 */

	/* Namco System 1 games */
	DRIVER( shadowld )	/* (c) 1987 */
	DRIVER( youkaidk )	/* (c) 1987 (Japan) */
	DRIVER( dspirit )	/* (c) 1987 old version */
	/* 1987 Dragon Spirit new version */
	DRIVER( blazer )	/* (c) 1987 (Japan) */
	/* 1987 Quester */
	DRIVER( pacmania )	/* (c) 1987 */
	DRIVER( pacmanij )	/* (c) 1987 (Japan) */
	DRIVER( galaga88 )	/* (c) 1987 */
	DRIVER( galag88j )	/* (c) 1987 (Japan) */
	/* 1988 World Stadium */
TESTDRIVER( berabohm )	/* (c) 1988 */
	/* 1988 Alice in Wonderland (English version of Marchen maze) */
	DRIVER( mmaze )		/* (c) 1988 (Japan) */
TESTDRIVER( bakutotu )	/* (c) 1988 */
	DRIVER( wldcourt )	/* (c) 1988 (Japan) */
	DRIVER( splatter )	/* (c) 1988 (Japan) */
	/* 1988 Face Off */
	DRIVER( rompers )	/* (c) 1989 (Japan) */
	DRIVER( blastoff )	/* (c) 1989 (Japan) */
	/* World Stadium '89 */
	DRIVER( dangseed )	/* (c) 1989 (Japan) */
	DRIVER( ws90 )		/* (c) 1990 (Japan) */
	DRIVER( pistoldm )	/* (c) 1990 (Japan) */
	DRIVER( soukobdx )	/* (c) 1990 (Japan) */
TESTDRIVER( tankfrce )	/* (c) 1991 (Japan) */

	/* Namco System 2 games */
TESTDRIVER( assault )
TESTDRIVER( assaulta )
TESTDRIVER( assaultp )
TESTDRIVER( ordyne )
TESTDRIVER( metlhawk )
TESTDRIVER( mirninja )
TESTDRIVER( phelios )
TESTDRIVER( walkyrie )
TESTDRIVER( finehour )
TESTDRIVER( burnforc )
TESTDRIVER( marvland )
TESTDRIVER( dsaber )
TESTDRIVER( rthun2 )
TESTDRIVER( cosmogng )
TESTDRIVER( fourtrax )

/*
>Namco games
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
>90.5  [Kyuukai Douchuuki] (baseball game. uses character of Youkai Douchuuki)
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
	DRIVER( mariojp )	/* (c) 1983 Nintendo */
	DRIVER( masao )		/* bootleg */
	DRIVER( hunchbkd )	/* hacked Donkey Kong board */
TESTDRIVER( herocast )
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
	DRIVER( invadpt2 )	/* 852 [1980] Taito */
	DRIVER( invdpt2m )	/* 852 [1980] Midway */
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
	DRIVER( tappera )	/* (c) 1983 */
	DRIVER( sutapper )	/* (c) 1983 */
	DRIVER( rbtapper )	/* (c) 1984 */
	DRIVER( timber )	/* (c) 1984 */
	DRIVER( dotron )	/* (c) 1983 */
	DRIVER( dotrone )	/* (c) 1983 */
	DRIVER( destderb )	/* (c) 1984 */
	DRIVER( destderm )	/* (c) 1984 */
	DRIVER( sarge )		/* (c) 1985 */
	DRIVER( rampage )	/* (c) 1986 */
	DRIVER( rampage2 )	/* (c) 1986 */
	DRIVER( powerdrv )	/* (c) 1986 */
	DRIVER( maxrpm )	/* (c) 1986 */
	DRIVER( spyhunt )	/* (c) 1983 */
	DRIVER( turbotag )	/* (c) 1985 */
	DRIVER( crater )	/* (c) 1984 */
	/* MCR 68000 */
	DRIVER( zwackery )	/* (c) 1984 */
	DRIVER( xenophob )	/* (c) 1987 */
	DRIVER( spyhunt2 )	/* (c) 1987 */
	DRIVER( blasted )	/* (c) 1988 */
	DRIVER( archrivl )	/* (c) 1989 */
	DRIVER( trisport )	/* (c) 1989 */
	DRIVER( pigskin )	/* (c) 1990 */
/* other possible MCR games:
Black Belt
Shoot the Bull
Special Force
MotorDome
Six Flags (?)
*/

	/* Bally / Sente games */
	DRIVER( sentetst )
	DRIVER( cshift )	/* (c) 1984 */
	DRIVER( gghost )	/* (c) 1984 */
	DRIVER( hattrick )	/* (c) 1984 */
	DRIVER( otwalls )	/* (c) 1984 */
	DRIVER( snakepit )	/* (c) 1984 */
	DRIVER( snakjack )	/* (c) 1984 */
	DRIVER( stocker )	/* (c) 1984 */
	DRIVER( triviag1 )	/* (c) 1984 */
	DRIVER( triviag2 )	/* (c) 1984 */
	DRIVER( triviasp )	/* (c) 1984 */
	DRIVER( triviayp )	/* (c) 1984 */
	DRIVER( triviabb )	/* (c) 1984 */
	DRIVER( gimeabrk )	/* (c) 1985 */
	DRIVER( minigolf )	/* (c) 1985 */
	DRIVER( minigol2 )	/* (c) 1985 */
	DRIVER( toggle )	/* (c) 1985 */
	DRIVER( nstocker )	/* (c) 1986 */
	DRIVER( sfootbal )	/* (c) 1986 */
	DRIVER( nametune )	/* (c) 1986 */
	DRIVER( rescraid )	/* (c) 1987 */

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

	DRIVER( vigilant )	/* (c) 1988 (World) */
	DRIVER( vigilntu )	/* (c) 1988 (US) */
	DRIVER( vigilntj )	/* (c) 1988 (Japan) */
	DRIVER( kikcubic )	/* (c) 1988 (Japan) */
	/* M72 (and derivatives) */
	DRIVER( rtype )		/* (c) 1987 + Nintendo USA license (US) */
	DRIVER( rtypeb )	/* bootleg */
	DRIVER( rtypej )	/* (c) 1987 (Japan) */
TESTDRIVER( bchopper )	/* (c) 1987 */
TESTDRIVER( mrheli )	/* (c) 1987 (Japan) */
TESTDRIVER( nspirit )	/* (c) 1988 */
TESTDRIVER( nspiritj )	/* (c) 1988 (Japan) */
TESTDRIVER( imgfight )	/* (c) 1988 */
TESTDRIVER( loht )		/* (c) 1989 */
TESTDRIVER( xmultipl )	/* (c) 1989 (Japan) */
TESTDRIVER( dbreed )	/* (c) 1989 */
	DRIVER( rtype2 )	/* (c) 1989 */
	DRIVER( majtitle )	/* (c) 1990 (Japan) */
	DRIVER( hharry )	/* (c) 1990 (World) */
	DRIVER( hharryu )	/* (c) 1990 Irem America (US) */
	DRIVER( dkgensan )	/* (c) 1990 (Japan) */
	DRIVER( gallop )	/* (c) 1991 (Japan) */
TESTDRIVER( poundfor )
	/* not M72, but same sound hardware */
	DRIVER( sichuan2 )	/* (c) 1989 Tamtex */
	DRIVER( sichuana )	/* (c) 1989 Tamtex */
	DRIVER( shisen )	/* (c) 1989 Tamtex */
	/* M92 */
	DRIVER( bmaster )	/* (c) 1991 Irem */
	DRIVER( gunforce )	/* (c) 1991 Irem (World) */
	DRIVER( gunforcu )	/* (c) 1991 Irem America (US) */
	DRIVER( hook )		/* (c) 1992 Irem (World) */
	DRIVER( hooku )		/* (c) 1992 Irem America (US) */
	DRIVER( uccops )	/* (c) 1992 Irem */
	DRIVER( rtypeleo )	/* (c) 1992 Irem */
	DRIVER( majtitl2 )	/* (c) 1992 Irem */
	DRIVER( skingame )	/* (c) 1992 Irem America */
	DRIVER( skingam2 )	/* (c) 1992 Irem America */
	DRIVER( inthunt )	/* (c) 1993 Irem */
	DRIVER( kaiteids )	/* (c) 1993 Irem (Japan) */
TESTDRIVER( nbbatman )	/* (c) 1993 Irem */
TESTDRIVER( leaguemn )	/* (c) 1993 Irem (Japan) */
	DRIVER( lethalth )	/* (c) 1991 Irem */
	/* M97 */
TESTDRIVER( riskchal )
TESTDRIVER( shisen2 )
TESTDRIVER( quizf1 )
TESTDRIVER( atompunk )
TESTDRIVER( bbmanw )

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
	DRIVER( wwester1 )	/* (c) 1982 Taito Corporation */
	DRIVER( frontlin )	/* (c) 1982 Taito Corporation */
	DRIVER( elevator )	/* (c) 1983 Taito Corporation */
	DRIVER( elevatob )	/* bootleg */
	DRIVER( tinstar )	/* (c) 1983 Taito Corporation */
	DRIVER( waterski )	/* (c) 1983 Taito Corporation */
	DRIVER( bioatack )	/* (c) 1983 Taito Corporation + Fox Video Games license */
	DRIVER( hwrace )	/* (c) 1983 Taito Corporation */
	DRIVER( sfposeid )	/* 1984 */
	DRIVER( kikstart )

	/* other Taito games */
	DRIVER( bking2 )	/* (c) 1983 Taito Corporation */
	DRIVER( gsword )	/* (c) 1984 Taito Corporation */
	DRIVER( lkage )		/* (c) 1984 Taito Corporation */
	DRIVER( lkageb )	/* bootleg */
	DRIVER( retofinv )	/* (c) 1985 Taito Corporation */
	DRIVER( retofin1 )	/* bootleg */
	DRIVER( retofin2 )	/* bootleg */
	DRIVER( tsamurai )	/* (c) 1985 Taito */
	DRIVER( nunchaku )	/* (c) 1985 Taito */
	DRIVER( yamagchi )	/* (c) 1985 Taito */
TESTDRIVER( flstory )	/* (c) 1985 Taito */
	DRIVER( gladiatr )	/* (c) 1986 Taito America Corporation (US) */
	DRIVER( ogonsiro )	/* (c) 1986 Taito Corporation (Japan) */
//	DRIVER( gcastle )	/* bootleg */ temporarily removed since a complete dump is not available
	DRIVER( bublbobl )	/* (c) 1986 Taito Corporation */
	DRIVER( bublbobr )	/* (c) 1986 Taito America Corporation */
	DRIVER( boblbobl )	/* bootleg */
	DRIVER( sboblbob )	/* bootleg */
	DRIVER( tokio )		/* 1986 */
	DRIVER( tokiob )	/* bootleg */
TESTDRIVER( mexico86 )
TESTDRIVER( kicknrun )
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
TESTDRIVER( arkbl3 )	/* bootleg */
	DRIVER( arkatayt )	/* bootleg */
TESTDRIVER( arkblock )	/* bootleg */
	DRIVER( arkbloc2 )	/* bootleg */
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

	/* Taito L-System games */
	DRIVER( fhawk )		/* (c) 1988 Taito Corporation (Japan) */
	DRIVER( raimais )	/* (c) 1988 Taito Corporation (Japan) */
	DRIVER( champwr )	/* (c) 1989 Taito Corporation Japan (World) */
	DRIVER( puzznic )	/* (c) 1989 Taito Corporation (Japan) */
	DRIVER( plotting )	/* (c) 1989 Taito Corporation Japan (World) */
	DRIVER( palamed )	/* (c) 1990 Taito Corporation (Japan) */
	DRIVER( horshoes )	/* (c) 1990 Taito America Corporation (US) */
	DRIVER( cachat )	/* (c) 1993 Taito Corporation (Japan) */

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
10/92 Tatsujin II/Truxton II       Taito              Kit 2P 8W+2B     VC    Shooter TP-024
10/92 Truxton II/Tatsujin II       Taito              Kit 2P 8W+2B     VC    Shooter TP-024
      Pipi & Bipi                                                                    TP-025
   92 Fix Eight                                       Kit 2P 8W+2B     VC    Action  TP-026
12/92 V  -  V (5)/Grind Stormer                       Kit 2P 8W+2B     VC    Shooter TP-027
 1/93 Grind Stormer/V - V (Five)                      Kit 2P 8W+2B     VC    Shooter TP-027
 2/94 Batsugun                                        Kit 2P 8W+2B     VC            TP-
 4/94 Snow Bros. 2                                    Kit 2P 8W+2B     HC    Action  TP-
*/

	/* Kyugo games */
	/* Kyugo only made four games: Repulse, Flash Gal, SRD Mission and Air Wolf. */
	/* Gyrodine was made by Crux. Crux was antecedent of Toa Plan, and spin-off from Orca. */
	DRIVER( gyrodine )	/* (c) 1984 Taito Corporation */
	DRIVER( sonofphx )	/* (c) 1985 Associated Overseas MFR */
	DRIVER( repulse )	/* (c) 1985 Sega */
	DRIVER( c99 )		/* (c) 1985 Proma (bootleg?) */
	DRIVER( flashgal )	/* (c) 1985 Sega */
	DRIVER( srdmissn )	/* (c) 1986 Taito Corporation */
	DRIVER( airwolf )	/* (c) 1987 Kyugo */
	DRIVER( skywolf )	/* bootleg */
	DRIVER( skywolf2 )	/* bootleg */

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
	DRIVER( defcmnd )	/* bootleg */
	DRIVER( defence )	/* bootleg */
	DRIVER( splat )		/* (c) 1982 */
	DRIVER( blaster )	/* (c) 1983 */
	DRIVER( colony7 )	/* (c) 1981 Taito */
	DRIVER( colony7a )	/* (c) 1981 Taito */
	DRIVER( lottofun )	/* (c) 1987 H.A.R. Management */
	DRIVER( mysticm )	/* (c) 1983 */
	DRIVER( tshoot )	/* (c) 1984 */
	DRIVER( inferno )	/* (c) 1984 */
	DRIVER( joust2 )	/* (c) 1986 */

	/* Capcom games */
	/* The following is a COMPLETE list of the Capcom games up to 1997, as shown on */
	/* their web site. The list is sorted by production date. */
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
	DRIVER( trojan )	/*  4/1986 (c) 1986 (US) */
	DRIVER( trojanr )	/*  4/1986 (c) 1986 + Romstar */
	DRIVER( trojanj )	/*  4/1986 (c) 1986 (Japan) */
	DRIVER( srumbler )	/*  9/1986 (c) 1986 */
	DRIVER( srumblr2 )	/*  9/1986 (c) 1986 */
	DRIVER( rushcrsh )	/*  9/1986 (c) 1986 */
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
	/* 11/1995 Battle Arena Toshinden 2 (PSX hardware) */
	/*  7/1996 Star Gladiator (PSX hardware?) */
TESTDRIVER( sfex )		/* 12/1996 Street Fighter EX (PSX hardware) */
	/*  4/1997 Street Fighter EX Plus (PSX hardware) */
	/*  Rival Schools (PSX hardware) */
	/*  Rival Schools 2 (PSX hardware) */

	/* Capcom CPS1 games */
	DRIVER( forgottn )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lostwrld )	/*  7/1988 (c) 1988 (Japan) */
	DRIVER( ghouls )	/* 12/1988 (c) 1988 */
	DRIVER( ghoulsj )	/* 12/1988 (c) 1988 */
	DRIVER( strider )	/*  3/1989 (c) 1989 */
	DRIVER( striderj )	/*  3/1989 (c) 1989 */
	DRIVER( stridrja )	/*  3/1989 (c) 1989 */
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
	DRIVER( wonder3 )	/*  5/20/1991 (c) 1991 (Japan) */
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
	DRIVER( sf2rb )		/* hack */
	DRIVER( sf2red )	/* hack */
	DRIVER( sf2accp2 )	/* hack */
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
TESTDRIVER( ssf2j )		/* 10/1993 Super Street Fighter II */
TESTDRIVER( dadtod )	/*  1/1994 Dungeons & Dragons - Tower of Doom */
						/*  3/1994 Super Street Fighter II Turbo / Super Street Fighter II X */
TESTDRIVER( avsp )		/*  5/1994 Alien vs Predator */
						/*  6/1994 Eco Fighters / Ultimate Ecology */
TESTDRIVER( vmj )		/*  7/1994 Dark Stalkers / Vampire */
						/*  9/1994 Saturday Night Slam Masters 2 - Ring of Destruction / Super Muscle Bomber */
						/* 10/1994 Armored Warriors / Powered Gear */
TESTDRIVER( xmencota )	/* 12/1994 X-Men - Children of the Atom */
TESTDRIVER( vphj )		/*  3/1995 Night Warriors - Dark Stalkers Revenge / Vampire Hunter */
						/*  4/1995 Cyberbots */
TESTDRIVER( sfa )		/*  6/1995 Street Fighter Alpha / Street Fighter Zero */
TESTDRIVER( sfz )		/*  6/1995 Street Fighter Alpha / Street Fighter Zero */
TESTDRIVER( msh )		/* 11/1995 Marvel Super Heroes */
						/*  1/1996 19XX: The Battle Against Destiny */
						/*  2/1996 Dungeons & Dragons - Shadow Over Mystara */
						/*  3/1996 Street Fighter Alpha 2 / Street Fighter Zero 2 */
						/*  6/1996 Super Puzzle Fighter II / Turbo Super Puzzle Fighter II X */
						/*  7/1996 Rockman 2 - The Power Fighters */
						/*  8/1996 Street Fighter Zero 2 Alpha */
						/*  9/1996 Quiz Naneiro Dreams */
TESTDRIVER( xmnvssf )	/*  9/1996 X-Men vs. Street Fighter */
TESTDRIVER( vsavior )	/* 1996 Dark Stalkers 3 - Jedah's Damnation / Vampire Savior */
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
	DRIVER( dokaben )	/*  3/1989 (c) 1989 Capcom (Japan) */
	/*  8/1989 Dokaben 2 (baseball) */
	DRIVER( pang )		/* (c) 1989 Mitchell (World) */
	DRIVER( pangb )		/* bootleg */
	DRIVER( bbros )		/* (c) 1989 Capcom (US) not listed on Capcom's site */
	DRIVER( pompingw )	/* (c) 1989 Mitchell (Japan) */
	DRIVER( cbasebal )	/* 10/1989 (c) 1989 Capcom (Japan) (different hardware) */
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
TESTDRIVER( mayhem )	/* (c) 1985 Cinematronics */
TESTDRIVER( wseries )	/* (c) 1985 Cinematronics Inc. */
TESTDRIVER( dangerz )	/* (c) 1986 Cinematronics USA Inc. */
TESTDRIVER( basebal2 )	/* (c) 1987 Cinematronics Inc. */
TESTDRIVER( dblplay )	/* (c) 1987 Tradewest / The Leland Corp. */
TESTDRIVER( teamqb )	/* (c) 1988 Leland Corp. */
TESTDRIVER( strkzone )	/* (c) 1988 The Leland Corporation */
TESTDRIVER( offroad )	/* (c) 1989 Leland Corp. */
TESTDRIVER( offroadt )
TESTDRIVER( pigout )	/* (c) 1990 The Leland Corporation */
TESTDRIVER( pigoutj )	/* (c) 1990 The Leland Corporation */
TESTDRIVER( redlin2p )
TESTDRIVER( viper )
TESTDRIVER( aafb )
TESTDRIVER( aafb2p )
TESTDRIVER( aafbu )
TESTDRIVER( alleymas )
TESTDRIVER( cerberus )
TESTDRIVER( indyheat )
TESTDRIVER( wsf )
TESTDRIVER( ataxx )

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
	DRIVER( razmataz )	/* modified 834-0213, 834-0214 (c) 1983 */
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
	DRIVER( gardiab )	/* bootleg */
	DRIVER( blockgal )	/* 834-6303 (S1) */
	DRIVER( blckgalb )	/* bootleg */
	DRIVER( tokisens )	/* (c) 1987 (from a bootleg board) (S2) */
	DRIVER( wbml )		/* bootleg (S2) */
	DRIVER( wbmlj )		/* (c) 1987 Sega/Westone (S2) */
	DRIVER( wbmlj2 )	/* (c) 1987 Sega/Westone (S2) */
	DRIVER( wbmlju )	/* bootleg? (S2) */
	DRIVER( dakkochn )	/* 836-6483? (S2) */
	DRIVER( ufosensi )	/* 834-6659 (S2) */
/*
other System 1 / System 2 games:

WarBall
Rafflesia
Sanrin Sanchan
DokiDoki Penguin Land *not confirmed
*/

	/* Sega System E games (Master System hardware) */
/*
???          834-5492 (??? not sure it's System E)
Transformer  834-5803 (c) 1986
Opa Opa
Fantasy Zone 2
Hang-On Jr.
(more?)
*/

	/* other Sega 8-bit games */
	DRIVER( turbo )		/* (c) 1981 Sega */
	DRIVER( turboa )	/* (c) 1981 Sega */
	DRIVER( turbob )	/* (c) 1981 Sega */
TESTDRIVER( kopunch )	/* 834-0103 (c) 1981 Sega */
	DRIVER( suprloco )	/* (c) 1982 Sega */
	DRIVER( champbas )	/* (c) 1983 Sega */
	DRIVER( champbb2 )
	DRIVER( appoooh )	/* (c) 1984 Sega */
	DRIVER( bankp )		/* (c) 1984 Sega */

	/* Sega System 16 games */
	// Not working
	DRIVER( alexkidd )	/* (c) 1986 (protected) */
	DRIVER( aliensya )	/* (c) 1987 (protected) */
	DRIVER( aliensyb )	/* (c) 1987 (protected) */
	DRIVER( astorm )	/* (c) 1990 (protected) */
	DRIVER( auraila )	/* (c) 1990 Sega / Westone (protected) */
	DRIVER( bayrtbl1 )	/* (c) 1989 (protected) (bootleg) */
	DRIVER( bayrtbl2 )	/* (c) 1989 (protected) (bootleg) */
	DRIVER( bayrouta )	/* (c) 1989 (protected) */
	DRIVER( eswat )		/* (c) 1989 (protected) */
	DRIVER( enduror )	/* (c) 1985 (protected) */
	DRIVER( fpoint )	/* (c) 1989 (protected) */
	DRIVER( goldnaxb )	/* (c) 1989 (protected) */
	DRIVER( goldnaxc )	/* (c) 1989 (protected) */
	DRIVER( moonwalk )	/* (c) 1990 (protected) */
	DRIVER( passsht )	/* (protected) */
	DRIVER( shangon )	/* (c) 1992 (protected) */
	DRIVER( shinobia )	/* (c) 1987 (protected) */
	DRIVER( tetris )	/* (c) 1988 (protected) */
	DRIVER( wb3a )		/* (c) 1988 Sega / Westone (protected) */

TESTDRIVER( aceattac )	/* (protected) */
TESTDRIVER( aburner )	/* */
TESTDRIVER( bloxeed )	/* (protected) */
TESTDRIVER( cltchitr )	/* (protected) */
TESTDRIVER( cotton )	/* (protected) */
TESTDRIVER( cottona )	/* (protected) */
TESTDRIVER( ddcrew )	/* (protected) */
TESTDRIVER( dunkshot )	/* (protected) */
TESTDRIVER( lghost )	/* (protected) */
TESTDRIVER( loffire )	/* (protected) */
TESTDRIVER( mvp )		/* (protected) */
TESTDRIVER( thndrbld )	/* (protected) */
TESTDRIVER( toutrun )	/* (protected) */
TESTDRIVER( toutruna )	/* (protected) */

	// Working
	DRIVER( alexkida )	/* (c) 1986 */
	DRIVER( aliensyn )	/* (c) 1987 */
	DRIVER( altbeast )	/* (c) 1988 */
	DRIVER( altbeas2 )	/* (c) 1988 */
	DRIVER( astormbl )	/* bootleg */
	DRIVER( atomicp )	/* (c) 1990 Philko */
	DRIVER( aurail )	/* (c) 1990 Sega / Westone */
	DRIVER( bayroute )	/* (c) 1989 */
	DRIVER( bodyslam )	/* (c) 1986 */
	DRIVER( dduxbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( endurobl )	/* (c) 1985 (Herb bootleg) */
	DRIVER( endurob2 )	/* (c) 1985 (Beta bootleg) */
	DRIVER( eswatbl )	/* (c) 1989 (but bootleg) */
	DRIVER( fantzone )	/* (c) 1986 */
	DRIVER( fpointbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( goldnaxe )	/* (c) 1989 */
	DRIVER( goldnaxa )	/* (c) 1989 */
	DRIVER( goldnabl )	/* (c) 1989 (bootleg) */
	DRIVER( hangon )	/* (c) 1985 */
	DRIVER( hwchamp )	/* (c) 1987 */
	DRIVER( moonwlkb )	/* bootleg */
	DRIVER( mjleague )	/* (c) 1985 */
	DRIVER( outrun )	/* (c) 1986 (bootleg)*/
	DRIVER( outruna )	/* (c) 1986 (bootleg) */
	DRIVER( outrunb )	/* (c) 1986 (protected beta bootleg) */
	DRIVER( passshtb )	/* bootleg */
	DRIVER( quartet )	/* (c) 1986 */
	DRIVER( quartet2 )	/* (c) 1986 */
	DRIVER( riotcity )	/* (c) 1991 Sega / Westone */
	DRIVER( sdi )		/* (c) 1987 */
	DRIVER( shangonb )	/* (c) 1992 (but bootleg) */
	DRIVER( sharrier )	/* (c) 1985 */
	DRIVER( shdancer )	/* (c) 1989 */
	DRIVER( shdancbl )	/* (c) 1989 (but bootleg) */
	DRIVER( shdancrj )	/* (c) 1989 */
	DRIVER( shinobi )	/* (c) 1987 */
	DRIVER( shinobl )	/* (c) 1987 (but bootleg) */
	DRIVER( tetrisbl )	/* (c) 1988 (but bootleg) */
	DRIVER( timscanr )	/* (c) 1987 */
	DRIVER( tturf )		/* (c) 1989 Sega / Sunsoft */
	DRIVER( tturfbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( wb3 )		/* (c) 1988 Sega / Westone */
	DRIVER( wb3bl )		/* (c) 1988 Sega / Westone (but bootleg) */
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
	/* cassette system */
TESTDRIVER( decocass )
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
	DRIVER( sidepckt )	/* (c) 1986 Data East Corporation */
	DRIVER( sidepckb )	/* bootleg */
	DRIVER( exprraid )	/* (c) 1986 Data East USA (US) */
	DRIVER( wexpress )	/* (c) 1986 Data East Corporation (World?) */
	DRIVER( wexpresb )	/* bootleg */
	DRIVER( pcktgal )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( pcktgalb )	/* bootleg */
	DRIVER( pcktgal2 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3i )	/* (c) 1990 Data East Corporation + I-Vics license */
	DRIVER( actfancr )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( actfancj )	/* (c) 1989 Data East Corporation (Japan) */

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
	DRIVER( garyoret )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( cobracom )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( oscar )		/* (c) 1988 Data East USA (US) */
	DRIVER( oscarj )	/* (c) 1987 Data East Corporation (Japan) */

	/* Data East 16-bit games */
	DRIVER( karnov )	/* (c) 1987 Data East USA (US) */
	DRIVER( karnovj )	/* (c) 1987 Data East Corporation (Japan) */
TESTDRIVER( wndrplnt )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( chelnov )	/* (c) 1988 Data East USA (US) */
	DRIVER( chelnovj )	/* (c) 1988 Data East Corporation (Japan) */
/* the following ones all run on similar hardware */
	DRIVER( hbarrel )	/* (c) 1987 Data East USA (US) */
	DRIVER( hbarrelw )	/* (c) 1987 Data East Corporation (World) */
	DRIVER( baddudes )	/* (c) 1988 Data East USA (US) */
	DRIVER( drgninja )	/* (c) 1988 Data East Corporation (Japan) */
TESTDRIVER( birdtry )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( robocop )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( robocopu )	/* (c) 1988 Data East USA (US) */
	DRIVER( robocpu0 )	/* (c) 1988 Data East USA (US) */
	DRIVER( robocopb )	/* bootleg */
	DRIVER( hippodrm )	/* (c) 1989 Data East USA (US) */
	DRIVER( ffantasy )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( slyspy )	/* (c) 1989 Data East USA (US) */
	DRIVER( slyspy2 )	/* (c) 1989 Data East USA (US) */
	DRIVER( secretag )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( midres )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( midresu )	/* (c) 1989 Data East USA (US) */
	DRIVER( midresj )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( bouldash )	/* (c) 1990 Data East Corporation */
/* end of similar hardware */
	DRIVER( stadhero )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( madmotor )	/* (c) [1989] Mitchell */
	DRIVER( vaportra )	/* (c) 1989 Data East Corporation (US) */
	DRIVER( kuhga )		/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( cbuster )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( cbusterw )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( cbusterj )	/* (c) 1990 Data East Corporation (Japan) */
	DRIVER( twocrude )	/* (c) 1990 Data East USA (US) */
	DRIVER( darkseal )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( darksea1 )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( gatedoom )	/* (c) 1990 Data East Corporation (US) */
	DRIVER( gatedom1 )	/* (c) 1990 Data East Corporation (US) */
	DRIVER( supbtime )	/* (c) 1990 Data East Corporation (Japan) */
TESTDRIVER( edrandy )	/* (c) 1990 Data East Corporation (World) */
TESTDRIVER( edrandyj )	/* (c) 1990 Data East Corporation (Japan) */
	DRIVER( cninja )	/* (c) 1991 Data East Corporation (World) */
	DRIVER( cninja0 )	/* (c) 1991 Data East Corporation (World) */
	DRIVER( cninjau )	/* (c) 1991 Data East Corporation (US) */
	DRIVER( joemac )	/* (c) 1991 Data East Corporation (Japan) */
	DRIVER( stoneage )	/* bootleg */
	DRIVER( tumblep )	/* 1991 bootleg */
	DRIVER( tumblep2 )	/* 1991 bootleg */
TESTDRIVER( funkyjet )	/* (c) 1992 Mitchell */

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
	DRIVER( psurge )	/* (c) 1988 unknown (NOT Konami) */
	DRIVER( megazone )	/* GX319 (c) 1983 + Kosuka */
	DRIVER( pandoras )	/* GX328 (c) 1984 + Interlogic */
	DRIVER( gyruss )	/* GX347 (c) 1983 */
	DRIVER( gyrussce )	/* GX347 (c) 1983 + Centuri license */
	DRIVER( venus )		/* bootleg */
	DRIVER( trackfld )	/* GX361 (c) 1983 */
	DRIVER( trackflc )	/* GX361 (c) 1983 + Centuri license */
	DRIVER( hyprolym )	/* GX361 (c) 1983 */
	DRIVER( hyprolyb )	/* bootleg */
	DRIVER( rocnrope )	/* GX364 (c) 1983 + Kosuka */
	DRIVER( ropeman )	/* bootleg */
	DRIVER( circusc )	/* GX380 (c) 1984 */
	DRIVER( circusc2 )	/* GX380 (c) 1984 */
	DRIVER( circuscc )	/* GX380 (c) 1984 + Centuri license */
	DRIVER( circusce )	/* GX380 (c) 1984 + Centuri license */
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
	DRIVER( mrgoemon )	/* GX621 (c) 1986 (Japan) */
	DRIVER( jailbrek )	/* GX507 (c) 1986 */
	DRIVER( ironhors )	/* GX560 (c) 1986 */
	DRIVER( farwest )
	DRIVER( jackal )	/* GX631 (c) 1986 */
	DRIVER( topgunr )	/* GX631 (c) 1986 */
	DRIVER( topgunbl )	/* bootleg */
	DRIVER( ddrible )	/* GX690 (c) 1986 */
	DRIVER( contra )	/* GX633 (c) 1987 */
	DRIVER( contrab )	/* bootleg */
	DRIVER( contrajb )	/* bootleg */
	DRIVER( gryzor )	/* GX633 (c) 1987 */
	DRIVER( combasc )	/* GX611 (c) 1988 */
	DRIVER( combasct )	/* GX611 (c) 1987 */
	DRIVER( combascj )	/* GX611 (c) 1987 (Japan) */
	DRIVER( bootcamp )	/* GX611 (c) 1987 */
	DRIVER( combascb )	/* bootleg */
	DRIVER( rockrage )	/* GX620 (c) 1986 (World?) */
	DRIVER( rockragj )	/* GX620 (c) 1986 (Japan) */
	DRIVER( battlnts )	/* GX777 (c) 1987 */
	DRIVER( battlntj )	/* GX777 (c) 1987 (Japan) */
	DRIVER( bladestl )	/* GX797 (c) 1987 */
	DRIVER( hcastle )	/* GX768 (c) 1988 */
	DRIVER( hcastlea )	/* GX768 (c) 1988 */
	DRIVER( hcastlej )	/* GX768 (c) 1988 (Japan) */
	DRIVER( ajax )		/* GX770 (c) 1987 */
	DRIVER( scontra )	/* GX775 (c) 1988 */
	DRIVER( scontraj )	/* GX775 (c) 1988 (Japan) */
	DRIVER( thunderx )	/* GX873 (c) 1988 */
	DRIVER( thnderxj )	/* GX873 (c) 1988 (Japan) */
	DRIVER( mainevt )	/* GX799 (c) 1988 */
	DRIVER( mainevt2 )	/* GX799 (c) 1988 */
	DRIVER( devstors )	/* GX890 (c) 1988 */
	DRIVER( devstor2 )	/* GX890 (c) 1988 */
	DRIVER( garuka )	/* GX890 (c) 1988 (Japan) */
	DRIVER( k88games )	/* GX861 (c) 1988 */
	DRIVER( konami88 )	/* GX861 (c) 1988 */
	DRIVER( gbusters )	/* GX878 (c) 1988 */
	DRIVER( crimfght )	/* GX821 (c) 1989 (US) */
	DRIVER( crimfgtj )	/* GX821 (c) 1989 (Japan) */
	DRIVER( bottom9 )	/* GX891 (c) 1989 */
	DRIVER( bottom9n )	/* GX891 (c) 1989 */
	DRIVER( quarth )	/* GX973 (c) 1989 */
	DRIVER( aliens )	/* GX875 (c) 1990 */
	DRIVER( aliens2 )	/* GX875 (c) 1990 */
	DRIVER( aliensj )	/* GX875 (c) 1990 (Japan) */
	DRIVER( surpratk )	/* GX911 (c) 1990 (Japan) */
	DRIVER( parodius )	/* GX955 (c) 1990 (Japan) */
	DRIVER( rollerg )	/* GX999 (c) 1991 (US) */
	DRIVER( rollergj )	/* GX999 (c) 1991 (Japan) */
TESTDRIVER( xexex )		/* GX067 (c) 1991 */
	DRIVER( simpsons )	/* GX072 (c) 1991 */
	DRIVER( simpsn2p )	/* GX072 (c) 1991 */
	DRIVER( simps2pj )	/* GX072 (c) 1991 (Japan) */
	DRIVER( vendetta )	/* GX081 (c) 1991 (Asia) */
	DRIVER( vendett2 )	/* GX081 (c) 1991 (Asia) */
	DRIVER( vendettj )	/* GX081 (c) 1991 (Japan) */
	DRIVER( wecleman )	/* GX602 (c) 1986 */
	DRIVER( hotchase )	/* GX763 (c) 1988 */

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

	/* Konami "Twin 16" games */
TESTDRIVER( devilw )	/* GX687 (c) 1987 */
TESTDRIVER( darkadv )	/* GX687 (c) 1987 */
TESTDRIVER( vulcan )	/* GX785 (c) 1988 */
TESTDRIVER( gradius2 )	/* GX785 (c) 1988 (Japan) */
TESTDRIVER( cuebrick )	/* GX903 (c) 1989 */
TESTDRIVER( fround )	/* GX870 (c) 1988 */

	/* (some) Konami 68000 games */
	DRIVER( mia )		/* GX808 (c) 1989 */
	DRIVER( mia2 )		/* GX808 (c) 1989 */
	DRIVER( tmnt )		/* GX963 (c) 1989 (US) */
	DRIVER( tmht )		/* GX963 (c) 1989 (UK) */
	DRIVER( tmntj )		/* GX963 (c) 1989 (Japan) */
	DRIVER( tmht2p )	/* GX963 (c) 1989 (UK) */
	DRIVER( tmnt2pj )	/* GX963 (c) 1990 (Japan) */
	DRIVER( punkshot )	/* GX907 (c) 1990 (US) */
	DRIVER( punksht2 )	/* GX907 (c) 1990 (US) */
	DRIVER( lgtnfght )	/* GX939 (c) 1990 (US) */
	DRIVER( trigon )	/* GX939 (c) 1990 (Japan) */
	DRIVER( detatwin )	/* GX060 (c) 1991 (Japan) */
TESTDRIVER( glfgreat )	/* GX061 (c) 1991 */
	DRIVER( tmnt2 )		/* GX063 (c) 1991 (US) */
	DRIVER( tmnt22p )	/* GX063 (c) 1991 (US) */
	DRIVER( tmnt2a )	/* GX063 (c) 1991 (Asia) */
	DRIVER( ssriders )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdrebd )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdrebc )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdruda )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdruac )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdrubc )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdrabd )	/* GX064 (c) 1991 (Asia) */
	DRIVER( ssrdrjbd )	/* GX064 (c) 1991 (Japan) */
	DRIVER( xmen )		/* GX065 (c) 1992 (US) */
	DRIVER( xmen6p )	/* GX065 (c) 1992 */

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
TESTDRIVER( polepos )
TESTDRIVER( poleposa )
TESTDRIVER( poleps2b )
	DRIVER( foodf )		/* (c) 1982 */	/* made by Gencomp */
	DRIVER( liberatr )	/* (c) 1982 */
TESTDRIVER( liberat2 )
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
	DRIVER( indytem4 )	/* (c) 1985 */
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
	DRIVER( toobin2 )	/* (c) 1988 */
	DRIVER( toobinp )	/* (c) 1988 */
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
	DRIVER( cyberbal )	/* (c) 1989 */
	DRIVER( cyberbt )	/* (c) 1989 */
	DRIVER( cyberb2p )	/* (c) 1989 */
	DRIVER( rampart )	/* (c) 1990 */
	DRIVER( ramprt2p )	/* (c) 1990 */
	DRIVER( shuuz )		/* (c) 1990 */
	DRIVER( shuuz2 )	/* (c) 1990 */
	DRIVER( hydra )		/* (c) 1990 */
	DRIVER( hydrap )	/* (c) 1990 */
	DRIVER( pitfight )	/* (c) 1990 */
	DRIVER( pitfigh3 )	/* (c) 1990 */
	DRIVER( thunderj )	/* (c) 1990 */
	DRIVER( batman )	/* (c) 1991 */
	DRIVER( relief )	/* (c) 1992 */
	DRIVER( relief2 )	/* (c) 1992 */
	DRIVER( offtwall )	/* (c) 1991 */
	DRIVER( offtwalc )	/* (c) 1991 */

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
	DRIVER( mnchmobl )	/* A2001 (c) 1983 + Centuri license */
	DRIVER( marvins )	/* A2003 (c) 1983 */
	DRIVER( madcrash )	/* A2005 (c) 1984 */
	DRIVER( vangrd2 )	/* (c) 1984 */
TESTDRIVER( hal21 )
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
	DRIVER( bermudaj )	/* (c) 1987 */
	DRIVER( psychos )	/* (c) 1987 */
	DRIVER( psychosa )	/* (c) 1987 */
	DRIVER( chopper )	/* (c) 1988 */
	DRIVER( legofair )	/* (c) 1988 */
TESTDRIVER( ftsoccer )	/* (c) 1988 */
	DRIVER( tdfever )	/* (c) 1987 */
	DRIVER( tdfeverj )	/* (c) 1987 */
	DRIVER( pow )		/* (c) 1988 */
	DRIVER( powj )		/* (c) 1988 */
	DRIVER( searchar )	/* (c) 1989 */
	DRIVER( streetsm )	/* (c) 1989 */
	DRIVER( streets2 )	/* (c) 1989 */
	DRIVER( streetsj )	/* (c) 1989 */
	DRIVER( ikari3 )	/* (c) 1989 */
	DRIVER( prehisle )	/* (c) 1989 */
	DRIVER( prehislu )	/* (c) 1989 */
	DRIVER( prehislj )	/* (c) 1989 */

	/* SNK / Alpha 68K games */
TESTDRIVER( kyros )
TESTDRIVER( sstingry )
TESTDRIVER( paddlema )	/* (c) 1988 SNK */
	DRIVER( timesold )	/* (c) 1987 SNK / Romstar */
	DRIVER( timesol2 )
	DRIVER( btlfield )
	DRIVER( skysoldr )	/* (c) 1988 SNK (Romstar with dip switch) */
	DRIVER( goldmedl )	/* (c) 1988 SNK */
TESTDRIVER( goldmedb )	/* bootleg */
	DRIVER( skyadvnt )	/* (c) 1989 SNK of America licensed from Alpha */
	DRIVER( gangwars )	/* (c) 1989 Alpha */
	DRIVER( gangwarb )	/* bootleg */
	DRIVER( sbasebal )	/* (c) 1989 SNK of America licensed from Alpha */

	/* Technos games */
	DRIVER( scregg )	/* TA-0001 (c) 1983 */
	DRIVER( eggs )		/* TA-0002 (c) 1983 Universal USA */
	DRIVER( tagteam )	/* TA-0007 (c) 1983 + Data East license */
	/* TA-0008 Syusse Oozumou */
	DRIVER( mystston )	/* TA-0010 (c) 1984 */
	/* TA-0011 Dog Fight (Data East) / Batten O'hara no Sucha-Raka Kuuchuu Sen 1985 */
	DRIVER( matmania )	/* TA-0015 (c) 1985 + Taito America license */
	DRIVER( excthour )	/* TA-0015 (c) 1985 + Taito license */
	DRIVER( maniach )	/* TA-0017 (c) 1986 + Taito America license */
	DRIVER( maniach2 )	/* TA-0017 (c) 1986 + Taito America license */
	DRIVER( renegade )	/* TA-0018 (c) 1986 + Taito America license */
	DRIVER( kuniokun )	/* TA-0018 (c) 1986 */
	DRIVER( kuniokub )	/* bootleg */
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
	DRIVER( nova2001 )	/* UPL-83005 (c) [1984?] + Universal license */
	DRIVER( pkunwar )	/* [1985?] */
	DRIVER( pkunwarj )	/* [1985?] */
	DRIVER( ninjakd2 )	/* (c) 1987 */
	DRIVER( ninjak2a )	/* (c) 1987 */
	DRIVER( ninjak2b )	/* (c) 1987 */
	DRIVER( rdaction )	/* (c) 1987 + World Games license */
	DRIVER( mnight )	/* (c) 1987 distributed by Kawakus */
	DRIVER( arkarea )	/* UPL-87007 (c) [1988?] */
/*
Urashima Mahjong    UPL-89052

UPL Game List
V1.2   May 27,1999

   83 Mouser                              Kit 2P              Action   83001
 3/84 Nova 2001                 Universal Kit 2P  8W+2B   HC  Shooter  85005
   84 Penguin Wars (Kun)                      2P              Action
   84 Ninja Kun                 Taito                                  85003
   85 Raiders 5                 Taito                                  85004
 8/87 Mission XX                          Kit 2P  8W+2B   VC  Shooter  86001
   87 Mutant Night                        Kit 2P  8W+2B   HC  Action
 7/87 Rad Action/Ninja Taro   World Games Kit 2P  8W+2B   HC  Action   87003
 7/87 Ninja Taro/Rad Action   World Games Kit 2P  8W+2B   HC  Action
   87 Ninja Taro II                       Kit 2P  8W+2B   HC  Action
   88 Aquaria                             Kit 2P  8W+2B
   89 Ochichi Mahjong                     Kit 2P  8W+2B   HC  Mahjong
 9/89 Omega Fighter        American Sammy Kit 2P  8W+2B   HC  Shooter  89016
12/89 Task Force Harrier   American Sammy Kit 2P  8W+2B   VC  Shooter  89053
   90 Atomic Robo-Kid      American Sammy Kit 2P  8W+2B   HC  Shooter  88013
   90 Mustang - U.S.A.A.F./Fire Mustang   Kit 2P  8W+2B   HC  Shooter  90058
   91 Acrobat Mission               Taito Kit 2P  8W+2B   VC  Shooter
   91 Bio Ship Paladin/Spaceship Gomera   Kit 2P  8W+2B   HC  Shooter  90062
   91 Black Heart                         Kit 2P  8W+2B   HC  Shooter
   91 Spaceship Gomera/Bio Ship Paladin   Kit 2P  8W+2B   HC  Shooter
   91 Van Dyke Fantasy                    Kit 2P  8W+2B
 2/92 Strahl                              Kit 2P  8W+3B                91074
   93 Fire Suplex (Neo-Geo System)       Cart 2P  8W+4B   HC

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
	/* this one uses 68000+Z80 instead of the Cinematronics CPU */
	DRIVER( cchasm )
	DRIVER( cchasm1 )	/* (c) 1983 Cinematronics / GCE */

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
	DRIVER( empcity )	/* (c) 1986 Seibu Kaihatsu (bootleg?) */
	DRIVER( stfight )	/* (c) 1986 Seibu Kaihatsu (Germany) (bootleg?) */
	DRIVER( raiden )	/* (c) 1990 Seibu Kaihatsu */
TESTDRIVER( raidena )	/* (c) 1990 Seibu Kaihatsu */

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
	DRIVER( ginganin )	/* (c) 1987 Jaleco */

	/* Jaleco Mega System 1 games */
	DRIVER( lomakaj )	/* (c) 1988 */
	DRIVER( p47 )		/* (c) 1988 */
	DRIVER( p47j )		/* (c) 1988 */
	DRIVER( rodland )	/* (c) 1990 */
	DRIVER( rodlandj )	/* (c) 1990 (Japan) */
	DRIVER( street64 )	/* (c) 1991 */
	DRIVER( streej64 )	/* (c) 1991 (Japan) */
	DRIVER( edf )		/* (c) 1991 */
	DRIVER( avspirit )	/* (c) 1991 */
	DRIVER( phantasm )	/* (c) 1991 (Japan) */
	DRIVER( astyanax )	/* (c) 1989 */
	DRIVER( lordofk )	/* (c) 1989 (Japan) */
	DRIVER( hachoo )	/* (c) 1989 */
TESTDRIVER( plusalph )	/* (c) 1989 */
	DRIVER( stdragon )	/* (c) 1989 */
TESTDRIVER( chimerab )	/* (c) 1993 */
	DRIVER( cybattlr )	/* (c) 1993 */
TESTDRIVER( iganinju )	/* (c) 1989 */

	/* Video System Co. games */
	DRIVER( pspikes )	/* (c) 1991 */
	DRIVER( turbofrc )	/* (c) 1991 */
	DRIVER( aerofgt )	/* (c) 1992 */
	DRIVER( aerofgtb )	/* (c) 1992 */
	DRIVER( aerofgtc )	/* (c) 1992 */
TESTDRIVER( unkvsys )

	/* Tad games */
	DRIVER( cabal )		/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabal2 )	/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabalbl )	/* bootleg */
	DRIVER( toki )		/* (c) 1989 Tad */
	DRIVER( toki2 )		/* (c) 1989 Tad */
	DRIVER( toki3 )		/* (c) 1989 Tad */
	DRIVER( tokiu )		/* (c) 1989 Tad + Fabtek license */
	DRIVER( tokib )		/* bootleg */
	DRIVER( bloodbro )	/* (c) 1990 Tad */
	DRIVER( weststry )	/* bootleg */

	/* Orca games */
	DRIVER( marineb )	/* (c) 1982 Orca */
	DRIVER( changes )	/* (c) 1982 Orca */
	DRIVER( springer )	/* (c) 1982 Orca */
	DRIVER( hoccer )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( hoccer2 )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( hopprobo )	/* (c) 1983 Sega */
	DRIVER( wanted )	/* (c) 1984 Sigma Ent. Inc. */
	DRIVER( funkybee )	/* (c) 1982 Orca */
	DRIVER( skylancr )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( zodiack )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( dogfight )	/* (c) 1983 Thunderbolt */
	DRIVER( moguchan )	/* (c) 1982 Orca + Eastern Commerce Inc. license (doesn't appear on screen) */
	DRIVER( espial )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */
	DRIVER( espiale )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */
	/* Vastar was made by Orca, but when it was finished, Orca had already bankrupted. */
	/* So they sold this game as "Made by Sesame Japan" because they couldn't use */
	/* the name "Orca" */
	DRIVER( vastar )	/* (c) 1983 Sesame Japan */
	DRIVER( vastar2 )	/* (c) 1983 Sesame Japan */

	DRIVER( spacefb )	/* (c) [1980?] Nintendo */
	DRIVER( spacefbg )	/* 834-0031 (c) 1980 Gremlin */
	DRIVER( spacebrd )	/* bootleg */
	DRIVER( spacedem )	/* (c) 1980 Nintendo / Fortrek */
	DRIVER( blueprnt )	/* (c) 1982 Bally Midway (Zilec in ROM 3U, and the programmer names) */
	DRIVER( saturn )	/* (c) 1983 Jaleco (Zilec in ROM R6, and the programmer names) */
	DRIVER( omegrace )	/* (c) 1981 Midway */
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
TESTDRIVER( irobot )
	DRIVER( spiders )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( spiders2 )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( stactics )	/* [1981 Sega] */
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
TESTDRIVER( shanghai )	/* (c) 1988 Sun Electronics */
	DRIVER( goindol )	/* (c) 1987 Sun a Electronics */
	DRIVER( homo )		/* bootleg */
TESTDRIVER( dlair )
	DRIVER( goldstar )
	DRIVER( goldstbl )
	DRIVER( csk227it )	/* (c) [1993] IGS */
	DRIVER( csk234it )	/* (c) [1993] IGS */
	DRIVER( meteor )	/* (c) 1981 Venture Line */
	DRIVER( bjtwin )	/* (c) 1993 NMK */
	DRIVER( aztarac )	/* (c) 1983 Centuri (vector game) */


#endif /* NEOMAME */

#ifndef NEOFREE

	/* Neo Geo games */
	/* the four digits number is the game ID stored at address 0x0108 of the program ROM */
	DRIVER( neo_nam1975 )	/* 0001 (c) 1990 SNK */
	DRIVER( neo_bstars )	/* 0002 (c) 1990 SNK */
	DRIVER( neo_tpgolf )	/* 0003 (c) 1990 SNK */
	DRIVER( neo_mahretsu )	/* 0004 (c) 1990 SNK */
	DRIVER( neo_maglord )	/* 0005 (c) 1990 Alpha Denshi Co */
	DRIVER( neo_maglordh )	/* 0005 (c) 1990 Alpha Denshi Co */
	DRIVER( neo_ridhero )	/* 0006 (c) 1990 SNK */
	DRIVER( neo_alpham2 )	/* 0007 (c) 1991 SNK */
	/* 0008 */
	DRIVER( neo_ncombat )	/* 0009 (c) 1990 Alpha Denshi Co */
	DRIVER( neo_cyberlip )	/* 0010 (c) 1990 SNK */
	DRIVER( neo_superspy )	/* 0011 (c) 1990 SNK */
	/* 0012 */
	/* 0013 */
	DRIVER( neo_mutnat )	/* 0014 (c) 1992 SNK */
	/* 0015 */
	DRIVER( neo_kotm )		/* 0016 (c) 1991 SNK */
	DRIVER( neo_sengoku )	/* 0017 (c) 1991 SNK */
	DRIVER( neo_burningf )	/* 0018 (c) 1991 SNK */
	DRIVER( neo_lbowling )	/* 0019 (c) 1990 SNK */
	DRIVER( neo_gpilots )	/* 0020 (c) 1991 SNK */
	DRIVER( neo_joyjoy )	/* 0021 (c) 1990 SNK */
	DRIVER( neo_bjourney )	/* 0022 (c) 1990 Alpha Denshi Co */
	DRIVER( neo_quizdais )	/* 0023 (c) 1991 SNK */
	DRIVER( neo_lresort )	/* 0024 (c) 1992 SNK */
	DRIVER( neo_eightman )	/* 0025 (c) 1991 SNK / Pallas */
	/* 0026 Fun Fun Brothers - prototype? */
	DRIVER( neo_minasan )	/* 0027 (c) 1990 Monolith Corp. */
	/* 0028 */
	DRIVER( neo_legendos )	/* 0029 (c) 1991 SNK */
	DRIVER( neo_2020bb )	/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( neo_2020bbh )	/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( neo_socbrawl )	/* 0031 (c) 1991 SNK */
	DRIVER( neo_roboarmy )	/* 0032 (c) 1991 SNK */
	DRIVER( neo_fatfury1 )	/* 0033 (c) 1991 SNK */
	DRIVER( neo_fbfrenzy )	/* 0034 (c) 1992 SNK */
	/* 0035 */
	DRIVER( neo_bakatono )	/* 0036 (c) 1991 Monolith Corp. */
	DRIVER( neo_crsword )	/* 0037 (c) 1991 Alpha Denshi Co */
	DRIVER( neo_trally )	/* 0038 (c) 1991 Alpha Denshi Co */
	DRIVER( neo_kotm2 )		/* 0039 (c) 1992 SNK */
	DRIVER( neo_sengoku2 )	/* 0040 (c) 1993 SNK */
	DRIVER( neo_bstars2 )	/* 0041 (c) 1992 SNK */
	DRIVER( neo_quizdai2 )	/* 0042 (c) 1992 SNK */
	DRIVER( neo_3countb )	/* 0043 (c) 1993 SNK */
	DRIVER( neo_aof )		/* 0044 (c) 1992 SNK */
	DRIVER( neo_samsho )	/* 0045 (c) 1993 SNK */
	DRIVER( neo_tophuntr )	/* 0046 (c) 1994 SNK */
	DRIVER( neo_fatfury2 )	/* 0047 (c) 1992 SNK */
	DRIVER( neo_janshin )	/* 0048 (c) 1994 Aicom */
	DRIVER( neo_androdun )	/* 0049 (c) 1992 Visco */
	DRIVER( neo_ncommand )	/* 0050 (c) 1992 Alpha Denshi Co */
	DRIVER( neo_viewpoin )	/* 0051 (c) 1992 Sammy */
	DRIVER( neo_ssideki )	/* 0052 (c) 1992 SNK */
	DRIVER( neo_wh1 )		/* 0053 (c) 1992 Alpha Denshi Co */
	/* 0054 Crossed Swords 2 (CD only) */
	DRIVER( neo_kof94 )		/* 0055 (c) 1994 SNK */
	DRIVER( neo_aof2 )		/* 0056 (c) 1994 SNK */
	DRIVER( neo_wh2 )		/* 0057 (c) 1993 ADK */
	DRIVER( neo_fatfursp )	/* 0058 (c) 1993 SNK */
	DRIVER( neo_savagere )	/* 0059 (c) 1995 SNK */
	DRIVER( neo_fightfev )	/* 0060 (c) 1994 Viccom */
	DRIVER( neo_ssideki2 )	/* 0061 (c) 1994 SNK */
	DRIVER( neo_spinmast )	/* 0062 (c) 1993 Data East Corporation */
	DRIVER( neo_samsho2 )	/* 0063 (c) 1994 SNK */
	DRIVER( neo_wh2j )		/* 0064 (c) 1994 ADK / SNK */
	DRIVER( neo_wjammers )	/* 0065 (c) 1994 Data East Corporation */
	DRIVER( neo_karnovr )	/* 0066 (c) 1994 Data East Corporation */
	DRIVER( neo_gururin )	/* 0067 (c) 1994 Face */
	DRIVER( neo_pspikes2 )	/* 0068 (c) 1994 Video System Co. */
	DRIVER( neo_fatfury3 )	/* 0069 (c) 1995 SNK */
	/* 0070 */
	/* 0071 */
	/* 0072 */
	DRIVER( neo_panicbom )	/* 0073 (c) 1994 Eighting / Hudson */
	DRIVER( neo_aodk )		/* 0074 (c) 1994 ADK / SNK */
	DRIVER( neo_sonicwi2 )	/* 0075 (c) 1994 Video System Co. */
	DRIVER( neo_zedblade )	/* 0076 (c) 1994 NMK */
	/* 0077 */
	DRIVER( neo_galaxyfg )	/* 0078 (c) 1995 Sunsoft */
	DRIVER( neo_strhoop )	/* 0079 (c) 1994 Data East Corporation */
	DRIVER( neo_quizkof )	/* 0080 (c) 1995 Saurus */
	DRIVER( neo_ssideki3 )	/* 0081 (c) 1995 SNK */
	DRIVER( neo_doubledr )	/* 0082 (c) 1995 Technos */
	DRIVER( neo_pbobble )	/* 0083 (c) 1994 Taito */
	DRIVER( neo_kof95 )		/* 0084 (c) 1995 SNK */
	/* 0085 Shinsetsu Samurai Spirits Bushidoretsuden / Samurai Shodown RPG (CD only) */
	DRIVER( neo_tws96 )		/* 0086 (c) 1996 Tecmo */
	DRIVER( neo_samsho3 )	/* 0087 (c) 1995 SNK */
	DRIVER( neo_stakwin )	/* 0088 (c) 1995 Saurus */
	DRIVER( neo_pulstar )	/* 0089 (c) 1995 Aicom */
	DRIVER( neo_whp )		/* 0090 (c) 1995 ADK / SNK */
	/* 0091 */
	DRIVER( neo_kabukikl )	/* 0092 (c) 1995 Hudson */
	DRIVER( neo_neobombe )	/* 0093 (c) 1997 Hudson */
	DRIVER( neo_gowcaizr )	/* 0094 (c) 1995 Technos */
	DRIVER( neo_rbff1 )		/* 0095 (c) 1995 SNK */
	DRIVER( neo_aof3 )		/* 0096 (c) 1996 SNK */
	DRIVER( neo_sonicwi3 )	/* 0097 (c) 1995 Video System Co. */
	/* 0098 Idol Mahjong - final romance 2 (CD only? not confirmed, MVS might exist) */
	/* 0099 */
	DRIVER( neo_turfmast )	/* 0200 (c) 1996 Nazca */
	DRIVER( neo_mslug )		/* 0201 (c) 1996 Nazca */
	DRIVER( neo_puzzledp )	/* 0202 (c) 1995 Taito (Visco license) */
	DRIVER( neo_mosyougi )	/* 0203 (c) 1995 ADK / SNK */
	/* 0204 ADK World (CD only) */
	/* 0205 Neo-Geo CD Special (CD only) */
	DRIVER( neo_marukodq )	/* 0206 (c) 1995 Takara */
	DRIVER( neo_neomrdo )	/* 0207 (c) 1996 Visco */
	DRIVER( neo_sdodgeb )	/* 0208 (c) 1996 Technos */
	DRIVER( neo_goalx3 )	/* 0209 (c) 1995 Visco */
	/* 0210 */
	/* 0211 Oshidashi Zintrick (CD only? not confirmed, MVS might exist) */
	DRIVER( neo_overtop )	/* 0212 (c) 1996 ADK */
	DRIVER( neo_neodrift )	/* 0213 (c) 1996 Visco */
	DRIVER( neo_kof96 )		/* 0214 (c) 1996 SNK */
	DRIVER( neo_ssideki4 )	/* 0215 (c) 1996 SNK */
	DRIVER( neo_kizuna )	/* 0216 (c) 1996 SNK */
	DRIVER( neo_ninjamas )	/* 0217 (c) 1996 ADK / SNK */
	DRIVER( neo_ragnagrd )	/* 0218 (c) 1996 Saurus */
	DRIVER( neo_pgoal )		/* 0219 (c) 1996 Saurus */
	/* 0220 Choutetsu Brikin'ger - iron clad (CD only? not confirmed, MVS might exist) */
	DRIVER( neo_magdrop2 )	/* 0221 (c) 1996 Data East Corporation */
	DRIVER( neo_samsho4 )	/* 0222 (c) 1996 SNK */
	DRIVER( neo_rbffspec )	/* 0223 (c) 1996 SNK */
	DRIVER( neo_twinspri )	/* 0224 (c) 1996 ADK */
	DRIVER( neo_wakuwak7 )	/* 0225 (c) 1996 Sunsoft */
	/* 0226 */
	DRIVER( neo_stakwin2 )	/* 0227 (c) 1996 Saurus */
	/* 0228 */
	/* 0229 King of Fighters '96 CD Collection (CD only) */
	DRIVER( neo_breakers )	/* 0230 (c) 1996 Visco */
	DRIVER( neo_miexchng )	/* 0231 (c) 1997 Face */
	DRIVER( neo_kof97 )		/* 0232 (c) 1997 SNK */
	DRIVER( neo_magdrop3 )	/* 0233 (c) 1997 Data East Corporation */
	DRIVER( neo_lastblad )	/* 0234 (c) 1997 SNK */
	DRIVER( neo_puzzldpr )	/* 0235 (c) 1997 Taito (Visco license) */
	DRIVER( neo_irrmaze )	/* 0236 (c) 1997 SNK / Saurus */
	DRIVER( neo_popbounc )	/* 0237 (c) 1997 Video System Co. */
	DRIVER( neo_shocktro )	/* 0238 (c) 1997 Saurus */
	DRIVER( neo_blazstar )	/* 0239 (c) 1998 Yumekobo */
	DRIVER( neo_rbff2 )		/* 0240 (c) 1998 SNK */
	DRIVER( neo_mslug2 )	/* 0241 (c) 1998 SNK */
	DRIVER( neo_kof98 )		/* 0242 (c) 1998 SNK */
	DRIVER( neo_lastbld2 )	/* 0243 (c) 1998 SNK */
	DRIVER( neo_neocup98 )	/* 0244 (c) 1998 SNK */
	DRIVER( neo_breakrev )	/* 0245 (c) 1998 Visco */
	DRIVER( neo_shocktr2 )	/* 0246 (c) 1998 Saurus */
	DRIVER( neo_flipshot )	/* 0247 (c) 1998 Visco */
TESTDRIVER( neo_pbobbl2n )	/* 0248 (c) 1999 Taito (SNK license) */
TESTDRIVER( neo_ctomaday )	/* 0249 (c) 1999 Visco */
TESTDRIVER( neo_mslugx )	/* 0250 (c) 1999 SNK */
//	DRIVER( neo_kof99 )		/* 0251 (c) 1999 SNK */

#endif	/* NEOFREE */

#endif	/* DRIVER_RECURSIVE */

#endif	/* TINY_COMPILE */
