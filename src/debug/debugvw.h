/*###################################################################################################
**
**
**		debugvw.h
**		Debugger view engine.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef __DEBUGVIEW_H__
#define __DEBUGVIEW_H__


/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* types passed to debug_view_alloc */
#define DVT_CONSOLE							(1)
#define DVT_REGISTERS						(2)
#define DVT_DISASSEMBLY						(3)
#define DVT_MEMORY							(4)
#define DVT_TIMERS							(5)
#define DVT_ALLOCS							(6)

/* properties available for all views */
#define DVP_VISIBLE_ROWS					(1)		/* r/w - UINT32 */
#define DVP_VISIBLE_COLS					(2)		/* r/w - UINT32 */
#define DVP_TOTAL_ROWS						(3)		/* r/w - UINT32 */
#define DVP_TOTAL_COLS						(4)		/* r/w - UINT32 */
#define DVP_TOP_ROW							(5)		/* r/w - UINT32 */
#define DVP_LEFT_COL						(6)		/* r/w - UINT32 */
#define DVP_UPDATE_CALLBACK					(7)		/* r/w - void (*update)(struct debug_view *) */ 
#define DVP_VIEW_DATA						(8)		/* r/o - struct debug_view_char * */ 
#define DVP_CPUNUM							(9)		/* r/w - UINT32 */
#define DVP_SUPPORTS_CURSOR					(10)	/* r/o - UINT32 */
#define DVP_CURSOR_VISIBLE					(11)	/* r/w - UINT32 */
#define DVP_CURSOR_ROW						(12)	/* r/w - UINT32 */
#define DVP_CURSOR_COL						(13)	/* r/w - UINT32 */
#define DVP_CHARACTER						(14)	/* w/o - UINT32 */

/* properties available for memory/disassembly views */
#define DVP_EXPRESSION						(100)	/* const char * */
#define DVP_TRACK_LIVE						(101)	/* UINT32 */

/* properties available for memory views */
#define DVP_SPACENUM						(102)	/* UINT32 */
#define DVP_BYTES_PER_CHUNK					(103)	/* UINT32 */
#define DVP_REVERSE_VIEW					(104)	/* UINT32 */
#define DVP_ASCII_VIEW						(105)	/* UINT32 */

/* attribute bits for debug_view_char.attrib */
#define DCA_NORMAL							(0x00)	/* in Windows: black on white */
#define DCA_CHANGED							(0x01)	/* in Windows: red foreground */
#define DCA_SELECTED						(0x02)	/* in Windows: light red background */
#define DCA_INVALID							(0x04)	/* in Windows: dark blue foreground */
#define DCA_DISABLED						(0x08)	/* in Windows: darker foreground */
#define DCA_ANCILLARY						(0x10)	/* in Windows: grey background */
#define DCA_CURRENT							(0x20)	/* in Windows: yellow background */

/* special characters that can be passed as a DVP_CHARACTER */
#define DCH_UP								(1)		/* up arrow */
#define DCH_DOWN							(2)		/* down arrow */
#define DCH_LEFT							(3)		/* left arrow */
#define DCH_RIGHT							(4)		/* right arrow */



/*###################################################################################################
**	MACROS
**#################################################################################################*/



/*###################################################################################################
**	TYPE DEFINITIONS
**#################################################################################################*/

/* opaque structure representing a debug view */
struct debug_view;

/* a single "character" in the debug view has an ASCII value and an attribute byte */
struct debug_view_char
{
	UINT8		byte;
	UINT8		attrib;
};



/*###################################################################################################
**	FUNCTION PROTOTYPES
**#################################################################################################*/

/* initialization */
void				debug_view_init(void);
void				debug_view_exit(void);

/* view creation/deletion */
struct debug_view *	debug_view_alloc(int type);
void				debug_view_free(struct debug_view *view);

/* property management */
void				debug_view_get_property(struct debug_view *view, int property, void *value);
void				debug_view_set_property(struct debug_view *view, int property, const void *value);

/* update management */
void				debug_view_begin_update(struct debug_view *view);
void				debug_view_end_update(struct debug_view *view);
void				debug_view_update_all(void);
void				debug_view_update_type(int type);



/*###################################################################################################
**	INLINE HELPERS
**#################################################################################################*/

INLINE UINT32 debug_view_get_property_UINT32(struct debug_view *view, int property)
{
	UINT32 value;
	debug_view_get_property(view, property, &value);
	return value;
}

INLINE void debug_view_set_property_UINT32(struct debug_view *view, int property, UINT32 value)
{
	debug_view_set_property(view, property, &value);
}


INLINE const char *debug_view_get_property_string(struct debug_view *view, int property)
{
	const char *value;
	debug_view_get_property(view, property, (void *)&value);
	return value;
}

INLINE void debug_view_set_property_string(struct debug_view *view, int property, const char *value)
{
	debug_view_set_property(view, property, value);
}


INLINE void *debug_view_get_property_ptr(struct debug_view *view, int property)
{
	void *value;
	debug_view_get_property(view, property, &value);
	return value;
}

INLINE void debug_view_set_property_ptr(struct debug_view *view, int property, void *value)
{
	debug_view_set_property(view, property, value);
}


INLINE genf *debug_view_get_property_fct(struct debug_view *view, int property)
{
	genf *value;
	debug_view_get_property(view, property, &value);
	return value;
}

INLINE void debug_view_set_property_fct(struct debug_view *view, int property, genf *value)
{
	debug_view_set_property(view, property, (void *) value);
}

#endif
