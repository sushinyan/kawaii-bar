#include <gtk-3.0/gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <pango/pango.h>
#include <string.h> // for strtok()
#include <stdlib.h> // for malloc()

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include <time.h>
#include <alsa/asoundlib.h>

#include "kawaii-bar.h"

/* init desktops state */
static char desktop_status[] = {'O','f','f','f','f','f','f','f','f','f'};
char *desktops[10] = {"1","2","3","4","5","6","7","8","9","0"};

typedef struct {
	char *hour;
	char *minute;
	char *dayofweek;
	char *dayofmonth;
	char *year;
	char *month;
	char *ampm;
} Clock;

static void draw(cairo_t *cr);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void init_window(GtkWidget *window);
static gboolean on_update(GtkWidget *widget);
static void draw_desktops(cairo_t *cr);
static void draw_clock(cairo_t *cr);
static int textw(cairo_t *cr, char *text);
static void draw_text(cairo_t *cr, char *text, gint x, gint y, GdkRGBA color);
static void get_desktop_status(void);
static void draw_stats(cairo_t *cr);
static void draw_arch_icon(cairo_t *cr);
static Clock initclock(void);

/* init stats stuff */
FILE *infile;
long jif1, jif2, jif3, jif4, lnum1, lnum2, lnum3, lnum4;

char *tiling_state;

typedef struct {
	PangoLayout *plo;
	PangoFontDescription *pfd;
	int x;
	int w;
} DC;
static DC dc;

/* Events */
static gboolean
on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	draw(cr);
	return FALSE;
}

static gboolean
on_update(GtkWidget *widget)
{
	gtk_widget_queue_draw(widget);
	return TRUE;
}

static void
setfont(cairo_t *cr, char *fontstr)
{
	dc.pfd = pango_font_description_from_string(fontstr);
	pango_layout_set_font_description(dc.plo,dc.pfd);
	pango_font_description_free(dc.pfd);
}

static void
init_window(GtkWidget *window)
{
	GdkScreen *screen;
	GdkVisual *visual;
	int width, height;

	gtk_widget_set_app_paintable(window, TRUE);
	screen = gdk_screen_get_default();
	visual = gdk_screen_get_rgba_visual(screen);
	gtk_widget_set_visual(window, visual);

	gtk_window_set_default_size(GTK_WINDOW(window), BARW, BARH);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);

	width = gdk_screen_get_width(screen);
	height = gdk_screen_get_height(screen);

	gtk_window_move(GTK_WINDOW(window), (width - BARW) / 2, topbar ? 0 : height - BARH);
}

static void
draw(cairo_t *cr)
{
	GdkRGBA c;
	gdk_rgba_parse(&c, bgcolor);
	// temporary fix, need to init elsewhere.
	if(!dc.plo)
		dc.plo = pango_cairo_create_layout(cr);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_paint(cr);
	
	draw_arch_icon(cr);
	draw_desktops(cr);
	draw_clock(cr);
	draw_stats(cr);
}

static void
draw_text(cairo_t *cr, char *text, gint x, gint y, GdkRGBA color)
{
	pango_layout_set_text(dc.plo, text, -1);
	//cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
	cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
	cairo_move_to(cr, x, y);
	pango_cairo_show_layout(cr, dc.plo);
}

/* draws the indicator for selected desktop */
static void
draw_line(cairo_t *cr)
{
	GdkRGBA c;
	gdk_rgba_parse(&c, desktop_line);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_set_line_width(cr, 3);
	cairo_move_to(cr, dc.x, 20);

	/* lazy fix to force monospace -- probably not needed anymore */
	dc.w = textw(cr, "X");

	cairo_line_to(cr, dc.x + dc.w, 20);
	cairo_stroke(cr);
}

static void
draw_arch_icon(cairo_t *cr)
{
	int w, h;
	cairo_surface_t *icon;
	icon = cairo_image_surface_create_from_png("/usr/share/icons/kawaii-bar/arch.png");
	w = cairo_image_surface_get_width(icon);
	h = cairo_image_surface_get_height(icon);
	cairo_save(cr);
	cairo_scale(cr, 20.0 / w, 20.0 / h);
	cairo_set_source_surface(cr, icon, 10, 0);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_surface_destroy(icon);
}

static void
draw_desktops(cairo_t *cr)
{
	int i;
	GdkRGBA c;
	dc.x = 25;
	int y = 0;

	get_desktop_status();

	setfont(cr, "Sans 12");

	for(i = 0; i < 10; i++) {
		char dd = *(desktops[i]);

		gdk_rgba_parse(&c, tolower(desktop_status[i]) == 'o' ? desktop_sel : desktop_norm);
		if(desktop_status[i] == 'F' || desktop_status[i] == 'O')
			draw_line(cr);
		draw_text(cr, desktops[i], dc.x, y, c);
		//pango_layout_get_pixel_size(dc.plo, &dc.w, NULL);
		dc.w = textw(cr, desktops[i]) + 2;

		dc.x += dc.w;
	}
	dc.x += 5; // padding for tiling status
	gdk_rgba_parse(&c, desktop_sel);
	draw_text(cr, tiling_state, dc.x, y, c);
}

static void
get_desktop_status(void)
{
	fd_set set;
	struct timeval timeout;
	int rv;
	FILE *infile;
	int fd;
	char *buf = NULL;
	size_t len = 0;

	fd = open("/tmp/panel-fifo", O_RDWR | O_NONBLOCK);
	FD_ZERO(&set);
	FD_SET(fd, &set);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	rv = select(fd + 1, &set, NULL, NULL, &timeout);
	if(rv > 0) {
		infile = fdopen(fd, "r");
		getline(&buf, &len, infile);
		fclose(infile);
	}
	close(fd);

	if(buf != NULL) {
		int i = 0;
		char *pch;
		char *test;
		char *tmp[10];

		test = strtok_r(buf, ":", &pch);
		while(test != NULL && i < 10) {
			test = strtok_r(NULL, ":", &pch);
			tmp[i++] = &test[0];
			char c;
			c = *(tmp[i - 1]);
			desktop_status[i-1] = c;
		}
		// last line is tiling state
		tiling_state = strtok_r(NULL, ":", &pch);
	}
}

static int
textw(cairo_t *cr, char *text)
{
	int len = strlen(text);
	
	PangoRectangle r;
	pango_layout_set_text(dc.plo, text, len);
	pango_layout_get_extents(dc.plo, &r, 0);

	return r.width / PANGO_SCALE;
}

static Clock
initclock(void)
{
	Clock clock;
	time_t current;
	time(&current);
	char buf[12];

	clock.hour = malloc(12);
	clock.minute = malloc(12);
	clock.dayofweek = malloc(12);
	clock.dayofmonth = malloc(12);
	clock.year = malloc(12);
	clock.month = malloc(12);
	clock.ampm = malloc(12);

	strftime(buf, 12,"%I", localtime(&current));
	strcpy(clock.hour, buf);
	strftime(buf, 12, "%M", localtime(&current));
	strcpy(clock.minute, buf);
	strftime(buf, 12, "%d", localtime(&current));
	strcpy(clock.dayofmonth, buf);
	strftime(buf, 12, "%B", localtime(&current));
	strcpy(clock.month, buf);
	strftime(buf, 12, "%A", localtime(&current));
	strcpy(clock.dayofweek, buf);
	strftime(buf, 12, "%Y", localtime(&current));
	strcpy(clock.year, buf);
	strcat(clock.year, " CE");
	strftime(buf, 12, "%p", localtime(&current));
	strcpy(clock.ampm, buf);

	return clock;
}

static int
get_clock_w(cairo_t *cr) {
	// testing code to calculate width of clock
	int x, w;

	w = 0;
	// init this globally -- no need to init twice
	Clock clock = initclock();

	setfont(cr, "DIN condensed 8");
	pango_layout_set_text(dc.plo, clock.ampm, -1);
	pango_layout_get_pixel_size(dc.plo, &x, NULL);
	w += x;

	setfont(cr, "Agency FB Bold 18");
	pango_layout_set_text(dc.plo, clock.hour , -1);
	pango_layout_get_pixel_size(dc.plo, &x, NULL);
	w += x;

	pango_layout_set_text(dc.plo, clock.minute, -1);
	pango_layout_get_pixel_size(dc.plo, &x, NULL);
	w += x;

	setfont(cr, "DIN Condensed 8");
	pango_layout_set_text(dc.plo, clock.dayofweek, -1);
	pango_layout_get_pixel_size(dc.plo, &x, NULL);
	w += x;

	pango_layout_set_text(dc.plo, clock.month, -1);
	pango_layout_get_pixel_size(dc.plo, &x, NULL);
	w += x;

	w += 16; // padding

	return w;
}

static void
draw_clock(cairo_t *cr)
{
	Clock clock = initclock();
	GdkRGBA c;

	dc.x = (BARW - get_clock_w(cr)) / 2;

	setfont(cr, "DIN condensed 8");
	pango_layout_set_text(dc.plo, clock.ampm, -1);
	gdk_rgba_parse(&c, clock_primary);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, strcmp(clock.ampm, "AM") == 0 ? -3 : 9);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, clock.ampm);
	dc.x += dc.w + 4;

	setfont(cr, "Agency FB Bold 18");
	pango_layout_set_text(dc.plo, clock.hour , -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, -4);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, clock.hour);
	dc.x += dc.w + 4;

	pango_layout_set_text(dc.plo, clock.minute, -1);
	gdk_rgba_parse(&c, clock_secondary);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, -4);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, clock.minute);
	dc.x += dc.w + 4;

	setfont(cr, "DIN Condensed 8");
	pango_layout_set_text(dc.plo, clock.dayofweek, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);	
	cairo_move_to(cr, dc.x, -1);
	pango_cairo_show_layout(cr, dc.plo);

	pango_layout_set_text(dc.plo, clock.dayofmonth, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, 9);
	pango_cairo_show_layout(cr, dc.plo);

	// day of week string is always longer than day of month
	dc.w = textw(cr, clock.dayofweek);
	dc.x += dc.w + 4;

	pango_layout_set_text(dc.plo, clock.year, -1);
	gdk_rgba_parse(&c, clock_primary);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, -1);
	pango_cairo_show_layout(cr, dc.plo);

	pango_layout_set_text(dc.plo, clock.month, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, 9);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, clock.month);
	dc.x += dc.w;

	// free clock variables -- temporary fix
	free(clock.hour); free(clock.minute); free(clock.month);
	free(clock.dayofweek); free(clock.dayofmonth); free(clock.year);
}

static void
draw_progress_bar(cairo_t *cr, int percent, gboolean urgent)
{
	GdkRGBA fg, bg;

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(cr, 3);
	gdk_rgba_parse(&fg, urgent ? progress_urg : progress_fg);
	gdk_rgba_parse(&bg, progress_bg);

	// testing line to make sure percent isnt over 100
	percent = percent > 100 ? 100 : percent;

	// this has to be drawn first for ends of progress to look right
	// can possibly fix by changing line cap ???
	if(percent != 100) {
	// draw bar from progress to end
		cairo_set_source_rgba(cr, bg.red, bg.green, bg.blue, bg.alpha);
		cairo_move_to(cr, dc.x + (PROGRESS_BAR_W * ((float)percent / 100)), 18);
		cairo_line_to(cr, dc.x + PROGRESS_BAR_W , 18);
		cairo_stroke(cr);
	}

	if(percent != 0) {
	// draw bar from beginning to progress
		cairo_set_source_rgba(cr, fg.red, fg.green, fg.blue, fg.alpha);
		//cairo_set_line_width(cr, 3);
		cairo_move_to(cr, dc.x, 18);
		cairo_line_to(cr, dc.x + PROGRESS_BAR_W * ((float)percent / 100), 18);
		cairo_stroke(cr);
	}
}

static void
draw_stats(cairo_t *cr)
{
	GdkRGBA c;
	gdk_rgba_parse(&c, stats_text);

	dc.x = 950;
	int y = 0;
	int w, h;
	char buf[20];

	cairo_surface_t *icon;	// free this later

	// DRAW VOLUME ICON	
	icon = cairo_image_surface_create_from_png("/usr/share/icons/kawaii-bar/vol.png");
	cairo_rectangle(cr, dc.x, 0, dc.x + 20, 20);
	cairo_set_source_surface(cr, icon, dc.x, 3);
	cairo_fill(cr);
	cairo_surface_destroy(icon);
	dc.x += 20; // adjust for icon width

	// GET VOLUME
	long max = 0, min = 0, vol = 0;
        int mute = 0;

        snd_mixer_t *handle;
        snd_mixer_elem_t *pcm_mixer, *mas_mixer;
        snd_mixer_selem_id_t *vol_info, *mute_info;

        snd_mixer_open(&handle, 0);
        snd_mixer_attach(handle, "default");
        snd_mixer_selem_register(handle, NULL, NULL);
        snd_mixer_load(handle);
        snd_mixer_selem_id_malloc(&vol_info);
        snd_mixer_selem_id_malloc(&mute_info);
        snd_mixer_selem_id_set_name(vol_info, "Master");
        snd_mixer_selem_id_set_name(mute_info, "Master"); // VOL_CH was used here...
        pcm_mixer = snd_mixer_find_selem(handle, vol_info);
        mas_mixer = snd_mixer_find_selem(handle, mute_info);
        snd_mixer_selem_get_playback_volume_range((snd_mixer_elem_t *)pcm_mixer, &min, &max);
        snd_mixer_selem_get_playback_volume((snd_mixer_elem_t *)pcm_mixer, SND_MIXER_SCHN_MONO, &vol);
        snd_mixer_selem_get_playback_switch(mas_mixer, SND_MIXER_SCHN_MONO, &mute);
        sprintf(buf, !(mute) ? "Mute" : "%d%%", (int)vol * 100 / (int)max);
        if(vol_info)
                snd_mixer_selem_id_free(vol_info);
        if(mute_info)
                snd_mixer_selem_id_free(mute_info);
        if(handle)
                snd_mixer_close(handle);

	setfont(cr, "Sans 9");
	pango_layout_set_text(dc.plo, buf, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, y);
	pango_cairo_show_layout(cr, dc.plo);
	
	// DRAW VOLUME PROGRESS BAR
	draw_progress_bar(cr, !(mute) ? 0 : (int)vol * 100 / (int)max, FALSE);
	dc.x += PROGRESS_BAR_W + 10; // adjust dc.x for progress bar + 10px pad

	// DRAW CPU ICON
	icon = cairo_image_surface_create_from_png("/usr/share/icons/kawaii-bar/cpu.png");

	cairo_rectangle(cr, dc.x, 0, dc.x + 20, 20);
	w = cairo_image_surface_get_width(icon);
	h = cairo_image_surface_get_height(icon);
	cairo_set_source_surface(cr, icon, dc.x, 3);
	cairo_fill(cr);
	cairo_surface_destroy(icon);
	dc.x += 20; // adjust dc.x for icon width

	// GET CPU
	int num;
	infile = fopen("/proc/stat", "r");
	fscanf(infile, "cpu %ld %ld %ld %ld", &lnum1, &lnum2, &lnum3, &lnum4); fclose(infile);
	num = lnum4 > jif4 ? (int)((100 * ((lnum1 - jif1) + (lnum2 - jif2) + (lnum3 - jif3))) / (lnum4 - jif4)) : 0;
	jif1 = lnum1; jif2 = lnum2; jif3 = lnum3; jif4 = lnum4;
	sprintf(buf, "%02d%%", num);
	
	dc.w = textw(cr, buf);

	// DRAW CPU
	setfont(cr, "Sans 9");
	pango_layout_set_text(dc.plo, buf, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, y);
	pango_cairo_show_layout(cr, dc.plo);

	// need dc.x and dc.w set in order to draw progress bar
	draw_progress_bar(cr, num, FALSE);

	dc.x += PROGRESS_BAR_W + 10; // adjust dc.x for progress bar + 10px pad

	// DRAW MEMORY ICON
	icon = cairo_image_surface_create_from_png("/usr/share/icons/kawaii-bar/mem.png");
	cairo_rectangle(cr, dc.x, 0, dc.x + 20, 20);
	cairo_set_source_surface(cr, icon, dc.x, 3);
	cairo_fill(cr);
	cairo_surface_destroy(icon);
	dc.x += 20; // adjust for icon width

	// GET MEMORY
	long buffers, cached, free, total;
        infile = fopen("/proc/meminfo", "r");
        fscanf(infile, "MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n",
         &total, &free, &buffers, &cached); fclose(infile);
        num = 100 * (free + buffers + cached) / total;
        sprintf(buf, "%02d%%", num);

	// DRAW MEMORY
	pango_layout_set_text(dc.plo, buf, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, y);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, buf);
	draw_progress_bar(cr, num, FALSE);

	dc.x += PROGRESS_BAR_W + 10; // adjust for progress bar width

	// DRAW BATTERY ICON
	icon = cairo_image_surface_create_from_png("/usr/share/icons/kawaii-bar/bat_full.png");
	cairo_rectangle(cr, dc.x, 0, dc.x + 20, 20);
	cairo_set_source_surface(cr, icon, dc.x, 3);
	cairo_fill(cr);
	cairo_surface_destroy(icon);

	// GET BATTERY
	char state[8];
        long full = -1, now = -1, rate = -1, voltage = -1;
        int perc, hours, minutes, seconds = -1;

        infile = fopen("/sys/class/power_supply/BAT0/energy_now", "r"); fscanf(infile, "%ld\n", &now); fclose(infile);
        infile = fopen("/sys/class/power_supply/BAT0/energy_full", "r"); fscanf(infile, "%ld\n", &full); fclose(infile);
        infile = fopen("/sys/class/power_supply/BAT0/status", "r"); fscanf(infile, "%s\n", state); fclose(infile);
        infile = fopen("/sys/class/power_supply/BAT0/voltage_now", "r"); fscanf(infile, "%ld\n", &voltage); fclose(infile);
        infile = fopen("/sys/class/power_supply/BAT0/power_now", "r"); fscanf(infile, "%ld\n", &rate); fclose(infile);

        now = ((float)voltage * (float)now);
        full = ((float)voltage * (float)full);
        rate = ((float)voltage * (float)rate);
        perc = (now * 100) / full;

        if(perc == 100)
                sprintf(buf, "full");
        else {
                if(strncmp(state, "Charging", 8) == 0)
                        seconds = 3600 * (((float)full - (float)now) / (float)rate);
                else
                        seconds = 3600 * ((float)now / (float)rate);
                hours = seconds / 3600;
                seconds -= 3600 * hours;
                minutes = seconds / 60;
                if(strncmp(state, "Charging", 8) == 0)
                	sprintf(buf, "%d%%, %02d:%02d", perc, hours, minutes);
                else
                        sprintf(buf, "%d%%, %02d:%02d", perc, hours, minutes);
        }
	dc.x += 20; // adjust for icon width

	// DRAW BATTERY STATUS
	pango_layout_set_text(dc.plo, buf, -1);
	cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
	cairo_move_to(cr, dc.x, y);
	pango_cairo_show_layout(cr, dc.plo);

	dc.w = textw(cr, buf);
	draw_progress_bar(cr, perc, FALSE);

	dc.x += PROGRESS_BAR_W + 10;
}

int
main(int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *da;

	/* stats stuff */
	infile = fopen("/proc/stat", "r");
	fscanf(infile, "cpu %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4); fclose(infile);

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	init_window(window);
	da = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), da);

	g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(on_draw), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_timeout_add_seconds(1, (GSourceFunc) on_update, (gpointer) window);

	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}
