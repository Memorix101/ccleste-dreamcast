#
# Memorix101
# Copyright (C) 2024
#   

TARGET = celeste.elf
OBJS = FAKE_mixer.o mm.o celeste.o sdl20compat.inc.o sdl12main.o romdisk.o
#OBJS = mixer_ogg.o celeste.o sdl20compat.inc.o sdl12main.o romdisk.o
KOS_ROMDISK_DIR = romdisk

KOS_CFLAGS += -I../lmikmod/include -I/opt/toolchains/dc/kos/addons/include -I../supertux-dc-SDL_dreamhal-Fakemixer/ports/mikmod/lmikmod/include -I$(KOS_BASE)/kernel/arch/dreamcast/include/dc -DDREAMCAST -DCELESTE_P8_FIXEDP -DCELESTE_P8_ENABLE_AUDIO=1

KOS_CPPFLAGS += -fexceptions -std=c++11 -fpermissive

# Update library paths and libraries for SDL2
KOS_LIBS+= -L./tools -L/opt/toolchains/dc/kos/addons/lib/dreamcast

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) celeste-dc.cdi romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) $(OBJEXTRA) -lADX -lm -lmikmod -lSDL2 -lSDL2main -lGL -lkosutils $(KOS_LIBS) -fno-exceptions -fno-rtti
	./make_cdi.sh

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

