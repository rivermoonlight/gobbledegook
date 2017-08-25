# Build a static library (libggk.a)
noinst_LIBRARIES = libggk.a
libggk_a_CXXFLAGS = -fPIC -Wall -Wextra -std=c++11 $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GOBJECT_CFLAGS)
libggk_a_SOURCES = DBusInterface.cpp \
                   DBusInterface.h \
                   DBusMethod.cpp \
                   DBusMethod.h \
                   DBusObject.cpp \
                   DBusObject.h \
                   DBusObjectPath.h \
                   GattCharacteristic.cpp \
                   GattCharacteristic.h \
                   GattDescriptor.cpp \
                   GattDescriptor.h \
                   GattInterface.cpp \
                   GattInterface.h \
                   GattProperty.cpp \
                   GattProperty.h \
                   GattService.cpp \
                   GattService.h \
                   GattUuid.h \
                   Globals.h \
                   Gobbledegook.cpp \
                   ../include/Gobbledegook.h \
                   HciAdapter.cpp \
                   HciAdapter.h \
                   HciSocket.cpp \
                   HciSocket.h \
                   Init.cpp \
                   Init.h \
                   Logger.cpp \
                   Logger.h \
                   Mgmt.cpp \
                   Mgmt.h \
                   Server.cpp \
                   Server.h \
                   ServerUtils.cpp \
                   ServerUtils.h \
                   standalone.cpp \
                   TickEvent.h \
                   Utils.cpp \
                   Utils.h
# Build our standalone server (linking statically with libggk.a, linking dynamically with GLib)
standalone_CXXFLAGS = -fPIC -Wall -Wextra -std=c++11
noinst_PROGRAMS = standalone
standalone_SOURCES = standalone.cpp
standalone_LDADD = libggk.a
standalone_LDLIBS = $(GLIB_LIBS) $(GIO_LIBS) $(GOBJECT_LIBS)
