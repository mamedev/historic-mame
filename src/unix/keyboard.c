/*
 * X-Mame video specifics code
 *
 */
#define __XDEP_C_

/*
 * Include files.
 */

#include "xmame.h"

/*
 * Wait until player wants to resume game.
 */

/* Key handling was way to fast using this routine */

int osd_read_key (void)
{
	int	i;
	int slowdown = 30;
	i = OSD_NUMKEYS;

	while (i == OSD_NUMKEYS)
	{
		osd_key_pressed (-1);

		i = 0;
		while (!key[i++] && (i < OSD_NUMKEYS));
	}
        if (slowdown == 0) {
          slowdown = 30;
	  return (i-1);
        }
	else {
	  slowdown--;
	  return (0);
	}

}

/*
 * Check if a key is pressed. The keycode is the standard PC keyboard code,
 * as defined in osdepend.h.
 *
 * Handles all incoming keypress/keyrelease events and updates the 'key'
 * array accordingly.
 *
 * This poses some problems, since keycodes returned from X aren't quite the
 * same as normal PC keyboard codes. For now, we only catch the keys we're
 * interested in in a switch-statement. Could be improved so that programs
 * don't expect osdepend.c to map keys to a certain value.
 *
 * Return 0 if the key is not pressed, nonzero otherwise.
 *
 */

int osd_key_pressed (int request)
{
  XEvent 		E;
  int	 		keycode;
  int			mask;

  while (XCheckWindowEvent (display,window, KeyPressMask | KeyReleaseMask, &E))
  {
  	mask = TRUE; 	/* In case of KeyPress. */

    if ((E.type == KeyPress) || (E.type == KeyRelease))
    {
			if (E.type == KeyRelease)
			{
				mask = FALSE;
			}

	    keycode = XLookupKeysym ((XKeyEvent *) &E, 0);

      switch (keycode)
      {
        case XK_Escape:
        {
        	key[OSD_KEY_ESC] = mask;
        	break;
        }
        case XK_Down:
        {
        	key[OSD_KEY_DOWN] = mask;
       		break;
       	}
       	case XK_Up:
       	{
       		key[OSD_KEY_UP] = mask;
       		break;
       	}
       	case XK_Left:
       	{
       		key[OSD_KEY_LEFT] = mask;
       		break;
       	}
       	case XK_Right:
       	{
       		key[OSD_KEY_RIGHT] = mask;
       		break;
       	}
       	case XK_Control_L:
       	case XK_Control_R:
       	{
       		key[OSD_KEY_CONTROL] = mask;
       		break;
       	}
       	case XK_Tab:
       	{
       		key[OSD_KEY_TAB] = mask;
       		break;
       	}
      	case XK_F1:
      	{
      		key[OSD_KEY_F1] = mask;
      		break;
      	}
      	case XK_F2:
      	{
      		key[OSD_KEY_F2] = mask;
      		break;
      	}
      	case XK_F3:
      	{
       		key[OSD_KEY_F3] = mask;
      		break;
      	}
      	case XK_F4:
      	{
      		key[OSD_KEY_F4] = mask;
      		break;
      	}
      	case XK_F5:
      	{
      		key[OSD_KEY_F5] = mask;
      		break;
      	}
      	case XK_F6:
      	{
      		key[OSD_KEY_F6] = mask;
      		break;
      	}
      	case XK_F7:
      	{
      		key[OSD_KEY_F7] = mask;
      		break;
      	}
      	case XK_F8:
      	{
      		key[OSD_KEY_F8] = mask;
      		break;
      	}
      	case XK_F9:
      	{
      		key[OSD_KEY_F9] = mask;
      		break;
      	}
        case XK_F10:
        {
        	key[OSD_KEY_F10] = mask;
        	break;
        }
        case XK_F11:
        {
        	key[OSD_KEY_F11] = mask;
        	break;
        }
        case XK_F12:
        {
        	key[OSD_KEY_F12] = mask;
        	break;
        }
      	case XK_0:
      	{
      		key[OSD_KEY_0] = mask;
      		break;
      	}
      	case XK_1:
      	{
      		key[OSD_KEY_1] = mask;
      		break;
      	}
      	case XK_2:
      	{
      		key[OSD_KEY_2] = mask;
      		break;
      	}
      	case XK_3:
      	{
      		key[OSD_KEY_3] = mask;
      		break;
      	}
      	case XK_4:
      	{
      		key[OSD_KEY_4] = mask;
      		break;
      	}
      	case XK_5:
      	{
      		key[OSD_KEY_5] = mask;
      		break;
      	}
      	case XK_6:
      	{
      		key[OSD_KEY_6] = mask;
      		break;
      	}
      	case XK_7:
      	{
      		key[OSD_KEY_7] = mask;
      		break;
      	}
      	case XK_8:
      	{
      		key[OSD_KEY_8] = mask;
      		break;
      	}
      	case XK_9:
      	{
      		key[OSD_KEY_9] = mask;
      		break;
      	}

/* Mike Zimmerman noticed this one */
/* Pause now works */
	case XK_p:
	case XK_P:
	{
		key[OSD_KEY_P]=mask;
		break;
	}
/* For Crazy Climber - Jack Patton (jpatton@intserv.com)*/
/* Based on this stuff from ../drivers/cclimber.c */
/* Left stick: OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, */
/* Right stick: OSD_KEY_I, OSD_KEY_K, OSD_KEY_J, OSD_KEY_L */
/* and values from the keycode variable */

      	case 101:
      	{
      		key[OSD_KEY_E] = mask;
      		break;
      	}
      	case 100:
      	{
      		key[OSD_KEY_D] = mask;
      		break;
      	}
      	case 115:
      	{
      		key[OSD_KEY_S] = mask;
      		break;
      	}
      	case 102:
      	{
      		key[OSD_KEY_F] = mask;
      		break;
      	}
      	case 105:
      	{
      		key[OSD_KEY_I] = mask;
      		break;
      	}
      	case 106:
      	{
      		key[OSD_KEY_J] = mask;
      		break;
      	}
      	case 107:
      	{
      		key[OSD_KEY_K] = mask;
      		break;
      	}
      	case 108:
      	{
      		key[OSD_KEY_L] = mask;
      		break;
      	}
/* Scramble and Space Cobra bomb key OSD_KEY_ALT*/
/* from one of the keysym includes */
/* Jack Patton (jpatton@intserv.com) */
      	case XK_Alt_L:
      	{
      		key[OSD_KEY_ALT] = mask;
      		break;
      	}
      	case XK_Alt_R:
      	{
      		key[OSD_KEY_ALT] = mask;
      		break;
      	}
				default:
				{
				}
      }
		}
	}

	if (request >= 0)
		return key[request];
	else
		return (FALSE);
}

