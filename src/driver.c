#include "driver.h"



extern struct GameDriver pacman_driver;
extern struct GameDriver pacmod_driver;
extern struct GameDriver namcopac_driver;
extern struct GameDriver hangly_driver;
extern struct GameDriver puckman_driver;
extern struct GameDriver piranha_driver;
extern struct GameDriver mspacman_driver;
extern struct GameDriver crush_driver;
extern struct GameDriver pengo_driver;
extern struct GameDriver penta_driver;
extern struct GameDriver ladybug_driver;
extern struct GameDriver mrdo_driver;
extern struct GameDriver mrdot_driver;
extern struct GameDriver mrlo_driver;
extern struct GameDriver docastle_driver;
extern struct GameDriver cclimber_driver;
extern struct GameDriver ccjap_driver;
extern struct GameDriver ccboot_driver;
extern struct GameDriver seicross_driver;
extern struct GameDriver ckong_driver;
extern struct GameDriver ckongs_driver;
extern struct GameDriver dkong_driver;
extern struct GameDriver dkongjp_driver;
extern struct GameDriver dkongjr_driver;
extern struct GameDriver dkong3_driver;
extern struct GameDriver mario_driver;
extern struct GameDriver bagman_driver;
extern struct GameDriver wow_driver;
extern struct GameDriver robby_driver;
extern struct GameDriver gorf_driver;
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
extern struct GameDriver mooncrst_driver;
extern struct GameDriver mooncrsb_driver;
extern struct GameDriver fantazia_driver;
extern struct GameDriver moonqsr_driver;
extern struct GameDriver scramble_driver;
extern struct GameDriver atlantis_driver;
extern struct GameDriver theend_driver;
extern struct GameDriver froggers_driver;
extern struct GameDriver scobra_driver;
extern struct GameDriver scobrak_driver;
extern struct GameDriver scobrab_driver;
extern struct GameDriver losttomb_driver;
extern struct GameDriver frogger_driver;
extern struct GameDriver frogsega_driver;
extern struct GameDriver amidar_driver;
extern struct GameDriver amidarjp_driver;
extern struct GameDriver turtles_driver;
extern struct GameDriver rallyx_driver;
extern struct GameDriver timeplt_driver;
extern struct GameDriver spaceplt_driver;
extern struct GameDriver pooyan_driver;
extern struct GameDriver phoenix_driver;
extern struct GameDriver pleiads_driver;
extern struct GameDriver carnival_driver;
extern struct GameDriver invaders_driver;
extern struct GameDriver earthinv_driver;
extern struct GameDriver spaceatt_driver;
extern struct GameDriver invrvnge_driver;
extern struct GameDriver invdelux_driver;
extern struct GameDriver galxwars_driver;
extern struct GameDriver lrescue_driver;
extern struct GameDriver desterth_driver;
extern struct GameDriver zaxxon_driver;
extern struct GameDriver congo_driver;
extern struct GameDriver bombjack_driver;
extern struct GameDriver centiped_driver;
extern struct GameDriver milliped_driver;
extern struct GameDriver nibbler_driver;
extern struct GameDriver fantasy_driver;
extern struct GameDriver mpatrol_driver;
extern struct GameDriver mranger_driver;
extern struct GameDriver btime_driver;
extern struct GameDriver btimea_driver;
extern struct GameDriver jumpbug_driver;
extern struct GameDriver jbugsega_driver;
extern struct GameDriver vanguard_driver;
extern struct GameDriver gberet_driver;
extern struct GameDriver rushatck_driver;
extern struct GameDriver venture_driver;
extern struct GameDriver mtrap_driver;
extern struct GameDriver pepper2_driver;
extern struct GameDriver qbert_driver;
extern struct GameDriver qbertjp_driver;
extern struct GameDriver mplanets_driver;
extern struct GameDriver junglek_driver;
extern struct GameDriver jungleh_driver;
extern struct GameDriver elevator_driver;
extern struct GameDriver elevatob_driver;
extern struct GameDriver panic_driver;
extern struct GameDriver arabian_driver;
extern struct GameDriver reactor_driver;
extern struct GameDriver c1942_driver;
extern struct GameDriver popeyebl_driver;
extern struct GameDriver warpwarp_driver;
extern struct GameDriver gyruss_driver;
extern struct GameDriver krull_driver;
extern struct GameDriver superpac_driver;
extern struct GameDriver kangaroo_driver;
extern struct GameDriver galaga_driver;
extern struct GameDriver galagabl_driver;


const struct GameDriver *drivers[] =
{
	&pacman_driver,
	&pacmod_driver,
	&namcopac_driver,
	&hangly_driver,
	&puckman_driver,
	&piranha_driver,
	&mspacman_driver,
	&crush_driver,
	&pengo_driver,
	&penta_driver,
	&ladybug_driver,
	&mrdo_driver,
	&mrdot_driver,
	&mrlo_driver,
	&docastle_driver,
	&cclimber_driver,
	&ccjap_driver,
	&ccboot_driver,
	&seicross_driver,
	&ckong_driver,
	&ckongs_driver,
	&dkong_driver,
        &dkongjp_driver,
	&dkongjr_driver,
	&dkong3_driver,
	&mario_driver,
	&bagman_driver,
	&wow_driver,
	&robby_driver,
	&gorf_driver,
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
	&mooncrst_driver,
	&mooncrsb_driver,
	&fantazia_driver,
	&moonqsr_driver,
	&scramble_driver,
	&atlantis_driver,
	&theend_driver,
	&froggers_driver,
	&scobra_driver,
	&scobrak_driver,
	&scobrab_driver,
	&losttomb_driver,
	&frogger_driver,
	&frogsega_driver,
	&amidar_driver,
	&amidarjp_driver,
	&turtles_driver,
	&rallyx_driver,
	&timeplt_driver,
	&spaceplt_driver,
	&pooyan_driver,
	&phoenix_driver,
	&pleiads_driver,
	&carnival_driver,
	&invaders_driver,
	&earthinv_driver,
	&spaceatt_driver,
	&invrvnge_driver,
	&invdelux_driver,
	&galxwars_driver,
	&lrescue_driver,
	&desterth_driver,
	&zaxxon_driver,
	&congo_driver,
	&bombjack_driver,
	&centiped_driver,
	&milliped_driver,
	&nibbler_driver,
	&fantasy_driver,
	&mpatrol_driver,
	&mranger_driver,
	&btime_driver,
	&btimea_driver,
	&jumpbug_driver,
	&jbugsega_driver,
	&vanguard_driver,
	&gberet_driver,
	&rushatck_driver,
	&venture_driver,
	&mtrap_driver,
	&pepper2_driver,
	&qbert_driver,
	&qbertjp_driver,
	&mplanets_driver,
	&junglek_driver,
	&jungleh_driver,
	&elevator_driver,
        &elevatob_driver,
	&panic_driver,
        &arabian_driver,
        &reactor_driver,
        &popeyebl_driver,
        &warpwarp_driver,
        &c1942_driver,
        &gyruss_driver,
        &krull_driver,
        &superpac_driver,
        &kangaroo_driver,
        &galaga_driver,
        &galagabl_driver,
	0	/* end of array */
};
