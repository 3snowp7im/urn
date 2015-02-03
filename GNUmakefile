BIN      := urn-gtk
OBJS     := urn.o urn-gtk.o
CFLAGS   := `pkg-config --cflags gtk+-3.0`
LDLIBS   := -ljansson `pkg-config --libs gtk+-3.0`
BIN_DIR  := /usr/local/bin
APP      := urn.desktop
APP_DIR  := /usr/share/applications
ICON     := urn
ICON_DIR := /usr/share/icons/hicolor

$(BIN): $(OBJS)

install:
	cp $(BIN) $(BIN_DIR)
	cp $(APP) $(APP_DIR)
	for size in 16 22 24 32 36 48 64 72 96 128 256 512; do \
	  convert $(ICON).svg -resize "$$size"x"$$size" \
	          $(ICON_DIR)/"$$size"x"$$size"/apps/$(ICON).png ; \
	done
	gtk-update-icon-cache -f -t $(ICON_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(BIN)
	rm -f $(APP_DIR)/$(APP)
	for size in 16 22 24 32 36 48 64 72 96 128 256 512; do \
	  rm -f $(ICON_DIR)/"$$size"x"$$size"/apps/$(ICON).png ; \
	done

clean:
	rm -f $(OBJS) $(BIN)
