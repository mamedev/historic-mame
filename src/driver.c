#include "driver.h"


/* "Pacman hardware" games */
extern struct GameDriver pacman_driver;
extern struct GameDriver pacplus_driver;
extern struct GameDriver jrpacman_driver;
extern struct GameDriver pacmod_driver;
extern struct GameDriver namcopac_driver;
extern struct GameDriver pacmanjp_driver;
extern struct GameDriver hangly_driver;
extern struct GameDriver puckman_driver;
extern struct GameDriver piranha_driver;
extern struct GameDriver mspacman_driver;
extern struct GameDriver mspacatk_driver;
extern struct GameDriver crush_driver;
extern struct GameDriver pengo_driver;
extern struct GameDriver pengoa_driver;
extern struct GameDriver penta_driver;
/* "Galaxian hardware" games */
extern struct GameDriver galaxian_driver;
extern struct GameDriver galmidw_driver;
extern struct GameDriver galnamco_driver;
extern struct GameDriver superg_driver;
extern struct GameDriver galapx_driver;
extern struct GameDriver galap1_driver;
extern struct GameDriver galap4_driver;
extern struct GameDriver galturbo_driver;
extern struct GameDriver pisces_driver;
extern struct GameDriver japirem_driver;
extern struct GameDriver uniwars_driver;
extern struct GameDriver warofbug_driver;
extern struct GameDriver redufo_driver;
extern struct GameDriver pacmanbl_driver;
extern struct GameDriver mooncrst_driver;
extern struct GameDriver mooncrsg_driver;
extern struct GameDriver mooncrsb_driver;
extern struct GameDriver fantazia_driver;
extern struct GameDriver moonqsr_driver;
/* "Scramble hardware" (and variations) games */
extern struct GameDriver scramble_driver;
extern struct GameDriver atlantis_driver;
extern struct GameDriver theend_driver;
extern struct GameDriver ckongs_driver;
extern struct GameDriver froggers_driver;
extern struct GameDriver triplep_driver;
extern struct GameDriver scobra_driver;
extern struct GameDriver scobrak_driver;
extern struct GameDriver scobrab_driver;
extern struct GameDriver losttomb_driver;
extern struct GameDriver anteater_driver;
extern struct GameDriver rescue_driver;
extern struct GameDriver frogger_driver;
extern struct GameDriver frogsega_driver;
extern struct GameDriver amidar_driver;
extern struct GameDriver amidarjp_driver;
extern struct GameDriver amigo_driver;
extern struct GameDriver turtles_driver;
extern struct GameDriver turpin_driver;
extern struct GameDriver jumpbug_driver;
extern struct GameDriver jbugsega_driver;
/* "Crazy Climber hardware" games */
extern struct GameDriver cclimber_driver;
extern struct GameDriver ccjap_driver;
extern struct GameDriver ccboot_driver;
extern struct GameDriver ckong_driver;
extern struct GameDriver ckonga_driver;
extern struct GameDriver ckongjeu_driver;
/* "Phoenix hardware" (and variations) games */
extern struct GameDriver phoenix_driver;
extern struct GameDriver phoenixt_driver;
extern struct GameDriver phoenix3_driver;
extern struct GameDriver pleiads_driver;
extern struct GameDriver naughtyb_driver;
extern struct GameDriver popflame_driver;
/* "Space Invaders hardware " games */
extern struct GameDriver invaders_driver;
extern struct GameDriver earthinv_driver;
extern struct GameDriver spaceatt_driver;
extern struct GameDriver invrvnge_driver;
extern struct GameDriver invdelux_driver;
extern struct GameDriver galxwars_driver;
extern struct GameDriver lrescue_driver;
extern struct GameDriver desterth_driver;
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
extern struct GameDriver docastle_driver;	/* (c) 1983 */
extern struct GameDriver docastl2_driver;	/* (c) 1983 */
extern struct GameDriver dounicorn_driver;	/* (c) 1983 */
extern struct GameDriver dowild_driver;		/* (c) 1984 */
extern struct GameDriver dorunrun_driver;	/* (c) 1984 */
extern struct GameDriver kickridr_driver;	/* (c) 1984 */
/* Nintendo games */
extern struct GameDriver dkong_driver;		/* (c) 1981 Nintendo of America */
extern struct GameDriver radarscp_driver;
extern struct GameDriver dkongjp_driver;	/* (c) 1981 Nintendo */
extern struct GameDriver dkongjr_driver;	/* (c) 1982 Nintendo of America */
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
extern struct GameDriver dotron_driver;		/* (c) 1983 */
extern struct GameDriver destderb_driver;	/* (c) 1984 */
extern struct GameDriver timber_driver;		/* (c) 1984 */
extern struct GameDriver spyhunt_driver;
extern struct GameDriver rampage_driver;	/* (c) 1986 */
/* Irem games */
extern struct GameDriver mpatrol_driver;	/* (c) 1982 + Williams license */
extern struct GameDriver mranger_driver;	/* bootleg */
extern struct GameDriver yard_driver;		/* (c) 1983 */
extern struct GameDriver kungfum_driver;	/* (c) 1984 */
extern struct GameDriver kungfub_driver;	/* bootleg */
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
extern struct GameDriver zookeep_driver;
/* Taito "Jungle King hardware" games */
extern struct GameDriver junglek_driver;	/* (c) 1982 */
extern struct GameDriver jhunt_driver;		/* (c) 1982 Taito America */
extern struct GameDriver elevator_driver;	/* (c) 1983 */
extern struct GameDriver elevatob_driver;	/* bootleg */
extern struct GameDriver frontlin_driver;	/* (c) 1982 */
extern struct GameDriver wwestern_driver;	/* (c) 1982 */
/* Williams games */
extern struct GameDriver robotron_driver;	/* (c) 1982 */
extern struct GameDriver stargate_driver;	/* (c) 1981 */
extern struct GameDriver joust_driver;		/* (c) 1982 */
extern struct GameDriver sinistar_driver;	/* (c) 1982 */
extern struct GameDriver bubbles_driver;	/* (c) 1982 */
extern struct GameDriver defender_driver;	/* (c) 1980 */
extern struct GameDriver splat_driver;		/* (c) 1982 */
extern struct GameDriver blaster_driver;	/* (c) 1983 */
/* Capcom games */
extern struct GameDriver c1942_driver;		/* (c) 1984 */
extern struct GameDriver vulgus_driver;		/* (c) 1984 */
extern struct GameDriver commando_driver;	/* (c) 1985 */
extern struct GameDriver commandj_driver;	/* (c) 1985 */
extern struct GameDriver gng_driver;		/* (c) 1985 */
extern struct GameDriver gngcross_driver;	/* (c) 1985 */
extern struct GameDriver diamond_driver;	/* (c) 1989 KH Video (runs on GnG hardware) */
extern struct GameDriver sonson_driver;		/* (c) 1984 */
extern struct GameDriver exedexes_driver;	/* (c) 1985 */
extern struct GameDriver lwings_driver;		/* (c) 1986 */
extern struct GameDriver sectionz_driver;	/* (c) 1985 */
extern struct GameDriver c1943_driver;		/* (c) 1987 */
extern struct GameDriver gunsmoke_driver;	/* (c) 1985 + Romstar */
extern struct GameDriver blktiger_driver;	/* (c) 1987 */
extern struct GameDriver capbowl_driver;	/* (c) 1989 Incredible Technologies - different hardware */
/* Sega "dual game board" games */
extern struct GameDriver carnival_driver;	/* (c) 1980 */
extern struct GameDriver pulsar_driver;		/* (c) 1981 */
extern struct GameDriver invho2_driver;		/* (c) 1979 */
extern struct GameDriver sspaceat_driver;	/* (c) */
extern struct GameDriver invinco_driver;	/* (c) 1979 */
/* Sega vector games */
extern struct GameDriver spacfury_driver;
extern struct GameDriver zektor_driver;		/* (c) 1982 */
extern struct GameDriver tacscan_driver;	/* (c) */
extern struct GameDriver elim2_driver;		/* (c) 1981 Gremlin */
extern struct GameDriver startrek_driver;	/* (c) 1982 */
/* Sega "Zaxxon hardware" games */
extern struct GameDriver zaxxon_driver;		/* (c) 1982 */
extern struct GameDriver congo_driver;		/* (c) 1983 */
	/* other games running on this hardware: Super Zaxxon, Future Spy */
/* Data East games */
extern struct GameDriver btime_driver;		/* (c) 1982 + Midway */
extern struct GameDriver btimea_driver;		/* (c) 1982 */
	/* other games running on this hardware: Bump'n Jump, Zoar */
extern struct GameDriver eggs_driver;		/* (c) 1983 Universal USA */
/* Tehkan games */
extern struct GameDriver bombjack_driver;	/* (c) 1984 */
extern struct GameDriver starforc_driver;	/* (c) 1984 */
/* Konami games */
extern struct GameDriver pooyan_driver;		/* (c) 1982 Stern */
extern struct GameDriver timeplt_driver;	/* (c) 1982 */
extern struct GameDriver spaceplt_driver;	/* bootleg */
extern struct GameDriver gyruss_driver;		/* (c) 1983 */
extern struct GameDriver gyrussce_driver;	/* (c) 1983 + Centuri license */
extern struct GameDriver sbasketb_driver;	/* (c) 1984 */
extern struct GameDriver tp84_driver;		/* (c) 1984 */
extern struct GameDriver gberet_driver;		/* (c) 1985 */
extern struct GameDriver rushatck_driver;	/* (c) 1985 */
extern struct GameDriver yiear_driver;		/* (c) 1985 */
/* Exidy games */
extern struct GameDriver venture_driver;	/* (c) 1981 */
extern struct GameDriver mtrap_driver;		/* (c) 1981 */
extern struct GameDriver pepper2_driver;	/* (c) 1982 */
/* Atari vector games */
extern struct GameDriver asteroid_driver;
extern struct GameDriver asteroi2_driver;	/* (c) 1979 */
extern struct GameDriver astdelux_driver;	/* (c) 1980 */
extern struct GameDriver bwidow_driver;		/* (c) 1982 */
extern struct GameDriver bzone_driver;		/* (c) 1980 */
extern struct GameDriver bzone2_driver;		/* (c) 1980 */
extern struct GameDriver gravitar_driver;	/* (c) 1982 */
extern struct GameDriver llander_driver;
extern struct GameDriver redbaron_driver;	/* (c) 1980 */
extern struct GameDriver spacduel_driver;	/* (c) 1980 */
extern struct GameDriver tempest_driver;	/* (c) 1980 */
extern struct GameDriver starwars_driver;	/* (c) 1983 */
/* Rock-ola games */
extern struct GameDriver vanguard_driver;	/* (c) 1981 SNK */
extern struct GameDriver nibbler_driver;	/* (c) 1982 */
extern struct GameDriver fantasy_driver;	/* (c) 1981 */


extern struct GameDriver seicross_driver;
extern struct GameDriver bagman_driver;
extern struct GameDriver sbagman_driver;
extern struct GameDriver rallyx_driver;
extern struct GameDriver nrallyx_driver;
extern struct GameDriver locomotn_driver;
extern struct GameDriver centiped_driver;
extern struct GameDriver milliped_driver;
extern struct GameDriver warlord_driver;
extern struct GameDriver panic_driver;
extern struct GameDriver panica_driver;
extern struct GameDriver arabian_driver;
extern struct GameDriver kangaroo_driver;
extern struct GameDriver warpwarp_driver;
extern struct GameDriver mystston_driver;
extern struct GameDriver spacefb_driver;
extern struct GameDriver tutankhm_driver;
extern struct GameDriver ccastles_driver;
extern struct GameDriver missile_driver;
extern struct GameDriver bublbobl_driver;
extern struct GameDriver boblbobl_driver;
extern struct GameDriver sboblbob_driver;
extern struct GameDriver blueprnt_driver;
extern struct GameDriver omegrace_driver;
extern struct GameDriver bankp_driver;
extern struct GameDriver espial_driver;
extern struct GameDriver rastan_driver;
extern struct GameDriver rastsaga_driver;
extern struct GameDriver cloak_driver;
extern struct GameDriver berzerk_driver;
extern struct GameDriver berzerk1_driver;
extern struct GameDriver silkworm_driver;
extern struct GameDriver sidearms_driver;
extern struct GameDriver champbas_driver;
extern struct GameDriver punchout_driver;



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
	&triplep_driver,
	&scobra_driver,
	&scobrak_driver,
	&scobrab_driver,
	&losttomb_driver,
	&anteater_driver,
	&rescue_driver,
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
	&docastle_driver,
	&docastl2_driver,
	&dounicorn_driver,
	&dowild_driver,
	&dorunrun_driver,
	&kickridr_driver,
	/* Nintendo games */
	&dkong_driver,
	&radarscp_driver,
	&dkongjp_driver,
	&dkongjr_driver,
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
	&dotron_driver,
	&destderb_driver,
	&timber_driver,
	&spyhunt_driver,
	&rampage_driver,
	/* Irem games */
	&mpatrol_driver,
	&mranger_driver,
	&yard_driver,
	&kungfum_driver,
	&kungfub_driver,
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
	&zookeep_driver,
	/* Taito "Jungle King hardware" games */
	&junglek_driver,
	&jhunt_driver,
	&elevator_driver,
	&elevatob_driver,
	&frontlin_driver,
	&wwestern_driver,
	/* Williams games */
	&robotron_driver,
	&stargate_driver,
	&joust_driver,
	&sinistar_driver,
	&bubbles_driver,
	&defender_driver,
	&splat_driver,
	&blaster_driver,
	/* Capcom games */
	&c1942_driver,
	&vulgus_driver,
	&commando_driver,
	&commandj_driver,
	&gng_driver,
	&gngcross_driver,
	&diamond_driver,
	&sonson_driver,
	&exedexes_driver,
	&lwings_driver,
	&sectionz_driver,
	&c1943_driver,
	&gunsmoke_driver,
	&blktiger_driver,
	&capbowl_driver,
	/* Sega "dual game board" games */
	&carnival_driver,
	&pulsar_driver,
	&invho2_driver,
	&sspaceat_driver,
	&invinco_driver,
	/* Sega vector games */
	&spacfury_driver,
	&zektor_driver,
	&tacscan_driver,
	&elim2_driver,
	&startrek_driver,
	/* Sega "Zaxxon hardware" games */
	&zaxxon_driver,
	&congo_driver,
	/* Data East games */
	&btime_driver,
	&btimea_driver,
	&eggs_driver,
	/* Tehkan games */
	&bombjack_driver,
	&starforc_driver,
	/* Konami games */
	&pooyan_driver,
	&timeplt_driver,
	&spaceplt_driver,
	&gyruss_driver,
	&gyrussce_driver,
	&sbasketb_driver,
	&tp84_driver,
	&gberet_driver,
	&rushatck_driver,
	&yiear_driver,
	/* Exidy games */
	&venture_driver,
	&mtrap_driver,
	&pepper2_driver,
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
	&starwars_driver,
	/* Rock-ola games */
	&vanguard_driver,
	&nibbler_driver,
	&fantasy_driver,


	&seicross_driver,
	&bagman_driver,
	&sbagman_driver,
	&rallyx_driver,
	&nrallyx_driver,
	&locomotn_driver,
	&centiped_driver,
	&milliped_driver,
	&warlord_driver,
	&panic_driver,
	&panica_driver,
	&arabian_driver,
	&kangaroo_driver,
	&warpwarp_driver,
	&mystston_driver,
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
	&berzerk_driver,
	&berzerk1_driver,
	&silkworm_driver,
	&sidearms_driver,
	&champbas_driver,
	&punchout_driver,
	0	/* end of array */
};
