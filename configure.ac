AC_INIT(camsource, 0.1.0)
AM_INIT_AUTOMAKE(camsource, 0.1.0)
AC_CONFIG_SRCDIR(src/grab.c)
AC_CONFIG_FILES([Makefile src/Makefile])
AM_CONFIG_HEADER(config.h)

AC_LANG(C)

AC_PROG_CC
AC_PROG_LIBTOOL

AC_C_CONST
AC_C_BIGENDIAN

AC_HEADER_STDC
AC_CHECK_HEADER(linux/videodev.h,, [AC_MSG_ERROR([Could not find linux/videodev.h])])
AC_CHECK_HEADER(jpeglib.h,, [AC_MSG_ERROR([Could not find jpeglib.h])])

AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)

# Libs for main prog
AC_CHECK_LIB(dl, dlopen,, [AC_MSG_ERROR([Could not find dynamic link lib])])
AC_CHECK_LIB(pthread, pthread_create,, [AC_MSG_ERROR([Could not find posix thread lib])])

AC_MSG_CHECKING([for libxml])
XMLVER=$(xml2-config --version 2> /dev/null)
if test $? -eq 0; then
	AC_MSG_RESULT([yes, $XMLVER])
	CFLAGS="$CFLAGS $(echo $(xml2-config --cflags))"
	LDFLAGS="$LDFLAGS $(echo $(xml2-config --cflags --libs))"
else
	XMLVER=$(xml-config --version 2> /dev/null)
	if test $? -eq 0; then
		AC_MSG_RESULT([yes, $XMLVER])
		CFLAGS="$CFLAGS $(echo $(xml-config --cflags))"
		LDFLAGS="$LDFLAGS $(echo $(xml-config --cflags --libs))"
	else
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([Could not find libxml])
	fi
fi

# Libs for plugins
AC_CHECK_LIB(jpeg, jpeg_start_compress, :, [AC_MSG_ERROR([Could not find libjpeg])])

CFLAGS="$CFLAGS -Wall -D_REENTRANT -DPREFIX=\"\\\"\${prefix}\\\"\" -DPKGLIBDIR=\"\\\"\${pkglibdir}\\\"\""
LDFLAGS="$LDFLAGS -rdynamic"

AC_OUTPUT

