#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <gtk/gtk.h>
#include "urn.h"
#include "urn-gtk.h"
#include "keybinder.h"
#include "components/urn-component.h"

#define URN_APP_TYPE (urn_app_get_type ())
#define URN_APP(obj)                            \
    (G_TYPE_CHECK_INSTANCE_CAST                 \
     ((obj), URN_APP_TYPE, UrnApp))

typedef struct _UrnApp       UrnApp;
typedef struct _UrnAppClass  UrnAppClass;

#define URN_APP_WINDOW_TYPE (urn_app_window_get_type ())
#define URN_APP_WINDOW(obj)                             \
    (G_TYPE_CHECK_INSTANCE_CAST                         \
     ((obj), URN_APP_WINDOW_TYPE, UrnAppWindow))

typedef struct _UrnAppWindow         UrnAppWindow;
typedef struct _UrnAppWindowClass    UrnAppWindowClass;

#define WINDOW_PAD (8)

typedef struct {
    guint key;
    GdkModifierType mods;
} Keybind;

struct _UrnAppWindow {
    GtkApplicationWindow parent;
    char data_path[256];
    int decorated;
    urn_game *game;
    urn_timer *timer;
    GdkDisplay *display;
    GtkWidget *box;
    GList *components;
    GtkWidget *footer;
    GtkCssProvider *style;
    gboolean hide_cursor;
    gboolean global_hotkeys;
    Keybind keybind_start_split;
    Keybind keybind_stop_reset;
    Keybind keybind_cancel;
    Keybind keybind_unsplit;
    Keybind keybind_skip_split;
    Keybind keybind_toggle_decorations;
};

struct _UrnAppWindowClass {
    GtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(UrnAppWindow, urn_app_window, GTK_TYPE_APPLICATION_WINDOW);

static Keybind parse_keybind(const gchar *accelerator) {
    Keybind kb;
    gtk_accelerator_parse(accelerator, &kb.key, &kb.mods);
    return kb;
}

static int keybind_match(Keybind kb, GdkEventKey key) {
    return key.keyval == kb.key &&
        kb.mods == (key.state & gtk_accelerator_get_default_mod_mask());
}

static void urn_app_window_destroy(GtkWidget *widget, gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    if (win->timer) {
        urn_timer_release(win->timer);
    }
    if (win->game) {
        urn_game_release(win->game);
    }
}

static gpointer save_game_thread(gpointer data) {
    urn_game *game = data;
    urn_game_save(game);
    return NULL;
}

static void save_game(urn_game *game) {
    g_thread_new("save_game", save_game_thread, game);
}

static void urn_app_window_clear_game(UrnAppWindow *win) {
    GdkScreen *screen;
    GList *l;

    gtk_widget_hide(win->box);

    for (l = win->components; l != NULL; l = l->next) {
        UrnComponent *component = l->data;
        if (component->ops->clear_game)
            component->ops->clear_game(component);
    }

    // remove game's style
    if (win->style) {
        screen = gdk_display_get_default_screen(win->display);
        gtk_style_context_remove_provider_for_screen(
                screen, GTK_STYLE_PROVIDER(win->style));
        g_object_unref(win->style);
        win->style = NULL;
    }
}

static gboolean urn_app_window_step(gpointer data) {
    UrnAppWindow *win = data;
    long long now = urn_time_now();
    static int set_cursor;
    if (win->hide_cursor && !set_cursor) {
        GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(win));
        if (gdk_window) {
            GdkCursor *cursor = gdk_cursor_new_for_display(win->display, GDK_BLANK_CURSOR);
            gdk_window_set_cursor(gdk_window, cursor);
            set_cursor = 1;
        }
    }
    if (win->timer) {
        urn_timer_step(win->timer, now);
    }
    return TRUE;
}

static int urn_app_window_find_theme(UrnAppWindow *win,
                                     const char *theme_name,
                                     const char *theme_variant,
                                     char *str) {
    char theme_path[256];
    struct stat st = {0};
    if (!theme_name || !strlen(theme_name)) {
        str[0] = '\0';
        return 0;
    }
    strcpy(theme_path, "/");
    strcat(theme_path, theme_name);
    strcat(theme_path, "/");
    strcat(theme_path, theme_name);
    if (theme_variant && strlen(theme_variant)) {
        strcat(theme_path, "-");
        strcat(theme_path, theme_variant);
    }
    strcat(theme_path, ".css");

    strcpy(str, win->data_path);
    strcat(str, "/themes");
    strcat(str, theme_path);
    if (stat(str, &st) == -1) {
        strcpy(str, "/usr/share/urn/themes");
        strcat(str, theme_path);
        if (stat(str, &st) == -1) {
            str[0] = '\0';
            return 0;
        }
    }
    return 1;
}

static void urn_app_window_show_game(UrnAppWindow *win) {
    GdkScreen *screen;
    char str[256];
    GList *l;

    // set dimensions
    if (win->game->width > 0 && win->game->height > 0) {
        gtk_widget_set_size_request(GTK_WIDGET(win),
                                    win->game->width,
                                    win->game->height);
    }

    // set game theme
    if (urn_app_window_find_theme(win,
                                  win->game->theme,
                                  win->game->theme_variant,
                                  str)) {
        win->style = gtk_css_provider_new();
        screen = gdk_display_get_default_screen(win->display);
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(win->style),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_css_provider_load_from_path(
            GTK_CSS_PROVIDER(win->style),
            str, NULL);
    }

    for (l = win->components; l != NULL; l = l->next) {
        UrnComponent *component = l->data;
        if (component->ops->show_game)
            component->ops->show_game(component, win->game, win->timer);
    }

    gtk_widget_show(win->box);
}

static void resize_window(UrnAppWindow *win,
                          int window_width,
                          int window_height) {
    GList *l;
    for (l = win->components; l != NULL; l = l->next) {
        UrnComponent *component = l->data;
        if (component->ops->resize) {
            component->ops->resize(component,
                    window_width - 2 * WINDOW_PAD,
                    window_height);
        }
    }
}

static gboolean urn_app_window_resize(GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    resize_window(win, event->configure.width, event->configure.height);
    return FALSE;
}

static void timer_start_split(UrnAppWindow *win) {
    if (win->timer) {
        GList *l;
        if (!win->timer->running) {
            if (urn_timer_start(win->timer)) {
                save_game(win->game);
            }
        } else {
            urn_timer_split(win->timer);
        }
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->start_split)
                component->ops->start_split(component, win->timer);
        }
    }
}

static void timer_stop_reset(UrnAppWindow *win) {
    if (win->timer) {
        GList *l;
        if (win->timer->running) {
            urn_timer_stop(win->timer);
        } else {
            if (urn_timer_reset(win->timer)) {
                urn_app_window_clear_game(win);
                urn_app_window_show_game(win);
                save_game(win->game);
            }
        }
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->stop_reset)
                component->ops->stop_reset(component, win->timer);
        }
    }
}

static void timer_cancel_run(UrnAppWindow *win) {
    if (win->timer) {
        GList *l;
        if (urn_timer_cancel(win->timer)) {
            urn_app_window_clear_game(win);
            urn_app_window_show_game(win);
            save_game(win->game);
        }
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->cancel_run)
                component->ops->cancel_run(component, win->timer);
        }
    }
}


static void timer_skip(UrnAppWindow *win) {
    if (win->timer) {
        GList *l;
        urn_timer_skip(win->timer);
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->skip)
                component->ops->skip(component, win->timer);
        }
    }
} 

static void timer_unsplit(UrnAppWindow *win) {
    if (win->timer) {
        GList *l;
        urn_timer_unsplit(win->timer);
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->unsplit)
                component->ops->unsplit(component, win->timer);
        }
    }
}

static void toggle_decorations(UrnAppWindow *win) {
    gtk_window_set_decorated(GTK_WINDOW(win), !win->decorated);
    win->decorated = !win->decorated;
}

static void keybind_start_split(GtkWidget *widget, UrnAppWindow *win) {
    timer_start_split(win);
}

static void keybind_stop_reset(const char *str, UrnAppWindow *win) {
    timer_stop_reset(win);
}

static void keybind_cancel(const char *str, UrnAppWindow *win) {
    timer_cancel_run(win);
}

static void keybind_skip(const char *str, UrnAppWindow *win) {
    timer_skip(win);
}

static void keybind_unsplit(const char *str, UrnAppWindow *win) {
    timer_unsplit(win);
}

static void keybind_toggle_decorations(const char *str, UrnAppWindow *win) {
    toggle_decorations(win);
}

static gboolean urn_app_window_keypress(GtkWidget *widget,
                                        GdkEvent *event,
                                        gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)data;
    if (keybind_match(win->keybind_start_split, event->key))
        timer_start_split(win);
    else if (keybind_match(win->keybind_stop_reset, event->key))
        timer_stop_reset(win);
    else if (keybind_match(win->keybind_cancel, event->key))
        timer_cancel_run(win);
    else if (keybind_match(win->keybind_unsplit, event->key))
        timer_unsplit(win);
    else if (keybind_match(win->keybind_skip_split, event->key))
        timer_skip(win);
    else if (keybind_match(win->keybind_toggle_decorations, event->key))
        toggle_decorations(win);
    return TRUE;
}

static gboolean urn_app_window_draw(gpointer data) {
    UrnAppWindow *win = data;
    if (win->timer) {
        GList *l;
        for (l = win->components; l != NULL; l = l->next) {
            UrnComponent *component = l->data;
            if (component->ops->draw)
                component->ops->draw(component, win->game, win->timer);
        }
    } else {
        GdkRectangle rect;
        gtk_widget_get_allocation(GTK_WIDGET(win), &rect);
        gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(win)),
                                   &rect, FALSE);
    }
    return TRUE;
}

static void urn_app_window_init(UrnAppWindow *win) {
    GtkCssProvider *provider;
    GdkScreen *screen;
    struct passwd *pw;
    char str[256];
    const char *theme;
    const char *theme_variant;
    int i;

    win->display = gdk_display_get_default();
    win->style = NULL;

    // make data path
    pw = getpwuid(getuid());
    strcpy(win->data_path, pw->pw_dir);
    strcat(win->data_path, "/.urn");

    // load settings
    GSettings *settings = g_settings_new("wildmouse.urn");
    win->hide_cursor = g_settings_get_boolean(settings, "hide-cursor");
    win->global_hotkeys = g_settings_get_boolean(settings, "global-hotkeys");
    win->keybind_start_split = parse_keybind(
	g_settings_get_string(settings, "keybind-start-split"));
    win->keybind_stop_reset = parse_keybind(
	g_settings_get_string(settings, "keybind-stop-reset"));
    win->keybind_cancel = parse_keybind(
	g_settings_get_string(settings, "keybind-cancel"));
    win->keybind_unsplit = parse_keybind(
	g_settings_get_string(settings, "keybind-unsplit"));
    win->keybind_skip_split = parse_keybind(
	g_settings_get_string(settings, "keybind-skip-split"));
    win->keybind_toggle_decorations = parse_keybind(
	g_settings_get_string(settings, "keybind-toggle-decorations"));
    win->decorated = g_settings_get_boolean(settings, "start-decorated");
    gtk_window_set_decorated(GTK_WINDOW(win), win->decorated);

    // Load CSS defaults
    provider = gtk_css_provider_new();
    screen = gdk_display_get_default_screen(win->display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data(
        GTK_CSS_PROVIDER(provider),
        (char *)urn_gtk_css, urn_gtk_css_len, NULL);
    g_object_unref(provider);

    // Load theme
    theme = g_settings_get_string(settings, "theme");
    theme_variant = g_settings_get_string(settings, "theme-variant");
    if (urn_app_window_find_theme(win, theme, theme_variant, str)) {
        provider = gtk_css_provider_new();
        screen = gdk_display_get_default_screen(win->display);
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_css_provider_load_from_path(
            GTK_CSS_PROVIDER(provider),
            str, NULL);
        g_object_unref(provider);
    }

    // Load window junk
    add_class(GTK_WIDGET(win), "window");
    win->game = 0;
    win->timer = 0;

    g_signal_connect(win, "destroy",
                     G_CALLBACK(urn_app_window_destroy), NULL);
    g_signal_connect(win, "configure-event",
                     G_CALLBACK(urn_app_window_resize), win);

    if (win->global_hotkeys) {
        keybinder_init();
        keybinder_bind(
            g_settings_get_string(settings, "keybind-start-split"),
            (KeybinderHandler)keybind_start_split,
            win);
        keybinder_bind(
            g_settings_get_string(settings, "keybind-stop-reset"),
            (KeybinderHandler)keybind_stop_reset,
            win);
        keybinder_bind(
            g_settings_get_string(settings, "keybind-cancel"),
            (KeybinderHandler)keybind_cancel,
            win);
        keybinder_bind(
            g_settings_get_string(settings, "keybind-unsplit"),
            (KeybinderHandler)keybind_unsplit,
            win);
        keybinder_bind(
            g_settings_get_string(settings, "keybind-skip-split"),
            (KeybinderHandler)keybind_skip,
            win);
        keybinder_bind(
            g_settings_get_string(settings, "keybind-toggle-decorations"),
            (KeybinderHandler)keybind_toggle_decorations,
            win);
    } else {
        g_signal_connect(win, "key_press_event",
                         G_CALLBACK(urn_app_window_keypress), win);
    }

    win->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(win->box, WINDOW_PAD);
    gtk_widget_set_margin_bottom(win->box, WINDOW_PAD);
    gtk_widget_set_vexpand(win->box, TRUE);
    gtk_container_add(GTK_CONTAINER(win), win->box);

    // Create all available components (TODO: change this in the future)
    win->components = NULL;
    for (i = 0; urn_components[i].name != NULL; i++) {
        UrnComponent *component = urn_components[i].new();
        if (component) {
            GtkWidget *widget = component->ops->widget(component);
            if (widget) {
                gtk_widget_set_margin_start(widget, WINDOW_PAD);
                gtk_widget_set_margin_end(widget, WINDOW_PAD);
                gtk_container_add(GTK_CONTAINER(win->box),
                        component->ops->widget(component));
            }
            win->components = g_list_append(win->components, component);
        }
    }

    win->footer = gtk_grid_new();
    add_class(win->footer, "footer");
    gtk_widget_set_margin_start(win->footer, WINDOW_PAD);
    gtk_widget_set_margin_end(win->footer, WINDOW_PAD);
    gtk_container_add(GTK_CONTAINER(win->box), win->footer);
    gtk_widget_show(win->footer);

    g_timeout_add(1, urn_app_window_step, win);
    g_timeout_add((int)(1000 / 30.), urn_app_window_draw, win); 
}

static void urn_app_window_class_init(UrnAppWindowClass *class) {
}

static UrnAppWindow *urn_app_window_new(UrnApp *app) {
    UrnAppWindow *win;
    win = g_object_new(URN_APP_WINDOW_TYPE, "application", app, NULL);
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DIALOG);
    return win;
}

static void urn_app_window_open(UrnAppWindow *win, const char *file) {
    if (win->timer) {
        urn_app_window_clear_game(win);
        urn_timer_release(win->timer);
        win->timer = 0;
    }
    if (win->game) {
        urn_game_release(win->game);
        win->game = 0;
    }
    if (urn_game_create(&win->game, file)) {
        win->game = 0;
    } else if (urn_timer_create(&win->timer, win->game)) {
        win->timer = 0;
    } else {
        urn_app_window_show_game(win);
    }
}

struct _UrnApp {
    GtkApplication parent;
};

struct _UrnAppClass {
    GtkApplicationClass parent_class;
};

G_DEFINE_TYPE(UrnApp, urn_app, GTK_TYPE_APPLICATION);

static void open_activated(GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       app) {
    gchar *splits_folder;
    GList *windows;
    UrnAppWindow *win;
    GtkWidget *dialog;
    gint res;

    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    dialog = gtk_file_chooser_dialog_new (
        "Open File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    
    GSettings *settings = g_settings_new("wildmouse.urn");
    splits_folder = g_settings_get_string(settings, "splits-folder");
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        splits_folder);
    g_free(splits_folder);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        gchar *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gchar *folder;

	//open dialog in same folder next time
	folder = gtk_file_chooser_get_current_folder(chooser);
	gint res2 = g_settings_set_string(settings, "splits-folder", folder);
	g_free(folder);
	if (res2) {
	  filename = gtk_file_chooser_get_filename(chooser);
	  urn_app_window_open(win, filename);
	  g_free(filename);
	}
    }
    gtk_widget_destroy(dialog);
}

static void save_activated(GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       app) {
    GList *windows;
    UrnAppWindow *win;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    if (win->game && win->timer) {
        int width, height;
        gtk_window_get_size(GTK_WINDOW(win), &width, &height);
        win->game->width = width;
        win->game->height = height;
        urn_game_update_splits(win->game, win->timer);
        save_game(win->game);
    }
}

static void save_bests_activated(GSimpleAction *action,
                                 GVariant      *parameter,
                                 gpointer       app) {
    GList *windows;
    UrnAppWindow *win;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    if (win->game && win->timer) {
        urn_game_update_bests(win->game, win->timer);
        save_game(win->game);
    }
}

static void reload_activated(GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       app) {
    GList *windows;
    UrnAppWindow *win;
    char *path;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    if (win->game) {
        path = strdup(win->game->path);
        urn_app_window_open(win, path);
        free(path);
    }
}

static void close_activated(GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       app) {
    GList *windows;
    UrnAppWindow *win;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    if (win->game && win->timer) {
        urn_app_window_clear_game(win);
    }
    if (win->timer) {
        urn_timer_release(win->timer);
        win->timer = 0;
    }
    if (win->game) {
        urn_game_release(win->game);
        win->game = 0;
    }
    gtk_widget_set_size_request(GTK_WIDGET(win), -1, -1);
}

static void quit_activated(GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       app) {
    g_application_quit(G_APPLICATION(app));
}

static GActionEntry app_entries[] = {
    { "open", open_activated, NULL, NULL, NULL },
    { "save", save_activated, NULL, NULL, NULL },
    { "save_bests", save_bests_activated, NULL, NULL, NULL },
    { "reload", reload_activated, NULL, NULL, NULL },
    { "close", close_activated, NULL, NULL, NULL },
    { "quit", quit_activated, NULL, NULL, NULL }
};

static void urn_app_activate(GApplication *app) {
    UrnAppWindow *win;
    win = urn_app_window_new(URN_APP(app));
    gtk_window_present(GTK_WINDOW(win));
    open_activated(NULL, NULL, app);
}

static void urn_app_startup(GApplication *app) {
    GtkBuilder *builder;
    GMenuModel *menubar;
    G_APPLICATION_CLASS(urn_app_parent_class)->startup(app);
    builder = gtk_builder_new_from_string (
        "<interface>"
        "  <menu id='menubar'>"
        "    <submenu>"
        "      <attribute name='label'>File</attribute>"
        "      <item>"
        "        <attribute name='label'>Open</attribute>"
        "        <attribute name='action'>app.open</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label'>Save</attribute>"
        "        <attribute name='action'>app.save</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label'>Save bests</attribute>"
        "        <attribute name='action'>app.save_bests</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label'>Reload</attribute>"
        "        <attribute name='action'>app.reload</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label'>Close</attribute>"
        "        <attribute name='action'>app.close</attribute>"
        "      </item>"
        "      <item>"
        "        <attribute name='label'>Quit</attribute>"
        "        <attribute name='action'>app.quit</attribute>"
        "      </item>"
        "    </submenu>"
        "  </menu>"
        "</interface>",
        -1);
    g_action_map_add_action_entries(G_ACTION_MAP(app),
                                    app_entries, G_N_ELEMENTS(app_entries),
                                    app);
    menubar = G_MENU_MODEL(gtk_builder_get_object(builder, "menubar"));
    gtk_application_set_menubar(GTK_APPLICATION(app), menubar);
    g_object_unref(builder);
}

static void urn_app_init(UrnApp *app) {
}

static void urn_app_open(GApplication  *app,
                         GFile        **files,
                         gint           n_files,
                         const gchar   *hint) {
    GList *windows;
    UrnAppWindow *win;
    int i;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows) {
        win = URN_APP_WINDOW(windows->data);
    } else {
        win = urn_app_window_new(URN_APP(app));
    }
    for (i = 0; i < n_files; i++) {
        urn_app_window_open(win, g_file_get_path(files[i]));
    }
    gtk_window_present(GTK_WINDOW(win));
}

UrnApp *urn_app_new(void) {
    g_set_application_name("urn");
    return g_object_new(URN_APP_TYPE,
                        "application-id", "wildmouse.urn",
                        "flags", G_APPLICATION_HANDLES_OPEN,
                        NULL);
}

static void urn_app_class_init(UrnAppClass *class) {
    G_APPLICATION_CLASS(class)->activate = urn_app_activate;
    G_APPLICATION_CLASS(class)->open = urn_app_open;
    G_APPLICATION_CLASS(class)->startup = urn_app_startup;
}

int main(int argc, char *argv[]) {
    return g_application_run(G_APPLICATION(urn_app_new()), argc, argv);
}
