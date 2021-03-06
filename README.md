kawaii-bar
===
**This is an alpha release. Some things may not work perfectly or even at all for you.**

kawaii-bar is a cute and functional bar designed for use with bspwm. The goal of this project is for a semi-lightweight and elegant bar written entirely in C. This is achieved through the use of GTK3 combined with the Cairo library, which provides modern, eye-appealing graphics and exceptional font rendering using the Pango backend.

**Note:**
As stated above, this is an alpha release. Most of the code isn't very clean (yet!) and I am still working on various features. All of the code dealing with the visual presentation (colors, fonts, icons, etc...) is currently hardcoded so it may not display correctly on your system. If you are interested in upcoming features or the status of the project, feel free to read the ToDo section and follow (or even contribute to) this project :).

**Dependencies:**

* gtk3 
* cairo
* pango
* alsa-lib

**Installation:**

Before installing, you must first setup a FIFO for bspwm to write to by adding the following lines to your ~/.xinitrc:

    export PANEL_FIFO="/tmp/panel-fifo"
    [ -e "$PANEL_FIFO" ] && rm "$PANEL_FIFO"
    mkfifo "$PANEL_FIFO"
    exec bspwm

You must also make bspwm write to the FIFO by adding another line to your bspwm config file:

    bspc control --subscribe > $PANEL_FIFO &

As of now, I am only including a Makefile until the project is more complete. I will include an Arch Linux PKGBUILD in future builds, but for now you can simply run:

	$ make
	# make install

* In order to allocate space for the bar to be displayed, simply add `bspc config top_padding xx` to your bspwm config file (~/.config/bspwm/bspwmrc), where "xx" equals the bar height - border width in pixels. If you prefer the bar to be at the bottom of the screen, change the TOPBAR global to FALSE in kawaii-bar.h and use "bottom\_padding" instead of "top\_padding" in your bspwmrc.

* **Important:** The Arch Linux icon needs to be manually copied  to "/usr/share/icons/kawaii-bar/". The path is hardcoded right now but will be able to be changed in the near future.

* **Note on fonts:** Fonts currently used are: Inconsolata, Agency FB Bold (clock), and Stlarch (status glyphs). <a href=https://www.archlinux.org/packages/community/any/ttf-inconsolata/>Inconsolata</a> is available in the official Arch repositories while <a href=https://aur.archlinux.org/packages/stlarch_font/>Stlarch</a> is available in the AUR. To install the "Agency FB Bold" font, copy it from the "fonts" directory to "~/.fonts" and update your font cache with `fc-cache -fv ~/.fonts`

**Todo:**

* Major cleanup of existing code.
* Rework code to allow for customization.
* ~~Move globals and user-defined variables to a header file.~~
* Remove stats monitoring -> external program?
* ~~Read tiling state from FIFO -- Ltiled or Lmonocle indicator.~~
* Add struct for desktop status and mode.
* Implement a simple method for user-defined themes (colors, fonts, etc...) -> CSS?
* Add some basic mouse support?
* much, much more...

**Bugs:**

* Desktops status is not shown correctly if updated more than once per second.
* ~~Desktops status does not update automatically (without external event).~~
