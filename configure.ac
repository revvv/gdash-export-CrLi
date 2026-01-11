dnl Process this file with autoconf to produce a configure script.

AC_INIT(GDash, export-1.10.0, [https://github.com/revvv/gdash-export-CrLi])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)
AM_SILENT_RULES([yes])

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

AC_CHECK_FUNCS([setlocale])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_PROG_INSTALL

AC_CHECK_HEADERS([stdint.h stdlib.h])
AC_CHECK_FUNCS([strchr strrchr])
AC_FUNC_MEMCMP
AC_FUNC_MALLOC
AC_TYPE_UINT32_T

PKG_CHECK_MODULES([GLIB], [glib-2.0])


dnl
dnl GTK+ or SDL or both?
dnl
AC_ARG_WITH(gtk,
  [AS_HELP_STRING([--without-gtk], [disables using gtk+])],
  [with_gtk=$withval],
  [with_gtk=yes])
AC_ARG_WITH(sdl,
  [AS_HELP_STRING([--without-sdl], [disables using sdl])],
  [with_sdl=$withval],
  [with_sdl=yes])

if test x"$with_gtk" = "xno" -a x"$with_sdl" = "xno"; then
  AC_MSG_ERROR(You cannot disable both SDL and GTK. Rerun with --with-gtk or --with-sdl.)
fi

dnl this conditional must ALWAYS be run, not inside the if below.
AM_CONDITIONAL(GTK, test x"$with_gtk" = "xyes")
if test x"$with_gtk" = "xyes" ; then
  AC_DEFINE(HAVE_GTK,,[Use GTK.])
  PKG_CHECK_MODULES([GTK], [gtk+-3.0])
else
  GTK_CFLAGS=""
  GTK_LIBS=""
fi

AM_CONDITIONAL(SDL, test x"$with_sdl" = "xyes")
if test x"$with_sdl" = "xyes" ; then
  AC_DEFINE(HAVE_SDL,,[Use SDL.])
  AM_PATH_SDL2(2.0.0, :, AC_MSG_ERROR(Test for SDL failed. See the file 'INSTALL' for help.))
  SDL_LIBS="$SDL_LIBS -lSDL2_mixer -lSDL2_image"
  AX_CHECK_GL
else
  GL_CFLAGS=""
  GL_LIBS=""
  SDL_CFLAGS=""
  SDL_LIBS=""
fi


PKG_CHECK_MODULES(LIBPNG, [libpng], AC_DEFINE(HAVE_LIBPNG, 1, Define if you have libpng), [])


AC_CONFIG_FILES([
Makefile
po/Makefile.in
include/Makefile
src/Makefile
caves/Makefile
docs/Makefile
sound/Makefile
music/Makefile
shaders/Makefile
])

AC_OUTPUT
