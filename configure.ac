AC_INIT([ggk], [1.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CHECK_LIB([pthread], pthread_create, [], [AC_MSG_ERROR([pthread library not found])])
AC_CHECK_LIB([glib-2.0], g_variant_new, [], [AC_MSG_ERROR([glib-2.0 library not found])])
AC_CHECK_LIB([gio-2.0], g_dbus_method_invocation_return_dbus_error, [], [AC_MSG_ERROR([gio-2.0 library not found])])
AC_CHECK_LIB([gobject-2.0], g_object_unref, [], [AC_MSG_ERROR([gobject-2.0 library not found])])
AX_CXX_COMPILE_STDCXX(11)
AC_PROG_RANLIB
AC_PROG_CXX
AM_PROG_AR

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GOBJECT_CFLAGS)

if pkg-config --atleast-version=2.00 glib-2.0; then
   GLIB_CFLAGS=`pkg-config --cflags glib-2.0`
else
   AC_MSG_ERROR(glib-2.0 not found)
fi

if pkg-config --atleast-version=2.00 gio-2.0; then
   GIO_CFLAGS=`pkg-config --cflags gio-2.0`
else
   AC_MSG_ERROR(gio-2.0 not found)
fi

if pkg-config --atleast-version=2.00 gobject-2.0; then
   GOBJECT_CFLAGS=`pkg-config --cflags gobject-2.0`
else
   AC_MSG_ERROR(gobject-2.0 not found)
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
