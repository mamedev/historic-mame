#include "driver.h"


/* "Pacman hardware" games */
extern struct GameDriver pacman_driver;		/* (c) 1980 Midway */
extern struct GameDriver pacplus_driver;	/* hack */
extern struct GameDriver jrpacman_driver;	/* (c) 1983 Midway */
extern struct GameDriver pacmod_driver;		/* (c) 1981 Midway */
extern struct GameDriver namcopac_driver;	/* (c) 1980 Namco */
extern struct GameDriver pacmanjp_driver;	/* (c) 1980 Namco */
extern struct GameDriver hangly_driver;		/* hack */
extern struct GameDriver puckman_driver;	/* hack */
extern struct GameDriver piranha_driver;	/* hack */
extern struct GameDriver mspacman_driver;	/* (c) 1981 Midway */
extern struct GameDriver mspacatk_driver;	/* hack */
extern struct GameDriver crush_driver;		/* bootleg Make Trax */
extern struct GameDriver maketrax_driver;	/* (c) 1981 Williams */
extern struct GameDriver pengo_driver;		/* (c) 1982 Sega */
extern struct GameDriver pengoa_driver;		/* (c) 1982 Sega */
extern struct GameDriver penta_driver;		/* bootleg */
/* "Galaxian hardware" games */
extern struct GameDriver galaxian_driver;	/* (c) Namco */
extern struct GameDriver galmidw_driver;	/* (c) Midway */
extern struct GameDriver galnamco_driver;	/* hack */
extern struct GameDriver superg_driver;		/* hack */
extern struct GameDriver galapx_driver;		/* hack */
extern struct GameDriver galap1_driver;		/* hack */
extern struct GameDriver galap4_driver;		/* hack */
extern struct GameDriver galturbo_driver;	/* hack */
extern struct GameDriver pisces_driver;		/* ? */
extern struct GameDriver japirem_driver;	/* (c) Irem */
extern struct GameDriver uniwars_driver;	/* (c) Karateco (bootleg?) */
extern struct GameDriver warofbug_driver;	/* (c) 1981 Armenia */
extern struct GameDriver redufo_driver;		/* ? */
extern struct GameDriver pacmanbl_driver;	/* bootleg */
extern struct GameDriver mooncrst_driver;	/* (c) 1980 Nihon Bussan */
extern struct GameDriver mooncrsg_driver;	/* (c) 1980 Gremlin */
extern struct GameDriver mooncrsb_driver;	/* bootleg */
extern struct GameDriver fantazia_driver;	/* bootleg */
extern struct GameDriver moonqsr_driver;	/* (c) 1980 Nichibutsu */
/* "Scramble hardware" (and variations) games */
extern struct GameDriver scramble_driver;	/* (c) 1981 Stern */
extern struct GameDriver atlantis_driver;	/* (c) 1981 Comsoft */
extern struct GameDriver theend_driver;		/* (c) 1980 Stern */
extern struct GameDriver ckongs_driver;		/* bootleg */
extern struct GameDriver froggers_driver;	/* bootleg */
extern struct GameDriver amidars_driver;	/* (c) 1982 Konami */
extern struct GameDriver triplep_driver;	/* (c) 1982 KKI */
extern struct GameDriver scobra_driver;		/* (c) 1981 Stern */
extern struct GameDriver scobrak_driver;	/* (c) 1981 Konami */
extern struct GameDriver scobrab_driver;	/* (c) 1981 Karateco (bootleg?) */
extern struct GameDriver losttomb_driver;	/* (c) 1982 Stern */
extern struct GameDriver anteater_driver;	/* (c) 1982 Tago */
extern struct GameDriver rescue_driver;		/* (c) 1982 Stern */
extern struct GameDriver minefld_driver;	/* (c) 1983 Stern */
extern struct GameDriver armorcar_driver;	/* (c) 1981 Stern */
extern struct GameDriver frogger_driver;	/* (c) 1981 Sega */
extern struct GameDriver frogsega_driver;	/* (c) 1981 Sega */
extern struct GameDriver amidar_driver;		/* (c) 1982 Konami + Stern license */
extern struct GameDriver amidarjp_driver;	/* (c) 1981 Konami */
extern struct GameDriver amigo_driver;		/* bootleg */
extern struct GameDriver turtles_driver;	/* (c) 1981 Stern */
extern struct GameDriver turpin_driver;		/* (c) 1981 Sega */
extern struct GameDriver jumpbug_driver;	/* (c) 1981 Rock-ola */
extern struct GameDriver jbugsega_driver;	/* (c) 1981 Sega */
/* "Crazy Climber hardware" games */
extern struct GameDriver cclimber_driver;	/* (c) 1980 Nihon Bussan */
extern struct GameDriver ccjap_driver;		/* (c) 1980 Nihon Bussan */
extern struct GameDriver ccboot_driver;		/* bootleg */
extern struct GameDriver ckong_driver;		/* (c) 1981 Falcon */
extern struct GameDriver ckonga_driver;		/* (c) 1981 Falcon */
extern struct GameDriver ckongjeu_driver;	/* bootleg */
extern struct GameDriver ckongalc_driver;	/* bootleg */
extern struct GameDriver seicross_driver;	/* (c) 1984 Nichibutsu + Alice */
extern struct GameDriver swimmer_driver;	/* (c) 1982 Tehkan */
/* "Phoenix hardware" (and variations) games */
extern struct GameDriver phoenix_driver;	/* (c) 1980 Amstar */
extern struct GameDriver phoenixt_driver;	/* (c) 1980 Taito */
extern struct GameDriver phoenix3_driver;	/* T.P.N. */
extern struct GameDriver pleiads_driver;	/* (c) 1981 Centuri + Tehkan */
extern struct GameDriver naughtyb_driver;	/* (c) 1982 Jaleco + Cinematronics */
extern struct GameDriver popflame_driver;	/* (c) 1982 Jaleco */
/* "Space Invaders hardware " games */
extern struct GameDriver invaders_driver;
extern struct GameDriver earthinv_driver;
extern struct GameDriver spaceatt_driver;
extern struct GameDriver invrvnge_driver;
extern struct GameDriver invdelux_driver;
extern struct GameDriver galxwars_driver;
extern struct GameDriver lrescue_driver;
extern struct GameDriver desterth_driver;
extern struct GameDriver invadpt2_driver;
extern struct GameDriver cosmicmo_driver;
extern struct GameDriver spaceph_driver;
extern struct GameDriver rollingc_driver;
/* Namco games */
extern struct GameDriver galaga_driver;		/* (c) 1981 Midway */
extern struct GameDriver galaganm_driver;	/* (c) 1981 */
extern struct GameDriver galagabl_driver;	/* bootleg */
extern struct GameDriver gallag_driver;		/* bootleg */
extern struct GameDriver bosco_driver;		/* (c) 1981 Midway */
extern struct GameDriver bosconm_driver;	/* (c) 1981 */
extern struct GameDriver digdugnm_driver;	/* (c) 1982 */
extern struct GameDriver digdugat_driver;	/* (c) 1982 Atari */
extern struct GameDriver xevious_driver;	/* (c) 1982 + Atari license */
extern struct GameDriver xeviousn_driver;	/* (c) 1982 */
extern struct GameDriver sxevious_driver;	/* (c) 1984 */
extern struct GameDriver superpac_driver;	/* (c) 1982 Midway */
extern struct GameDriver superpcn_driver;	/* (c) 1982 */
extern struct GameDriver pacnpal_driver;	/* (c) 1983 */
extern struct GameDriver mappy_driver;		/* (c) 1983 */
extern struct GameDriver mappyjp_driver;	/* (c) 1983 */
extern struct GameDriver digdug2_driver;	/* (c) 1985 */
extern struct GameDriver todruaga_driver;	/* (c) 1984 */
extern struct GameDriver motos_driver;		/* (c) 1985 */
/* Universal games */
extern struct GameDriver ladybug_driver;	/* (c) 1981 */
extern struct GameDriver snapjack_driver;	/* (c) */
extern struct GameDriver cavenger_driver;	/* (c) 1981 */
extern struct GameDriver mrdo_driver;		/* (c) 1982 */
extern struct GameDriver mrdot_driver;		/* (c) 1982 + Taito license */
extern struct GameDriver mrlo_driver;		/* bootleg */
extern struct GameDriver mrdu_driver;		/* bootleg */
extern struct GameDriver docastle_driver;	/* (c) 1983 */
extern struct GameDriver docastl2_driver;	/* (c) 1983 */
extern struct GameDriver dounicorn_driver;	/* (c) 1983 */
extern struct GameDriver dowild_driver;		/* (c) 1984 */
extern struct GameDriver jjack_driver;		/* (c) 1984 */
extern struct GameDriver dorunrun_driver;	/* (c) 1984 */
extern struct GameDriver spiero_driver;		/* (c) 1987 */
extern struct GameDriver kickridr_driver;	/* (c) 1984 */
/* Nintendo games */
extern struct GameDriver dkong_driver;		/* (c) 1981 Nintendo of America */
extern struct GameDriver radarscp_driver;	/* (c) 1980 Nintendo */
extern struct GameDriver dkongjp_driver;	/* (c) 1981 Nintendo */
extern struct GameDriver dkongjr_driver;	/* (c) 1982 Nintendo of America */
extern struct GameDriver dkngjrjp_driver;	/* no copyright notice */
extern struct GameDriver dkjrjp_driver;		/* (c) 1982 Nintendo */
extern struct GameDriver dkjrbl_driver;		/* (c) 1982 Nintendo of America */
extern struct GameDriver dkong3_driver;		/* (c) 1983 Nintendo of America */
extern struct GameDriver mario_driver;		/* (c) 1983 Nintendo of America */
extern struct GameDriver hunchy_driver;		/* hacked Donkey Kong board */
extern struct GameDriver popeyebl_driver;	/* bootleg - different hardware from the other ones */
/* Bally Midway "Astrocade" games */
extern struct GameDriver wow_driver;		/* (c) 1980 */
extern struct GameDriver robby_driver;		/* (c) 1981 */
extern struct GameDriver gorf_driver;		/* (c) 1981 */
extern struct GameDriver seawolf_driver;
extern struct GameDriver spacezap_driver;	/* (c) 1980 */
/* Bally Midway "MCR" games */
extern struct GameDriver kick_driver;		/* (c) 1981 */
extern struct GameDriver solarfox_driver;	/* (c) 1981 */
extern struct GameDriver tron_driver;		/* (c) 1982 */
extern struct GameDriver twotiger_driver;	/* (c) 1984 */
extern struct GameDriver domino_driver;		/* (c) 1982 */
extern struct GameDriver shollow_driver;	/* (c) 1981 */
extern struct GameDriver wacko_driver;		/* (c) 1982 */
extern struct GameDriver kroozr_driver;		/* (c) 1982 */
extern struct GameDriver journey_driver;	/* (c) 1983 */
extern struct GameDriver tapper_driver;		/* (c) 1983 */
extern struct GameDriver rbtapper_driver;	/* (c) 1984 */
extern struct GameDriver dotron_driver;		/* (c) 1983 */
extern struct GameDriver destderb_driver;	/* (c) 1984 */
extern struct GameDriver timber_driver;		/* (c) 1984 */
extern struct GameDriver spyhunt_driver;	/* (c) 1983 */
extern struct GameDriver rampage_driver;	/* (c) 1986 */
extern struct GameDriver sarge_driver;		/* (c) 1985 */
/* Irem games */
extern struct GameDriver mpatrol_driver;	/* (c) 1982 + Williams license */
extern struct GameDriver mranger_driver;	/* bootleg */
extern struct GameDriver yard_driver;		/* (c) 1983 */
extern struct GameDriver vsyard_driver;		/* (c) 1983/1984 */
extern struct GameDriver kungfum_driver;	/* (c) 1984 */
extern struct GameDriver kungfub_driver;	/* bootleg */
extern struct GameDriver travrusa_driver;	/* (c) 1983 */
/* Gottlieb games */
extern struct GameDriver qbert_driver;		/* (c) 1982 */
extern struct GameDriver qbertjp_driver;	/* (c) 1982 + Konami license */
extern struct GameDriver qbertqub_driver;	/* (c) 1983 Mylstar */
extern struct GameDriver krull_driver;		/* (c) 1983 */
extern struct GameDriver reactor_driver;	/* (c) 1982 */
extern struct GameDriver mplanets_driver;	/* (c) 1983 */
extern struct GameDriver stooges_driver;	/* (c) 1984 Mylstar */
/* Taito "Qix hardware" games */
extern struct GameDriver qix_driver;		/* (c) 1981 */
extern struct GameDriver qix2_driver;		/* (c) 1981 */
extern struct GameDriver zookeep_driver;	/* (c) 1982 Taito America */
/* Taito "Jungle King hardware" games */
extern struct GameDriver junglek_driver;	/* (c) 1982 */
extern struct GameDriver jhunt_driver;		/* (c) 1982 Taito America */
extern struct GameDriver elevator_driver;	/* (c) 1983 */
extern struct GameDriver elevatob_driver;	/* bootleg */
extern struct GameDriver wwestern_driver;	/* (c) 1982 */
extern struct GameDriver frontlin_driver;	/* (c) 1982 */
extern struct GameDriver alpine_driver;
/* Williams games */
extern struct GameDriver robotron_driver;	/* (c) 1982 */
extern struct GameDriver robotryo_driver;	/* (c) 1982 */
extern struct GameDriver stargate_driver;	/* (c) 1981 */
extern struct GameDriver joust_driver;		/* (c) 1982 */
extern struct GameDriver joustr_driver;		/* (c) 1982 */
extern struct GameDriver joustg_driver;		/* (c) 1982 */
extern struct GameDriver joustwr_driver;	/* (c) 1982 */
extern struct GameDriver sinistar_driver;	/* (c) 1982 */
extern struct GameDriver bubbles_driver;	/* (c) 1982 */
extern struct GameDriver bubblesr_driver;	/* (c) 1982 */
extern struct GameDriver defender_driver;	/* (c) 1980 */
extern struct GameDriver splat_driver;		/* (c) 1982 */
extern struct GameDriver blaster_driver;	/* (c) 1983 */
extern struct GameDriver colony7_driver;	/* (c) 1981 Taito */
extern struct GameDriver colony7a_driver;	/* (c) 1981 Taito */
/* Capcom games */
extern struct GameDriver c1942_driver;		/* (c) 1984 */
extern struct GameDriver vulgus_driver;		/* (c) 1984 */
extern struct GameDriver commando_driver;	/* (c) 1985 */
extern struct GameDriver commandj_driver;	/* (c) 1985 */
extern struct GameDriver gng_driver;		/* (c) 1985 */
extern struct GameDriver gngcross_driver;	/* (c) 1985 */
extern struct GameDriver gngjap_driver;		/* (c) 1985 */
extern struct GameDriver diamond_driver;	/* (c) 1989 KH Video (runs on GnG hardware) */
extern struct GameDriver sonson_driver;		/* (c) 1984 */
extern struct GameDriver exedexes_driver;	/* (c) 1985 */
extern struct GameDriver lwings_driver;		/* (c) 1986 */
extern struct GameDriver lwingsjp_driver;	/* (c) 1986 */
extern struct GameDriver sectionz_driver;	/* (c) 1985 */
extern struct GameDriver c1943_driver;		/* (c) 1987 */
extern struct GameDriver c1943kai_driver;	/* (c) 1987 */
extern struct GameDriver gunsmoke_driver;	/* (c) 1985 */
extern struct GameDriver gunsmrom_driver;	/* (c) 1985 + Romstar */
extern struct GameDriver gunsmokj_driver;	/* (c) 1985 */
extern struct GameDriver blktiger_driver;	/* (c) 1987 */
extern struct GameDriver blkdrgon_driver;	/* (c) 1987 */
extern struct GameDriver sidearms_driver;	/* (c) 1986 */
extern struct GameDriver sidearjp_driver;	/* (c) 1986 */
extern struct GameDriver capbowl_driver;	/* (c) 1989 Incredible Technologies - very different hardware */
/* Sega "dual game board" games */
extern struct GameDriver carnival_driver;	/* (c) 1980 */
extern struct GameDriver pulsar_driver;		/* (c) 1981 */
extern struct GameDriver invho2_driver;		/* (c) 1979 */
extern struct GameDriver sspaceat_driver;	/* (c) */
extern struct GameDriver invinco_driver;	/* (c) 1979 */
/* Sega G-80 vector games */
extern struct GameDriver spacfury_driver;	/* no copyright notice */
extern struct GameDriver spacfurc_driver;	/* (c) 1981 */
extern struct GameDriver zektor_driver;		/* (c) 1982 */
extern struct GameDriver tacscan_driver;	/* (c) */
extern struct GameDriver elim2_driver;		/* (c) 1981 Gremlin */
extern struct GameDriver elim4_driver;
extern struct GameDriver startrek_driver;	/* (c) 1982 */
/* Sega G-80 raster games */
extern struct GameDriver astrob_driver;		/* (c) 1981 */
extern struct GameDriver astrob1_driver;	/* (c) 1981 */
extern struct GameDriver s005_driver;		/* (c) 1981 */
extern struct GameDriver monsterb_driver;	/* (c) 1982 */
extern struct GameDriver spaceod_driver;	/* (c) 1981 */
/* Sega "Zaxxon hardware" games */
extern struct GameDriver zaxxon_driver;		/* (c) 1982 */
extern struct GameDriver szaxxon_driver;	/* (c) 1982 */
extern struct GameDriver futspy_driver;
extern struct GameDriver congo_driver;		/* (c) 1983 */
extern struct GameDriver tiptop_driver;		/* (c) 1983 */
/* Data East "Burger Time hardware" games */
extern struct GameDriver btime_driver;		/* (c) 1982 + Midway */
extern struct GameDriver btimea_driver;		/* (c) 1982 */
extern struct GameDriver bnj_driver;		/* (c) 1982 + Midway */
extern struct GameDriver brubber_driver;	/* (c) 1982 */
extern struct GameDriver eggs_driver;		/* (c) 1983 Universal USA */
	/* other games running on this hardware: Zoar */
/* other Data East games */
extern struct GameDriver astrof_driver;		/* (c) [1980?] */
extern struct GameDriver astrof2_driver;	/* (c) [1980?] */
extern struct GameDriver firetrap_driver;
extern struct GameDriver firetpbl_driver;
extern struct GameDriver brkthru_driver;	/* (c) 1986 */
/* Tehkan games */
extern struct GameDriver bombjack_driver;	/* (c) 1984 */
extern struct GameDriver starforc_driver;	/* (c) 1984 */
extern struct GameDriver megaforc_driver;	/* (c) 1985 + Videoware license */
extern struct GameDriver pbaction_driver;	/* (c) 1985 */
/* Konami games */
extern struct GameDriver pooyan_driver;		/* (c) 1982 Stern */
extern struct GameDriver pootan_driver;		/* bootleg */
extern struct GameDriver timeplt_driver;	/* (c) 1982 */
extern struct GameDriver spaceplt_driver;	/* bootleg */
extern struct GameDriver gyruss_driver;		/* (c) 1983 */
extern struct GameDriver gyrussce_driver;	/* (c) 1983 + Centuri license */
extern struct GameDriver sbasketb_driver;	/* (c) 1984 */
extern struct GameDriver tp84_driver;		/* (c) 1984 */
extern struct GameDriver gberet_driver;		/* (c) 1985 */
extern struct GameDriver ironhors_driver;	/* (c) 1986 */
extern struct GameDriver rushatck_driver;	/* (c) 1985 */
extern struct GameDriver yiear_driver;		/* (c) 1985 */
extern struct GameDriver mikie_driver;		/* (c) 1984 */
extern struct GameDriver shaolins_driver;	/* (c) 1985 */
extern struct GameDriver trackfld_driver;	/* (c) 1983 */
extern struct GameDriver hyprolym_driver;	/* (c) 1983 */
extern struct GameDriver hyperspt_driver;	/* (c) 1984 + Centuri */
extern struct GameDriver rocnrope_driver;	/* (c) 1983 + Kosuka */
extern struct GameDriver ropeman_driver;	/* bootleg */
extern struct GameDriver circusc_driver;	/* (c) 1984 */
extern struct GameDriver circusc2_driver;	/* (c) 1984 */
/* Exidy games */
extern struct GameDriver venture_driver;	/* (c) 1981 */
extern struct GameDriver mtrap_driver;		/* (c) 1981 */
extern struct GameDriver pepper2_driver;	/* (c) 1982 */
extern struct GameDriver targ_driver;		/* (c) 1980 */
extern struct GameDriver spectar_driver;	/* (c) 1980 */
/* Atari vector games */
extern struct GameDriver asteroid_driver;	/* no copyright notice */
extern struct GameDriver asteroi2_driver;	/* (c) 1979 */
extern struct GameDriver astdelux_driver;	/* (c) 1980 */
extern struct GameDriver bwidow_driver;		/* (c) 1982 */
extern struct GameDriver bzone_driver;		/* (c) 1980 */
extern struct GameDriver bzone2_driver;		/* (c) 1980 */
extern struct GameDriver gravitar_driver;	/* (c) 1982 */
extern struct GameDriver llander_driver;	/* no copyright notice */
extern struct GameDriver redbaron_driver;	/* (c) 1980 */
extern struct GameDriver spacduel_driver;	/* (c) 1980 */
extern struct GameDriver tempest_driver;	/* (c) 1980 */
extern struct GameDriver temptube_driver;	/* hack */
extern struct GameDriver starwars_driver;	/* (c) 1983 */
extern struct GameDriver mhavoc_driver;		/* (c) 1983 */
extern struct GameDriver mhavoc2_driver;	/* (c) 1983 */
extern struct GameDriver mhavocrv_driver;	/* hack */
extern struct GameDriver quantum_driver;	/* (c) 1982 */
extern struct GameDriver quantum2_driver;	/* (c) 1982 */
/* Atari "Centipede hardware" games */
extern struct GameDriver centiped_driver;	/* (c) 1980 */
extern struct GameDriver milliped_driver;	/* (c) 1982 */
extern struct GameDriver warlord_driver;	/* (c) 1980 */
/* Atari "Kangaroo hardware" games */
extern struct GameDriver kangaroo_driver;	/* (c) 1982 */
extern struct GameDriver arabian_driver;	/* (c) 1983 */
/* Rock-ola games */
extern struct GameDriver vanguard_driver;	/* (c) 1981 SNK */
extern struct GameDriver nibbler_driver;	/* (c) 1982 */
extern struct GameDriver fantasy_driver;	/* (c) 1981 */
extern struct GameDriver warpwarp_driver;	/* (c) 1981 - different hardware */
/* Technos games */
extern struct GameDriver mystston_driver;	/* (c) 1984 */
extern struct GameDriver matmania_driver;	/* (c) 1985 + Taito America license */
extern struct GameDriver excthour_driver;	/* (c) 1985 + Taito license */
/* Stern "Berzerk hardware" games */
extern struct GameDriver berzerk_driver;	/* (c) 1980 */
extern struct GameDriver berzerk1_driver;	/* (c) 1980 */
extern struct GameDriver frenzy_driver;		/* (c) 1982 */
extern struct GameDriver frenzy1_driver;	/* (c) 1982 */


extern struct GameDriver bagman_driver;		/* (c) 1982 Stern + Valadon license */
extern struct GameDriver sbagman_driver;	/* (c) 1984 Valadon + "Stern presents" */
extern struct GameDriver rallyx_driver;		/* (c) 1980 Midway */
extern struct GameDriver nrallyx_driver;	/* (c) 1981 Namco */
extern struct GameDriver locomotn_driver;	/* (c) 1982 Centuri + Konami license */
extern struct GameDriver panic_driver;		/* (c) 1980 Universal */
extern struct GameDriver panica_driver;		/* (c) 1980 Universal */
extern struct GameDriver spacefb_driver;	/* (c) Nintendo */
extern struct GameDriver tutankhm_driver;	/* (c) 1982 Stern */
extern struct GameDriver ccastles_driver;	/* (c) 1983 Atari */
extern struct GameDriver missile_driver;	/* (c) 1980 Atari */
extern struct GameDriver bublbobl_driver;	/* (c) Taito 1986 */
extern struct GameDriver boblbobl_driver;	/* bootleg */
extern struct GameDriver sboblbob_driver;	/* bootleg */
extern struct GameDriver blueprnt_driver;	/* (c) 1982 Bally Midway */
extern struct GameDriver omegrace_driver;	/* (c) 1981 Midway */
extern struct GameDriver bankp_driver;		/* (c) 1984 Sega */
extern struct GameDriver espial_driver;		/* (c) 1983 Thunderbolt */
extern struct GameDriver rastan_driver;		/* (c) 1987 Taito Japan */
extern struct GameDriver rastsaga_driver;	/* (c) 1987 Taito */
extern struct GameDriver cloak_driver;		/* (c) 1983 Atari */
extern struct GameDriver silkworm_driver;	/* (c) 1988 Tecmo */
extern struct GameDriver champbas_driver;	/* (c) 1983 Sega */
extern struct GameDriver punchout_driver;
extern struct GameDriver exerion_driver;	/* (c) 1983 Jaleco */
extern struct GameDriver arkanoid_driver;	/* (c) 1986 Taito */
extern struct GameDriver arknoidu_driver;	/* (c) 1986 Taito America */
extern struct GameDriver gauntlet_driver;	/* (c) 1985 Atari */
extern struct GameDriver gaunt2_driver;		/* (c) 1986 Atari */
extern struct GameDriver foodf_driver;		/* (c) 1982 Atari */
extern struct GameDriver circus_driver;		/* no copyright notice */
extern struct GameDriver robotbwl_driver;	/* no copyright notice */
extern struct GameDriver sprint2_driver;	/* no copyright notice */
extern struct GameDriver jack_driver;
extern struct GameDriver vastar_driver;		/* (c) 1983 Sesame Japan */



const struct GameDriver *drivers[] =
{
	/* "Pacman hardware" games */
	&pacman_driver,
	&pacplus_driver,
	&jrpacman_driver,
	&pacmod_driver,
	&namcopac_driver,
	&pacmanjp_driver,
	&hangly_driver,
	&puckman_driver,
	&piranha_driver,
	&mspacman_driver,
	&mspacatk_driver,
	&crush_driver,
	&maketrax_driver,
	&pengo_driver,
	&pengoa_driver,
	&penta_driver,
	/* "Galaxian hardware" games */
	&galaxian_driver,
	&galmidw_driver,
	&galnamco_driver,
	&superg_driver,
	&galapx_driver,
	&galap1_driver,
	&galap4_driver,
	&galturbo_driver,
	&pisces_driver,
	&japirem_driver,
	&uniwars_driver,
	&warofbug_driver,
	&redufo_driver,
	&pacmanbl_driver,
	&mooncrst_driver,
	&mooncrsg_driver,
	&mooncrsb_driver,
	&fantazia_driver,
	&moonqsr_driver,
	/* "Scramble hardware" (and variations) games */
	&scramble_driver,
	&atlantis_driver,
	&theend_driver,
	&ckongs_driver,
	&froggers_driver,
	&amidars_driver,
	&triplep_driver,
	&scobra_driver,
	&scobrak_driver,
	&scobrab_driver,
	&losttomb_driver,
	&anteater_driver,
	&rescue_driver,
	&minefld_driver,
	&armorcar_driver,
	&frogger_driver,
	&frogsega_driver,
	&amidar_driver,
	&amidarjp_driver,
	&amigo_driver,
	&turtles_driver,
	&turpin_driver,
	&jumpbug_driver,
	&jbugsega_driver,
	/* "Crazy Climber hardware" games */
	&cclimber_driver,
	&ccjap_driver,
	&ccboot_driver,
	&ckong_driver,
	&ckonga_driver,
	&ckongjeu_driver,
	&ckongalc_driver,
	&seicross_driver,
	&swimmer_driver,
	/* "Phoenix hardware" (and variations) games */
	&phoenix_driver,
	&phoenixt_driver,
	&phoenix3_driver,
	&pleiads_driver,
	&naughtyb_driver,
	&popflame_driver,
	/* "Space Invaders hardware" games */
	&invaders_driver,
	&earthinv_driver,
	&spaceatt_driver,
	&invrvnge_driver,
	&invdelux_driver,
	&galxwars_driver,
	&lrescue_driver,
	&desterth_driver,
	&invadpt2_driver,
	&cosmicmo_driver,
	&spaceph_driver,
	&rollingc_driver,
	/* Namco games */
	&galaga_driver,
	&galaganm_driver,
	&galagabl_driver,
	&gallag_driver,
	&bosco_driver,
	&bosconm_driver,
	&digdugnm_driver,
	&digdugat_driver,
	&xevious_driver,
	&xeviousn_driver,
	&sxevious_driver,
	&superpac_driver,
	&superpcn_driver,
	&pacnpal_driver,
	&mappy_driver,
	&mappyjp_driver,
	&digdug2_driver,
	&todruaga_driver,
	&motos_driver,
	/* Universal games */
	&ladybug_driver,
	&snapjack_driver,
	&cavenger_driver,
	&mrdo_driver,
	&mrdot_driver,
	&mrlo_driver,
	&mrdu_driver,
	&docastle_driver,
	&docastl2_driver,
	&dounicorn_driver,
	&dowild_driver,
	&jjack_driver,
	&dorunrun_driver,
	&spiero_driver,
	&kickridr_driver,
	/* Nintendo games */
	&dkong_driver,
	&radarscp_driver,
	&dkongjp_driver,
	&dkongjr_driver,
	&dkngjrjp_driver,
	&dkjrjp_driver,
	&dkjrbl_driver,
	&dkong3_driver,
	&mario_driver,
	&hunchy_driver,
	&popeyebl_driver,
	/* Bally Midway "Astrocade" games */
	&wow_driver,
	&robby_driver,
	&gorf_driver,
	&seawolf_driver,
	&spacezap_driver,
	/* Bally Midway "MCR" games */
	&kick_driver,
	&solarfox_driver,
	&tron_driver,
	&twotiger_driver,
	&domino_driver,
	&shollow_driver,
	&wacko_driver,
	&kroozr_driver,
	&journey_driver,
	&tapper_driver,
	&rbtapper_driver,
	&dotron_driver,
	&destderb_driver,
	&timber_driver,
	&spyhunt_driver,
	&rampage_driver,
	&sarge_driver,
	/* Irem games */
	&mpatrol_driver,
	&mranger_driver,
	&yard_driver,
	&vsyard_driver,
	&kungfum_driver,
	&kungfub_driver,
	&travrusa_driver,
	/* Gottlieb games */
	&qbert_driver,
	&qbertjp_driver,
	&qbertqub_driver,
	&krull_driver,
	&reactor_driver,
	&mplanets_driver,
	&stooges_driver,
	/* Taito "Qix hardware" games */
	&qix_driver,
	&qix2_driver,
	&zookeep_driver,
	/* Taito "Jungle King hardware" games */
	&junglek_driver,
	&jhunt_driver,
	&elevator_driver,
	&elevatob_driver,
	&wwestern_driver,
	&frontlin_driver,
	&alpine_driver,
	/* Williams games */
	&robotron_driver,
	&robotryo_driver,
	&stargate_driver,
	&joust_driver,
	&joustr_driver,
	&joustg_driver,
	&joustwr_driver,
	&sinistar_driver,
	&bubbles_driver,
	&bubblesr_driver,
	&defender_driver,
	&splat_driver,
	&blaster_driver,
	&colony7_driver,
	&colony7a_driver,
	/* Capcom games */
	&c1942_driver,
	&vulgus_driver,
	&commando_driver,
	&commandj_driver,
	&gng_driver,
	&gngcross_driver,
	&gngjap_driver,
	&diamond_driver,
	&sonson_driver,
	&exedexes_driver,
	&lwings_driver,
	&lwingsjp_driver,
	&sectionz_driver,
	&c1943_driver,
	&c1943kai_driver,
	&gunsmoke_driver,
	&gunsmrom_driver,
	&gunsmokj_driver,
	&blktiger_driver,
	&blkdrgon_driver,
	&capbowl_driver,
	&sidearms_driver,
	&sidearjp_driver,
	/* Sega "dual game board" games */
	&carnival_driver,
	&pulsar_driver,
	&invho2_driver,
	&sspaceat_driver,
	&invinco_driver,
	/* Sega G-80 vector games */
	&spacfury_driver,
	&spacfurc_driver,
	&zektor_driver,
	&tacscan_driver,
	&elim2_driver,
	&elim4_driver,
	&startrek_driver,
	/* Sega G-80 raster games */
	&astrob_driver,
	&astrob1_driver,
	&s005_driver,
	&monsterb_driver,
	&spaceod_driver,
	/* Sega "Zaxxon hardware" games */
	&zaxxon_driver,
	&szaxxon_driver,
	&futspy_driver,
	&congo_driver,
	&tiptop_driver,
	/* Data East games */
	&btime_driver,
	&btimea_driver,
	&bnj_driver,
	&brubber_driver,
	&eggs_driver,
	/* other Data East games */
	&astrof_driver,
	&astrof2_driver,
	&firetrap_driver,
	&firetpbl_driver,
	&brkthru_driver,
	/* Tehkan games */
	&bombjack_driver,
	&starforc_driver,
	&megaforc_driver,
	&pbaction_driver,
	/* Konami games */
	&pooyan_driver,
	&pootan_driver,
	&timeplt_driver,
	&spaceplt_driver,
	&gyruss_driver,
	&gyrussce_driver,
	&sbasketb_driver,
	&tp84_driver,
	&gberet_driver,
	&ironhors_driver,
	&rushatck_driver,
	&yiear_driver,
	&mikie_driver,
	&shaolins_driver,
	&trackfld_driver,
	&hyprolym_driver,
	&hyperspt_driver,
	&rocnrope_driver,
	&ropeman_driver,
	&circusc_driver,
	&circusc2_driver,
	/* Exidy games */
	&venture_driver,
	&mtrap_driver,
	&pepper2_driver,
	&targ_driver,
	&spectar_driver,
	/* Atari vector games */
	&asteroid_driver,
	&asteroi2_driver,
	&astdelux_driver,
	&bwidow_driver,
	&bzone_driver,
	&bzone2_driver,
	&gravitar_driver,
	&llander_driver,
	&redbaron_driver,
	&spacduel_driver,
	&tempest_driver,
	&temptube_driver,
	&starwars_driver,
	&mhavoc_driver,
	&mhavoc2_driver,
	&mhavocrv_driver,
	&quantum_driver,
	&quantum2_driver,
	/* Atari "Centipede hardware" games */
	&centiped_driver,
	&milliped_driver,
	&warlord_driver,
	/* Atari "Kangaroo hardware" games */
	&kangaroo_driver,
	&arabian_driver,
	/* Rock-ola games */
	&vanguard_driver,
	&nibbler_driver,
	&fantasy_driver,
	&warpwarp_driver,
	/* Technos games */
	&mystston_driver,
	&matmania_driver,
	&excthour_driver,
	/* Stern "Berzerk hardware" games */
	&berzerk_driver,
	&berzerk1_driver,
	&frenzy_driver,
	&frenzy1_driver,


	&bagman_driver,
	&sbagman_driver,
	&rallyx_driver,
	&nrallyx_driver,
	&locomotn_driver,
	&panic_driver,
	&panica_driver,
	&spacefb_driver,
	&tutankhm_driver,
	&ccastles_driver,
	&missile_driver,
	&bublbobl_driver,
	&boblbobl_driver,
	&sboblbob_driver,
	&blueprnt_driver,
	&omegrace_driver,
	&bankp_driver,
	&espial_driver,
	&rastan_driver,
	&rastsaga_driver,
	&cloak_driver,
	&silkworm_driver,
	&champbas_driver,
	&punchout_driver,
	&exerion_driver,
	&arkanoid_driver,
	&arknoidu_driver,
	&gauntlet_driver,
	&gaunt2_driver,
	&foodf_driver,
	&circus_driver,
	&robotbwl_driver,
	&sprint2_driver,
	&jack_driver,
	&vastar_driver,
	0	/* end of array */
};
