OBJS    := urn.o urn-gtk.o
BIN     := urn
CFLAGS  := `pkg-config --cflags gtk+-3.0`
LDLIBS  := -ljansson `pkg-config --libs gtk+-3.0`

$(BIN): $(OBJS)

install:
	cp $(BIN) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
