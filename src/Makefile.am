bin_PROGRAMS = camsource
pkglib_LTLIBRARIES = libwc_serv.la libjpeg_comp.la libflip.la \
	libresize.la libsocket.la

pkginclude_HEADERS = camdev.h configfile.h filter.h flip.h grab.h \
	image.h jpeg.h main.h mod_handle.h module.h resize.h \
	unpalette.h wc_serv.h xmlhelp.h socket.h

camsource_SOURCES = main.c grab.c camdev.c unpalette.c configfile.c \
	mod_handle.c image.c filter.c xmlhelp.c

libwc_serv_la_SOURCES = wc_serv.c

libjpeg_comp_la_SOURCES = jpeg.c
libjpeg_comp_la_LDFLAGS = -ljpeg

libflip_la_SOURCES = flip.c

libresize_la_SOURCES = resize.c

libsocket_la_SOURCES = socket.c

