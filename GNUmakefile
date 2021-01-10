BIN         = urn-gtk
OBJS        = urn.o urn-gtk.o bind.o $(COMPONENTS)
COMPONENTS  = $(addprefix components/, \
              urn-component.o title.o splits.o timer.o \
              prev-segment.o best-sum.o pb.o wr.o)

LIBS        = gtk+-3.0 x11 jansson
CFLAGS      += `pkg-config --cflags $(LIBS)` -Wall -Wno-unused-parameter -std=gnu99
LDLIBS      += `pkg-config --libs $(LIBS)`

BIN_DIR     = /usr/local/bin
APP         = urn.desktop
APP_DIR     = /usr/share/applications
ICON        = urn
ICON_DIR    = /usr/share/icons/hicolor
SCHEMAS_DIR = /usr/share/glib-2.0/schemas

$(BIN): $(OBJS)

$(OBJS): urn-gtk.h

urn-gtk.h: urn-gtk.css
	xxd --include urn-gtk.css > urn-gtk.h || (rm urn-gtk.h; false)

install:
	install -Dm755 $(BIN) $(BIN_DIR)/$(BIN)
	install -Dm644 $(APP) $(APP_DIR)/$(APP)
	for size in 16 22 24 32 36 48 64 72 96 128 256 512; do \
	  mkdir -p $(ICON_DIR)/"$$size"x"$$size"/apps ; \
	  convert $(ICON).svg -resize "$$size"x"$$size" \
	          $(ICON_DIR)/"$$size"x"$$size"/apps/$(ICON).png ; \
	done
	gtk-update-icon-cache -f -t $(ICON_DIR)
	install -Dm644 urn-gtk.gschema.xml $(SCHEMAS_DIR)/urn-gtk.gschema.xml
	glib-compile-schemas $(SCHEMAS_DIR)
	mkdir -p /usr/share/urn/themes
	rsync -a --exclude=".*" themes /usr/share/urn

uninstall:
	rm -f $(BIN_DIR)/$(BIN)
	rm -f $(APP_DIR)/$(APP)
	rm -rf /usr/share/urn
	for size in 16 22 24 32 36 48 64 72 96 128 256 512; do \
	  rm -f $(ICON_DIR)/"$$size"x"$$size"/apps/$(ICON).png ; \
	done

remove-schema:
	rm $(SCHEMAS_DIR)/urn-gtk.gschema.xml
	glib-compile-schemas $(SCHEMAS_DIR)

clean:
	rm -f $(OBJS) $(BIN) urn-gtk.h
