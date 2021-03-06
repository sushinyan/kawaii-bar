/* kawaii-bar.h */

#define BARW		1366		/* width of bar in pixels */
#define BARH		22		/* height of bar in pixels */

#define SYSTRAY_W	20		/* width of system tray in pixels, 0 if none */
#define PROGRESS_BAR_W	50		/* width of progress bar in pixels */
static gboolean topbar	= TRUE;		/* TRUE for bar to be displayed at top of screen */

/* colors */
static const char *bgcolor	    = "#1d1f21";

static const char *progress_fg	    = "#a1efe4";
static const char *progress_bg	    = "#75715e";
static const char *progress_urg	    = "#fd971f";

static const char *desktop_norm	    = "#75715e";
static const char *desktop_sel	    = "#f9f8f5";
static const char *desktop_line	    = "#fd971f";

static const char *stats_text	    = "#fd971f";

static const char *clock_primary    = "#66d9ef";
static const char *clock_secondary  = "#f9f8f5";

static const char *icon_color	    = "#f9f8f5";

static const char *separator_color  = "#66d9ef";

/* fonts */
static char *font	    = "Inconsolata 14";
static char *stats_font	    = "Inconsolata 11";
static char *icons_font	    = "Stlarch 10";

static char *clock_font_lg  = "Agency FB Bold 18";
static char *clock_font_sm  = "Inconsolata 11";

/* stats glyphs -- from stlarch font */
char *cpu_icon		= "";	/* uE026 */
char *mem_icon		= "";	/* uE028 */
char *bat_low_icon	= "";	/* uE031 */
char *bat_med_icon	= "";	/* uE032 */
char *bat_high_icon	= "";	/* uE033 */
char *vol_low_icon	= "";	/* uE050 */
char *vol_high_icon	= "";	/* uE05D */
char *vol_mute_icon	= "";	/* uE04f */

#define VOL_HIGH    70
#define BAT_LOW	    11
#define BAT_HIGH    70
