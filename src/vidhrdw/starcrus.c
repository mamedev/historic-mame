
/* Ramtek - Star Cruiser */

#include "driver.h"

/* included from sndhrdw/starcrus.c */
extern int starcrus_engine_sound_playing;
extern int starcrus_explode_sound_playing;
extern int starcrus_launch1_sound_playing;
extern int starcrus_launch2_sound_playing;

static int s1_x = 0;
static int s1_y = 0;
static int s2_x = 0;
static int s2_y = 0;
static int p1_x = 0;
static int p1_y = 0;
static int p2_x = 0;
static int p2_y = 0;

static int p1_sprite = 0;
static int p2_sprite = 0;
static int s1_sprite = 0;
static int s2_sprite = 0;

static int engine1_on = 0;
static int engine2_on = 0;
static int explode1_on = 0;
static int explode2_on = 0;

void starcrus_s1_x_w(int offset, int data) { s1_x = data^0xff; }
void starcrus_s1_y_w(int offset, int data) { s1_y = data^0xff; }
void starcrus_s2_x_w(int offset, int data) { s2_x = data^0xff; }
void starcrus_s2_y_w(int offset, int data) { s2_y = data^0xff; }
void starcrus_p1_x_w(int offset, int data) { p1_x = data^0xff; }
void starcrus_p1_y_w(int offset, int data) { p1_y = data^0xff; }
void starcrus_p2_x_w(int offset, int data) { p2_x = data^0xff; }
void starcrus_p2_y_w(int offset, int data) { p2_y = data^0xff; }

void starcrus_ship_parm_1_w(int offset, int data)
{
    s1_sprite = data&0x1f;
    engine1_on = ((data&0x20)>>5)^0x01;

    if (engine1_on || engine2_on)
        starcrus_engine_sound_playing = 1;
    else
        starcrus_engine_sound_playing = 0;
}

void starcrus_ship_parm_2_w(int offset, int data)
{
    s2_sprite = data&0x1f;
    osd_led_w(2, ((data&0x80)>>7)^0x01); /* game over lamp */
    coin_counter_w(0, ((data&0x40)>>6)^0x01); /* coin counter */
    engine2_on = ((data&0x20)>>5)^0x01;

    if (engine1_on || engine2_on)
        starcrus_engine_sound_playing = 1;
    else
        starcrus_engine_sound_playing = 0;
}

void starcrus_proj_parm_1_w(int offset, int data)
{
    p1_sprite = data&0x0f;
    starcrus_launch1_sound_playing = ((data&0x20)>>5)^0x01;
    explode1_on = ((data&0x10)>>4)^0x01;

    if (explode1_on || explode2_on)
        starcrus_explode_sound_playing = 1;
    else
        starcrus_explode_sound_playing = 0;
}

void starcrus_proj_parm_2_w(int offset, int data)
{
    p2_sprite = data&0x0f;
    starcrus_launch2_sound_playing = ((data&0x20)>>5)^0x01;
    explode2_on = ((data&0x10)>>4)^0x01;

    if (explode1_on || explode2_on)
        starcrus_explode_sound_playing = 1;
    else
        starcrus_explode_sound_playing = 0;
}

void starcrus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

    drawgfx(bitmap,
            Machine->gfx[8+((s1_sprite&0x04)>>2)],
            (s1_sprite&0x03)^0x03,
            0,
            (s1_sprite&0x08)>>3,(s1_sprite&0x10)>>4,
            s1_x,s1_y,
            &Machine->drv->visible_area,
            TRANSPARENCY_PEN,
            0);

    drawgfx(bitmap,
            Machine->gfx[10+((s2_sprite&0x04)>>2)],
            (s2_sprite&0x03)^0x03,
            0,
            (s2_sprite&0x08)>>3,(s2_sprite&0x10)>>4,
            s2_x,s2_y,
            &Machine->drv->visible_area,
            TRANSPARENCY_PEN,
            0);

    drawgfx(bitmap,
            Machine->gfx[(p1_sprite&0x0c)>>2],
            (p1_sprite&0x03)^0x03,
            0,
            0,0,
            p1_x,p1_y,
            &Machine->drv->visible_area,
            TRANSPARENCY_PEN,
            0);

    drawgfx(bitmap,
            Machine->gfx[4+((p2_sprite&0x0c)>>2)],
            (p2_sprite&0x03)^0x03,
            0,
            0,0,
            p2_x,p2_y,
            &Machine->drv->visible_area,
            TRANSPARENCY_PEN,
            0);
#if 0
    /* Collision detection logic only - TBD */

    collison_reg = 0xff;
    if (s1 hit s2)
    {
        collision_reg &= 0x07;
    }
    if (p1_sprite & 0x08)  /* if p1 is a projectile */
    {
        if (p1 hit s1)
        {
            collision_reg &= 0x0d;
        }
        if (p1 hit s2)
        {
            collision_reg &= 0x0e;
        }
    }
    if (p2_sprite & 0x08)  /* if p2 is a projectile */
    {
        if (p2 hit s1)
        {
            collision_reg &= 0x0d;
        }
        if (p2 hit s2)
        {
            collision_reg &= 0x0e;
        }
    }
    /* I don't think this logic is necessary, but it is in the game */
    if ((p1_sprite & 0x08) && (p2_sprite & 0x08))
    {
        if (p1 hit p2)
        {
            collision_reg &= 0x0b;
        }
    }
#endif

}

int starcrus_coll_det_r(int offset)
{
#if 0
    return collision_reg;
#endif
    return 0xff;
}
