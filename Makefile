EE_BIN = ular-tangga-3d.elf
EE_OBJS = main.o
EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
