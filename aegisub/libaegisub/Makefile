# $Id$
include ../Makefile.inc


LIB_SHARED = libaegisub-3.0.so
LIB_SHARED_INSTALL = yes
LIB_VERSION = 3

CXXFLAGS = -Iinclude -I../src -I.. -DLAGI -fPIC -Wno-variadic-macros

PRECOMPILED_HEADER_NAME = lagi_pre.h
lagi_pre.h.gch: CXXFLAGS := $(CXXFLAGS)

ifeq (yes, $(BUILD_DARWIN))
CXXFLAGS += -I/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Headers/
LDFLAGS += -L../universalchardet -luniversalchardet
endif

common/charset_conv.o: CXXFLAGS += $(CFLAGS_ICONV)
unix/path.o: CXXFLAGS += -DP_DATA=\"$(P_DATA)\" -DP_DOC=\"$(P_DOC)\" -DP_LOCALE=\"$(P_LOCALE)\"

SRC = \
	common/charset.cpp \
	common/charset_conv.cpp \
	common/charset_ucd.cpp \
	common/hotkey.cpp \
	common/json.cpp \
	common/mru.cpp \
	common/option.cpp \
	common/option_visit.cpp \
	common/path.cpp \
	common/progress.cpp \
	common/keyframe.cpp \
	common/util.cpp \
	common/log.cpp \
	common/validator.cpp \
	common/vfr.cpp \
	unix/util.cpp \
	unix/io.cpp \
	unix/access.cpp \
	unix/log.cpp \
	unix/path.cpp

ifeq (yes, $(BUILD_DARWIN))
SRC += \
	osx/util.cpp
endif

HEADERS = \
	*/*.h \
	include/aegisub/*.h \
	include/aegisub/cajun/*.h


include ../Makefile.target
-include */*.d
