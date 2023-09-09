ifndef NAVISERVER
    NAVISERVER  = /usr/local/ns
endif

#
# Module name
#
MOD      =  ksuid.so

#
# Objects to build.
#
MODOBJS     = src/library.o src/base62.o src/hex.o

MODLIBS  +=

CFLAGS += -DUSE_NAVISERVER
CXXFLAGS += $(CFLAGS)

include  $(NAVISERVER)/include/Makefile.module