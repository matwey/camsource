bin_PROGRAMS = camsource
pkglib_LTLIBRARIES = libwc_serv.la libjpeg_comp.la libflip.la \
	libresize.la libsocket.la libhttp.la libfilewrite.la \
	libftpup.la libtext.la librgbbgr.la librotate.la \
	libbw.la libvloopback.la libmotiondetect.la libregrab.la

pkginclude_HEADERS = camdev.h configfile.h filter.h grab.h \
	image.h jpeg.h mod_handle.h module.h unpalette.h xmlhelp.h \
	socket.h log.h

camsource_SOURCES = main.c grab.c camdev.c unpalette.c configfile.c \
	mod_handle.c image.c filter.c xmlhelp.c log.c main.h

libwc_serv_la_SOURCES = wc_serv.c wc_serv.h

libjpeg_comp_la_SOURCES = jpeg.c
libjpeg_comp_la_LDFLAGS = -ljpeg

libflip_la_SOURCES = flip.c
libresize_la_SOURCES = resize.c
libsocket_la_SOURCES = socket.c
libhttp_la_SOURCES = http.c http.h
libfilewrite_la_SOURCES = filewrite.c filewrite.h
libftpup_la_SOURCES = ftpup.c ftpup.h
libtext_la_SOURCES = text.c text.h font_6x11.h
librgbbgr_la_SOURCES = rgbbgr.c
librotate_la_SOURCES = rotate.c
libbw_la_SOURCES = bw.c
libvloopback_la_SOURCES = vloopback.c
libmotiondetect_la_SOURCES = motiondetect.c
libregrab_la_SOURCES = regrab.c

# $Id$

