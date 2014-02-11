
#define BARW		1366		/* width of bar in pixels */
#define BARH		22		/* height of bar in pixels */

#define SYSTRAY_W	20		/* width of system tray in pixels, 0 if none */
#define PROGRESS_BAR_W	50		/* width of progress bar in pixels */
static gboolean topbar = TRUE;		/* TRUE for bar to be displayed at top of screen */

/* colors */
static const char *bgcolor	= "#000000";

static const char *progress_fg	= "#0088cc";
static const char *progress_bg	= "#4b4b4b";
static const char *progress_urg	= "#ff0000";

static const char *desktop_norm	= "#646464";
static const char *desktop_sel	= "#ffffff";
static const char *desktop_line = "#00c5cd";

static const char *stats_text	= "#00c5cd";

static const char *clock_primary = "#1793d1";
static const char *clock_secondary = "#ffffff";

/* fonts */
static const char *font	= "Sans 12";
static const char *stats_font = "Sans 9";
