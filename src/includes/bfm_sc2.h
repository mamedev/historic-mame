#ifndef INC_BFMSC2
#define INC_BFMSC2

extern void on_scorpion2_reset(void);
extern void send_to_sc2(int data);
extern int  read_from_sc2(void);

extern int  receive_from_adder(void);
extern void send_to_adder(int data);


extern int sc2_show_door;		  // flag <>0, display door state
extern int sc2_door_state;		  // door state to display

#endif
