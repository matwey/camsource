bin_PROGRAMS = camsource
lib_LTLIBRARIES = libwc_serv.la libjpeg_comp.la

camsource_SOURCES = rwlock.c main.c grab.c camdev.c unpalette.c configfile.c mod_handle.c

libwc_serv_la_SOURCES = wc_serv.c

libjpeg_comp_la_SOURCES = jpeg.c
libjpeg_comp_la_LDFLAGS = -ljpeg
