/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"

#define ROTATE_TILE_WIDTH	256
#define ROTATE_TILE_HEIGHT	256
#define ROTATE_PIXEL_WIDTH	(ROTATE_TILE_WIDTH*8)
#define ROTATE_PIXEL_HEIGHT	(ROTATE_TILE_HEIGHT*8)
#define ROTATE_MASK_WIDTH   (ROTATE_PIXEL_WIDTH-1)
#define ROTATE_MASK_HEIGHT  (ROTATE_PIXEL_HEIGHT-1)

struct osd_bitmap *rotate_bitmap;

int namcos2_vh_start(void)
{
    int loop;

	if ((rotate_bitmap = osd_new_bitmap(ROTATE_PIXEL_WIDTH,ROTATE_PIXEL_HEIGHT,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	for(loop=0;loop<8192;loop++) palette_used_colors[loop] |= PALETTE_COLOR_VISIBLE;

    return 0;
}


void namcos2_vh_stop(void)
{
	osd_free_bitmap(rotate_bitmap);
	tmpbitmap = 0;
}


void namcos2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
    int i;
   	/* Zap the palette to Zero */

    for( i = 0; i < Machine->drv->total_colors; i++ )
    {
        palette[i*3+0] = 0;
        palette[i*3+1] = 0;
        palette[i*3+2] = 0;
    }
}



#define TITLE_X		    (6*8)
#define RGDISP_X		(5*6)
#define RGDISP_Y		(3*8)
#define RGDISP_Y_SEP	8
#ifdef MAME_DEBUG
static void show_reg(int regbank )
{
	int i;
	char buffer[256];
	int ypos=RGDISP_Y;
	int xpos=RGDISP_X;

	switch (regbank)
	{
		case 1:
            sprintf(buffer,"VRAM Control: %08lx",0x00420000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x10; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[i]));
                ui_text(buffer,1*RGDISP_X,ypos);

                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x10+i]));
                ui_text(buffer,2*RGDISP_X,ypos);

                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x20+i]));
                ui_text(buffer,3*RGDISP_X,ypos);

                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_vram_ctrl[0x30+i]));
                ui_text(buffer,4*RGDISP_X,ypos);

                ypos+=RGDISP_Y_SEP;
			}

            ypos+=RGDISP_Y_SEP;
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"Palette Control: %08lx",0x00443000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x08; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_palette_ram[0x3000 + i]));
                ui_text(buffer,1*RGDISP_X,ypos);
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_palette_ram[0x3008 + i]));
                ui_text(buffer,2*RGDISP_X,ypos);
                ypos+=RGDISP_Y_SEP;
			}

            break;
		case 2:
            ypos+=RGDISP_Y_SEP;
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"ROZ Control: %08lx",0x00cc0000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x08; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_roz_ctrl[i]));
                ui_text(buffer,1*RGDISP_X,ypos);
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_roz_ctrl[0x08+i]));
                ui_text(buffer,2*RGDISP_X,ypos);
                ypos+=RGDISP_Y_SEP;
			}
            ypos+=RGDISP_Y_SEP;
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"Sprite Bank : %08lx",0x00c40000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"%04x",namcos2_68k_sprite_bank_r(0));
            ui_text(buffer,RGDISP_X,ypos);
			break;
		case 3:
            sprintf(buffer,"Protection Key: %08lx",0x00d00000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x10; i+=2 )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_key[i]));
                ui_text(buffer,RGDISP_X,ypos);
                ypos+=RGDISP_Y_SEP;
			}
			break;
		case 4:
            sprintf(buffer,"IRQ Control: %08lx",0x001c0000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<0x20; i++ )
			{
                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_master_C148[i]));
                ui_text(buffer,RGDISP_X,ypos);

                sprintf(buffer,"%04x",READ_WORD(&namcos2_68k_slave_C148[i]));
                ui_text(buffer,2*RGDISP_X,ypos);
                ypos+=RGDISP_Y_SEP;
			}
			break;
		case 5:
            sprintf(buffer,"Sprite RAM: %08lx",0x00c00000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"SZ S -Y-  F F B NUM Q  -X-  SZ C P");
            ui_text(buffer,12,ypos);
            ypos+=RGDISP_Y_SEP;
			for( i=0; i<(0x10*0x08); i+=0x08 )
			{
                xpos=12;

                sprintf(buffer,"%02x %01x %03x  %01x %01x %01x %03x %01x  %03x  %02x %01x %01x",
                         ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x0a)&0x003f,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x09)&0x0001,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x00]))>>0x00)&0x01ff,

                         ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0f)&0x0001,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0e)&0x0001,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x0d)&0x0001,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x02)&0x07ff,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x02]))>>0x00)&0x0003,

                         ((READ_WORD(&namcos2_sprite_ram[i+0x04]))>>0x00)&0x03ff,

                         ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x0a)&0x003f,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x04)&0x000f,
                         ((READ_WORD(&namcos2_sprite_ram[i+0x06]))>>0x00)&0x0007);
                ui_text(buffer,xpos,ypos);
                ypos+=RGDISP_Y_SEP;
			}
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"Sprite Bank : %08lx",0x00c40000L);
            ui_text(buffer,TITLE_X,ypos);
            ypos+=RGDISP_Y_SEP;
            sprintf(buffer,"%04x",namcos2_68k_sprite_bank_r(0));
            ui_text(buffer,RGDISP_X,ypos);
			break;
		default:
			break;
	}
#if 0
	{
		static FILE *f=NULL;
		int i;
		if (!f) f = fopen ("roz.log", "w");
		for (i = 0; i < 0x10; i+=2) fprintf (f, "%08x ", READ_WORD(&namcos2_roz_ctrl[i]));
		fprintf (f, "\n");
	}
#endif
}
#endif


void rotatebitmap( struct osd_bitmap *dest_bitmap, struct osd_bitmap *source_bitmap,
                   int start_x,  int start_y,
                   int right_dx, int right_dy,
                   int down_dx,  int down_dy )
{
	int dest_x, dest_y, tmp_x, tmp_y;

    /* Temporary bodge code */

    if(Machine->orientation & ORIENTATION_SWAP_XY)
    {
        int tmp;

        tmp=start_x;
        start_x=start_y;
        start_y=tmp;

        tmp=right_dy;
        right_dy=down_dx;
        down_dx=tmp;

        tmp=right_dx;
        right_dx=down_dy;
        down_dy=tmp;

        /* Correct factor is needed for the screen offset */
        start_x+=48*down_dx;
        start_y+=48*down_dy;
    }
    else
    {
        /* Correct factor is needed for the screen offset */
        start_x-=32*right_dx;
        start_y-=32*right_dy;

        start_x+=16*down_dx;
        start_y+=16*down_dy;
    }


    if(dest_bitmap->depth == 16)
    {
    	unsigned short *dest_line,*tmp,pixel;

    	for( dest_y=0; dest_y<dest_bitmap->height; dest_y++ )
	    {
    		dest_line = (unsigned short*)dest_bitmap->line[dest_y];
		    tmp_x = start_x;
	    	tmp_y = start_y;

		    for( dest_x=dest_bitmap->width-1; dest_x>=0; dest_x-- )
    		{
	    	    int xind=tmp_x>>16;
		        int yind=tmp_y>>16;

/*	BODGE CODE COMMENTED OUT
              if(Machine->orientation & ORIENTATION_SWAP_XY)
              {
                  int tmp;
                  tmp=xind;
                  xind=yind;
                  yind=tmp;
              }
*/

                if(Machine->orientation & ORIENTATION_FLIP_X)
                {
    		        xind=2047-xind;
    		    }

/* BODGE CODE COMMENTED OUT
                if(Machine->orientation & ORIENTATION_FLIP_Y)
                {
  		            yind=2047-yind;
  		        }
*/

                tmp = (unsigned short*)&source_bitmap->line[yind&ROTATE_MASK_HEIGHT][(xind&ROTATE_MASK_WIDTH)*2];
                pixel= *tmp;
	    		if(pixel) dest_line[dest_x]=pixel;

		    	tmp_x += right_dx;
			    tmp_y += right_dy;
    		}
	    	start_x += down_dx;
		    start_y += down_dy;
	    }
    }
    else
    {
    	unsigned char *dest_line,pixel;

    	for( dest_y=0; dest_y<dest_bitmap->height; dest_y++ )
	    {
    		dest_line = dest_bitmap->line[dest_y];
		    tmp_x = start_x;
	    	tmp_y = start_y;

		    for( dest_x=dest_bitmap->width-1; dest_x>=0; dest_x-- )
    		{
	    	    int xind=tmp_x>>16;
		        int yind=tmp_y>>16;

		        xind=2047-xind;

                pixel= source_bitmap->line[yind&ROTATE_MASK_HEIGHT][xind&ROTATE_MASK_WIDTH];
	    		if(pixel) dest_line[dest_x]=pixel;

		    	tmp_x += right_dx;
			    tmp_y += right_dy;
    		}
	    	start_x += down_dx;
		    start_y += down_dy;
	    }
	}
}


static void draw_layerROZ( struct osd_bitmap *bitmap)
{
	static struct rectangle rozrect={0,ROTATE_PIXEL_WIDTH,0,ROTATE_PIXEL_HEIGHT};
    int right_dx,right_dy,down_dx,down_dy,start_x,start_y;
	int xloop,yloop,addr,tile;

    /* A much faster way to do this would be to skip the temporary rotation */
    /* bit map and generate the dest rotated image direct from the tile/ram */
    /* image/data.                                                          */
    //namcos2_68k_roz_ram_dirty=ROZ_REDRAW_ALL;

    if(namcos2_68k_roz_ram_dirty!=ROZ_CLEAN_TILE)
    {
        /* If its a redraw then wipe the bitmap clean first */
        if(namcos2_68k_roz_ram_dirty==ROZ_REDRAW_ALL) fillbitmap(rotate_bitmap,0,0);

        for(yloop=0;yloop<ROTATE_TILE_HEIGHT;yloop++)
        {
            for(xloop=0;xloop<ROTATE_TILE_WIDTH;xloop++)
            {
                if(namcos2_68k_roz_ram_dirty==ROZ_REDRAW_ALL || namcos2_68k_roz_dirty_buffer[(yloop<<8)+xloop]==ROZ_DIRTY_TILE)
                {
                    /* First clean the area under the tile because of the transparency pen issues */
                    if(namcos2_68k_roz_ram_dirty!=ROZ_REDRAW_ALL)
                    {
                        struct rectangle rect={0,0,0,0};
                        rect.min_x=xloop*8;
                        rect.max_x=rect.min_x+7;
                        rect.min_y=yloop*8;
                        rect.max_y=rect.min_y+7;
                        fillbitmap(rotate_bitmap,0,&rect);
                    }

                    /* Now paint the tile */
                    addr=((yloop<<8) + xloop)<<1;
        		    tile=READ_WORD(&namcos2_68k_roz_ram[addr]);
        	    	tile&=0xffff;
            		drawgfx(rotate_bitmap,Machine->gfx[GFX_ROZ],
		        		tile,
			        	(namcos2_68k_sprite_bank_r(0)>>8)&0x000f,     /* Selected colour bank */
				        0,0,
        				xloop*8,yloop*8,
	        			&rozrect,TRANSPARENCY_PEN,0xff);
	        	}
                namcos2_68k_roz_dirty_buffer[(yloop<<8)+xloop]=ROZ_CLEAN_TILE;
            }
        }
    }
    namcos2_68k_roz_ram_dirty=ROZ_CLEAN_TILE;

    /* These need to be sign extended for arithmetic useage */
    right_dx = namcos2_68k_roz_ctrl_r(0x06);
    right_dy = namcos2_68k_roz_ctrl_r(0x02);
    down_dx  = namcos2_68k_roz_ctrl_r(0x04);
    down_dy  = namcos2_68k_roz_ctrl_r(0x00);
    start_y  = namcos2_68k_roz_ctrl_r(0x0a);
    start_x  = namcos2_68k_roz_ctrl_r(0x08);

    if(right_dx&0x8000) right_dx|=0xffff0000;
    if(right_dy&0x8000) right_dy|=0xffff0000;
    if(down_dx &0x8000) down_dx |=0xffff0000;
    if(down_dy &0x8000) down_dy |=0xffff0000;
//  if(start_y &0x8000) start_y |=0xffff0000;
//  if(start_x &0x8000) start_x |=0xffff0000;

	/* Correct to 16 bit fixed point from 8/12 bit */
	right_dx <<=8;
	right_dy <<=8;
	down_dx  <<=8;
	down_dy  <<=8;
	start_x  <<=12;
	start_y  <<=12;

//  start_x=0;
//  start_y=0;
//  right_dx=0x92000;
//  right_dy=0;
//  down_dx =0;
//  down_dy =0x92000;


    rotatebitmap( bitmap, rotate_bitmap, start_x, start_y, right_dx, right_dy, down_dx, down_dy );
}

static void draw_layer64( struct osd_bitmap *bitmap, int mem_offset, int xoff, int yoff, int colour )
{
	int xloop,yloop,addr,tile,tmp;

    for(yloop=0;yloop<64;yloop++)
    {
        for(xloop=0;xloop<64;xloop++)
        {
            addr=((yloop<<6) + xloop)<<1;
    		tile=(READ_WORD(&videoram[mem_offset+addr]))&0xffff;
            /* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
            tmp=((tile&0xc000)>>3) | ((tile&0x3800)<<2);
            tile=(tile&0x07ff)|tmp;

       		drawgfx(bitmap,Machine->gfx[GFX_CHR],
	    		tile,
		    	colour,
			    0,0,
   				((xloop<<3)-xoff-48+4)&0x1ff,((yloop<<3)-yoff-24)&0x1ff,
    			&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
    			/* BEWARE - FUDGE FACTORS IN OPERATION */
        }
    }
}

static void draw_layer36( struct osd_bitmap *bitmap, int mem_offset, int colour )
{
	int xloop,yloop,addr,tile,tmp,linetot;

    for(yloop=0;yloop<28;yloop++)
    {
        linetot=yloop*36;
        for(xloop=0;xloop<36;xloop++)
        {
            addr=(linetot+xloop)<<1;
    		tile=(READ_WORD(&videoram[mem_offset+addr]))&0xffff;
            /* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
            tmp=((tile&0xc000)>>3) | ((tile&0x3800)<<2);
            tile=(tile&0x07ff)|tmp;

       		drawgfx(bitmap,Machine->gfx[GFX_CHR],
	    		tile,
		    	colour,
			    0,0,
   				xloop<<3,yloop<<3,
    			&Machine->drv->visible_area,TRANSPARENCY_PEN,0x00);
        }
    }
}


static void draw_sprites( struct osd_bitmap *bitmap, int priority )
{
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;
    struct rectangle rect;

	offset=(namcos2_68k_sprite_bank_r(0)&0x000f)*(128*8);

	for(loop=0;loop < 128;loop++)
	{
	    /****************************************
	    * Sprite data is 8 byte packed format   *
	    *                                       *
        * Offset 0,1                            *
        *   Sprite Y position           D00-D08 *
	    *   Sprite Size 16/32           D09     *
	    *   Sprite Size Y ????          D10-D15 *
	    *                                       *
	    * Offset 2,3                            *
	    *   Sprite Quadrant             D00-D01 *
	    *   Sprite Number               D02-D12 *
	    *   Sprite ROM Bank select      D13     *
	    *   Sprite flip X               D14     *
	    *   Sprite flip Y               D15     *
	    *                                       *
	    * Offset 4,5                            *
	    *   Sprite X position           D00-D10 *
	    *                                       *
	    * Offset 6,7                            *
	    *   Sprite priority             D00-D02 *
	    *   Sprite colour index         D04-D07 *
	    *   Sprite Size X ????          D10-D15 *
	    *                                       *
	    ****************************************/

		offset0 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+0]);
		offset2 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+2]);
		offset4 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+4]);
		offset6 = READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+6]);

        /* Fetch sprite size registers */

	    sizey=((offset0>>10)&0x3f)+1;
   		sizex=(offset6>>10)&0x3f;

	    if((offset0&0x0200)==0) sizex>>=1;

		if(sizey && sizex && (offset6&0x0007)==priority)
		{
		    rect=Machine->drv->visible_area;

    		sprn=(offset2>>2)&0x7ff;
    		spr_region=(offset2&0x2000)?GFX_OBJ2:GFX_OBJ1;

	    	ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
		    xpos=(offset4&0x07ff)-0x50+0x07;

    		flipy=offset2&0x8000;
	    	flipx=offset2&0x4000;

	    	scalex=((sizex<<16)/((offset0&0x0200)?0x20:0x10));
		    scaley=((sizey<<16)/((offset0&0x0200)?0x20:0x10));

	        /* Set the clipping rect to mask off the other portion of the sprite */
            rect.min_x=xpos;
            rect.max_x=xpos+(sizex-1);
            rect.min_y=ypos;
            rect.max_y=ypos+(sizey-1);

		    if((offset0&0x0200)==0)
		    {
		        if(((offset2&0x0001) && !flipx) || (!(offset2&0x0001) && flipx)) xpos-=sizex;
		        if(((offset2&0x0002) && !flipy) || (!(offset2&0x0002) && flipy)) ypos-=sizey;
		    }

            if((scalex==(1<<16)) && (scaley==(1<<16)))
            {
    			drawgfx(bitmap,Machine->gfx[spr_region],
	    			sprn,
		    		(offset6>>4)&0x000f,     /* Selected colour bank */
			    	flipx,flipy,
				    xpos,ypos,
    				&rect,TRANSPARENCY_PEN,0xff);
    		}
    		else if(scalex && scaley)
            {
    			drawgfxzoom(bitmap,Machine->gfx[spr_region],
	    			sprn,
		    		(offset6>>4)&0x000f,     /* Selected colour bank */
			    	flipx,flipy,
				    xpos,ypos,
    				&rect,TRANSPARENCY_PEN,0xff,
    				scalex,scaley);
    		}
		}
	}
}


void namcos2_vh_update(struct osd_bitmap *bitmap, int full_refresh)
{
    int priority,loop,offset,count;
    int colour_codes_used[NAMCOS2_COLOUR_CODES];
	static int show[10] = {0,0,0,0,0,0,0,0,0,0};
	static int planeshow=0;

#ifdef MAME_DEBUG
	static int regshow = 0;
	if(keyboard_pressed(KEYCODE_Z))     { while( keyboard_pressed(KEYCODE_Z));     show[0]=(show[0])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_X))     { while( keyboard_pressed(KEYCODE_X));     show[1]=(show[1])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_C))     { while( keyboard_pressed(KEYCODE_C));     show[2]=(show[2])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_V))     { while( keyboard_pressed(KEYCODE_V));     show[3]=(show[3])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_B))     { while( keyboard_pressed(KEYCODE_B));     show[4]=(show[4])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_N))     { while( keyboard_pressed(KEYCODE_N));     show[5]=(show[5])?0:1; planeshow=1; }
	if(keyboard_pressed(KEYCODE_COMMA)) { while( keyboard_pressed(KEYCODE_COMMA)); show[6]=(show[6])?0:1; planeshow=1; }
#endif

    palette_init_used_colors();

    for(loop=0;loop<NAMCOS2_COLOUR_CODES;loop++) colour_codes_used[loop]=0;

    /* Mark off all of the colour codes used by the sprites */
   	offset=(namcos2_68k_sprite_bank_r(0)&0x000f)*(128*8);
   	for(loop=0;loop<128;loop++)
    {
        int code=(READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+2])>>2)&0x7ff;
        if(code!=0) colour_codes_used[(READ_WORD(&namcos2_sprite_ram[offset+(loop*8)+6])>>4)&0x000f]=1;
    }
    /* Mark off the colour codes used by scroll & ROZ planes */
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x30)&0x000f)]=1;
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x32)&0x000f)]=1;
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x34)&0x000f)]=1;
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x36)&0x000f)]=1;
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x38)&0x000f)]=1;
    colour_codes_used[0x10+(namcos2_68k_vram_ctrl_r(0x3a)&0x000f)]=1;
    colour_codes_used[(namcos2_68k_sprite_bank_r(0)>>8)&0x000f]=1;

    /* Mark colour indexes used by text planes/roz/sprites */
    for(loop=0;loop<NAMCOS2_COLOUR_CODES;loop++)
    {
        if(colour_codes_used[loop])
        {
            for(count=0;count<256;count++) palette_used_colors[(loop*256)+count] |= PALETTE_COLOR_VISIBLE;
        }
    }

    if (palette_recalc())
    {
        /* Mark all planes as dirty */
        namcos2_68k_roz_ram_dirty=ROZ_REDRAW_ALL;
    }


    /* Scrub the bitmap clean */
    fillbitmap(bitmap,0,0);

    /* Render the screen */
    for(priority=1;priority<=7;priority++)
    {

        /* This makes most games look/work correctly BUT assault gives problems  */
        /* as one of the fixed planes has the same priority as the ROZ layer     */
        /* which means the ROZ layer is obscured as the fixed layer is not using */
        /* the transparent pen, hence its commented out currently                */

        /* Draw ROZ if enabled */
/*      if((((namcos2_68k_sprite_bank_r(0)>>12)&0x07)==priority && !planeshow) || ((show[6]) && planeshow)) draw_layerROZ( bitmap); */

        /* Sprites */
/*	    draw_sprites( bitmap,priority ); */

        /* Text planes */
 	    if(((namcos2_68k_vram_ctrl_r(0x20)&0x07)==priority && !planeshow) || ((show[0]) && planeshow)) draw_layer64( bitmap, 0x0000, (namcos2_68k_vram_ctrl_r(0x02)&0x0fff)+4, namcos2_68k_vram_ctrl_r(0x06)&0x0fff, namcos2_68k_vram_ctrl_r(0x30)&0x000f );
	    if(((namcos2_68k_vram_ctrl_r(0x22)&0x07)==priority && !planeshow) || ((show[1]) && planeshow)) draw_layer64( bitmap, 0x2000, (namcos2_68k_vram_ctrl_r(0x0a)&0x0fff)+2, namcos2_68k_vram_ctrl_r(0x0e)&0x0fff, namcos2_68k_vram_ctrl_r(0x32)&0x000f );
	    if(((namcos2_68k_vram_ctrl_r(0x24)&0x07)==priority && !planeshow) || ((show[2]) && planeshow)) draw_layer64( bitmap, 0x4000, (namcos2_68k_vram_ctrl_r(0x12)&0x0fff)+1, namcos2_68k_vram_ctrl_r(0x16)&0x0fff, namcos2_68k_vram_ctrl_r(0x34)&0x000f );
	    if(((namcos2_68k_vram_ctrl_r(0x26)&0x07)==priority && !planeshow) || ((show[3]) && planeshow)) draw_layer64( bitmap, 0x6000, (namcos2_68k_vram_ctrl_r(0x1a)&0x0fff)+0, namcos2_68k_vram_ctrl_r(0x1e)&0x0fff, namcos2_68k_vram_ctrl_r(0x36)&0x000f );
    	if(((namcos2_68k_vram_ctrl_r(0x28)&0x07)==priority && !planeshow) || ((show[4]) && planeshow)) draw_layer36( bitmap, 0x8010, namcos2_68k_vram_ctrl_r(0x38)&0x000f );
	    if(((namcos2_68k_vram_ctrl_r(0x2a)&0x07)==priority && !planeshow) || ((show[5]) && planeshow)) draw_layer36( bitmap, 0x8810, namcos2_68k_vram_ctrl_r(0x3a)&0x000f );

        /* Draw ROZ if enabled */
        if((((namcos2_68k_sprite_bank_r(0)>>12)&0x07)==priority && !planeshow) || ((show[6]) && planeshow)) draw_layerROZ( bitmap);

        /* Sprites */
	    draw_sprites( bitmap,priority );

        /* Only one runthrough if in planeshow mode */
        if(planeshow) break;
    }


#ifdef MAME_DEBUG

    /* Debug memory plane display */
    if(planeshow)
    {
        char buffer[256];
        sprintf(buffer,"Planes %d%d%d%d %d%d %d",show[0],show[1],show[2],show[3],show[4],show[5],show[6]);
        ui_text(buffer,4,4);
    }

	if(keyboard_pressed(KEYCODE_L)) { while( keyboard_pressed(KEYCODE_L)); regshow++;if(regshow>5) regshow=0; }
	if(regshow) show_reg(regshow);

#endif
}
