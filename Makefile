ifeq ($(OS),Windows_NT)
OBJECTS += gerror.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/windows/*.c))
endif
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/posix/*.c))

CPPFLAGS += -Iinclude -I. -I../
CFLAGS += -fPIC

LDFLAGS += -L../gimxlog
LDLIBS += -lgimxlog

ifeq ($(OS),Windows_NT)
LDLIBS += -lws2_32
endif

include Makedefs

ifeq ($(OS),Windows_NT)
gerror.o: ../gimxcommon/src/windows/gerror.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<
endif
