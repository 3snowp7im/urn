#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "urn.h"

// get rid of some annoying deprecation warnings
// on the computers i compile this on
#if GTK_CHECK_VERSION(3, 12, 0)
#  define gtk_widget_set_margin_left            \
    gtk_widget_set_margin_start
#  define gtk_widget_set_margin_right           \
    gtk_widget_set_margin_end
#endif

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

static const char *urn_app_window_style =
    ".window {\n"
    "  background-color: #000;\n"
    "  color: #FFF;\n"
    "}\n"

    ".timer {\n"
    "  font-size: 32pt;\n"
    "  text-shadow: 3px 3px #666;\n"
    "}\n"

    ".timer.delay {\n"
    "  color: #999;\n"
    "}\n"

    ".split-time {\n"
    "  color: #FFF;\n"
    "}\n"

    ".split-time.done {\n"
    "  color: #999;\n"
    "}\n"

    ".timer, .split-delta {\n"
    "  color: #0F0;\n"
    "}\n"

    ".losing {\n"
    "  color: #9F9;\n"
    "}\n"

    ".behind {\n"
    "  color: #F99;\n"
    "}\n"

    ".behind.losing {\n"
    "  color: #F00;\n"
    "}\n"

    ".split-delta.best-segment {\n"
    "  color: #F90;\n"
    "}\n"

    ".window .split-delta.best-split {\n"
    "  color: #99F;\n"
    "}\n"

    ".current-segment {\n"
    "  background-color: #336;\n"
    "}\n"
    ;

struct _UrnAppWindow {
    GtkApplicationWindow parent;
    urn_game *game;
    urn_timer *timer;
    int split_count;
    GtkWidget *box;
    GtkWidget *title;
    GtkWidget *split_box;
    GtkWidget **splits;
    GtkWidget **split_titles;
    GtkWidget **split_deltas;
    GtkWidget **split_times;
    GtkWidget *trailer;
    GtkWidget *sum_of_bests;
    GtkWidget *previous_segment_label;
    GtkWidget *previous_segment;
    GtkWidget *personal_best;
    GtkWidget *world_record_label;
    GtkWidget *world_record;
    GtkWidget *time;
};

struct _UrnAppWindowClass {
    GtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(UrnAppWindow, urn_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void urn_app_window_destroy(GtkWidget *widget, gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    if (win->timer) {
        urn_timer_release(win->timer);
    }
    if (win->game) {
        urn_game_release(win->game);
    }
}

#define PREVIOUS_SEGMENT "Prev segment"
#define LIVE_SEGMENT "Live segment"

static void urn_app_window_clear_game(UrnAppWindow *win) {
    int i;
    gtk_widget_hide(win->box);
    if (win->game->world_record) {
        gtk_container_remove(GTK_CONTAINER(win->trailer),
                             win->world_record_label);
        gtk_container_remove(GTK_CONTAINER(win->trailer),
                             win->world_record);
        win->world_record_label = 0;
        win->world_record = 0;
    }
    for (i = 0; i < win->split_count; ++i) {
        gtk_container_remove(GTK_CONTAINER(win->split_box),
                             win->splits[i]);
    }
    free(win->splits);
    free(win->split_titles);
    free(win->split_deltas);
    free(win->split_times);
    win->split_count = 0;
    gtk_label_set_text(GTK_LABEL(win->time), "");
    gtk_label_set_text(GTK_LABEL(win->previous_segment_label),
                       PREVIOUS_SEGMENT);
    gtk_label_set_text(GTK_LABEL(win->previous_segment), "");
    gtk_label_set_text(GTK_LABEL(win->sum_of_bests), "");
    gtk_label_set_text(GTK_LABEL(win->personal_best), "");
}

static void urn_app_window_start(UrnAppWindow *win) {
    if (win->split_count) {
        // highlight first split
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->splits[0]),
            "current-segment");
    }
}

static void urn_app_window_split(UrnAppWindow *win) {
    int prev = win->timer->curr_split - 1;
    char str[256];
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->splits[prev]),
        "current-segment");
    if (win->timer->curr_split < win->game->split_count) {
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->splits[win->timer->curr_split]),
            "current-segment");
    }
    gtk_style_context_add_class(
        gtk_widget_get_style_context(win->split_times[prev]),
        "done");
    urn_split_string(str, win->timer->split_times[prev]);
    gtk_label_set_text(GTK_LABEL(win->split_times[prev]), str);
    if (win->timer->split_info[prev] & URN_INFO_BEST_SPLIT) {
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->split_deltas[prev]),
            "best-split");
    } else if (win->timer->split_info[prev]
               & URN_INFO_BEST_SEGMENT) {
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->split_deltas[prev]),
            "best-segment");
    } else if (win->timer->split_info[prev]
               & URN_INFO_BEHIND_TIME) {
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->split_deltas[prev]),
            "behind");
        if (win->timer->split_info[prev]
            & URN_INFO_LOSING_TIME) {
            gtk_style_context_add_class(
                gtk_widget_get_style_context(win->split_deltas[prev]),
                "losing");
        } else {
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(win->split_deltas[prev]),
                "losing");
        }
    } else {
        gtk_style_context_remove_class(
            gtk_widget_get_style_context(win->split_deltas[prev]),
            "behind");
        if (win->timer->split_info[prev]
            & URN_INFO_LOSING_TIME) {
            gtk_style_context_add_class(
                gtk_widget_get_style_context(win->split_deltas[prev]),
                "losing");
        } else {
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(win->split_deltas[prev]),
                "losing");
        }
    }
    if (win->timer->split_deltas[prev]) {
        urn_delta_string(str, win->timer->split_deltas[prev]);
        gtk_label_set_text(GTK_LABEL(win->split_deltas[prev]), str);
    }
    // sum of bests
    if (win->timer->sum_of_bests) {
        urn_time_string(str, win->timer->sum_of_bests);
        gtk_label_set_text(GTK_LABEL(win->sum_of_bests), str);
    }
    // personal best
    if (win->game->split_count) {
        if (win->timer->split_times[win->game->split_count - 1]
            && !win->game->split_times[win->game->split_count - 1]
            || (win->timer->split_times[win->game->split_count - 1]
                < win->game->split_times[win->game->split_count - 1])) {
            urn_time_string(
                str, win->timer->split_times[win->game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(win->personal_best), str);
        } else if (win->game->split_times[win->game->split_count - 1]) {
            urn_time_string(
                str, win->game->split_times[win->game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(win->personal_best), str);
        }
    }
}

static gboolean urn_app_window_step(gpointer data) {
    UrnAppWindow *win = data;
    long long now = urn_time_now();
    if (win->timer) {
        urn_timer_step(win->timer, now);
    }
    return TRUE;
}

static void urn_app_window_show_game(UrnAppWindow *win) {
    char str[256];
    int i;

    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;
    
    char *style = 0;

    if (win->game->style) {
        int app_style_len = strlen(urn_app_window_style);
        style = malloc(strlen(win->game->style) + 1 + app_style_len + 1);
        strcpy(style, urn_app_window_style);
        style[app_style_len] = '\n';
        strcpy(&style[app_style_len + 1], win->game->style);
    } else {
        style = strdup(urn_app_window_style);
    }
    
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data(
        GTK_CSS_PROVIDER(provider),
        style, -1, NULL);
    g_object_unref(provider);

    if (style) {
        free(style);
    }

    gtk_label_set_text(GTK_LABEL(win->title), win->game->title);

    win->split_count = win->game->split_count;
    win->splits = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_titles = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_deltas = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_times = calloc(win->split_count, sizeof(GtkWidget *));

    for (i = 0; i < win->split_count; ++i) {
        win->splits[i] = gtk_grid_new();
        gtk_grid_set_column_homogeneous(GTK_GRID(win->splits[i]), TRUE);
        gtk_container_add(GTK_CONTAINER(win->split_box), win->splits[i]);
        gtk_widget_show(win->splits[i]);
        win->split_titles[i] = gtk_label_new(win->game->split_titles[i]);
        gtk_widget_set_halign(win->split_titles[i], GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(win->splits[i]),
                        win->split_titles[i], 0, 0, 3, 1);
        gtk_widget_show(win->split_titles[i]);
        win->split_deltas[i] = gtk_label_new(NULL);
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->split_deltas[i]),
            "split-delta");
        gtk_grid_attach(GTK_GRID(win->splits[i]),
                        win->split_deltas[i], 3, 0, 1, 1);
        gtk_widget_show(win->split_deltas[i]);
        win->split_times[i] = gtk_label_new(NULL);
        gtk_style_context_add_class(
            gtk_widget_get_style_context(win->split_times[i]),
            "split-time");
        gtk_widget_set_halign(win->split_times[i], GTK_ALIGN_END);
        gtk_grid_attach(GTK_GRID(win->splits[i]),
                        win->split_times[i], 4, 0, 1, 1);
        gtk_widget_show(win->split_times[i]);
        if (win->game->split_times[i]) {
            urn_split_string(str, win->game->split_times[i]);
            gtk_label_set_text(GTK_LABEL(win->split_times[i]), str);
        }
    }
    if (win->split_count) {
        // sum of bests
        if (win->timer->sum_of_bests) {
            urn_time_string(str, win->timer->sum_of_bests);
            gtk_label_set_text(GTK_LABEL(win->sum_of_bests), str);
        }
        // personal best
        if (win->game->split_times[win->game->split_count - 1]) {
            urn_time_string(
                str, win->game->split_times[win->game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(win->personal_best), str);
        }
    }
    
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->previous_segment),
        "behind");
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->previous_segment),
        "losing");
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->previous_segment),
        "best-segment");

    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->time),
        "behind");
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(win->time),
        "losing");

    if (win->game->world_record) {
        char str[64];
        win->world_record_label = gtk_label_new("World record");
        gtk_widget_set_halign(win->world_record_label, GTK_ALIGN_START);
        gtk_widget_set_hexpand(win->world_record_label, TRUE);
        gtk_widget_show(win->world_record_label);
        win->world_record = gtk_label_new(NULL);
        gtk_widget_set_halign(win->world_record, GTK_ALIGN_END);
        urn_time_string(str, win->game->world_record);
        gtk_label_set_text(GTK_LABEL(win->world_record), str);
        gtk_grid_attach(GTK_GRID(win->trailer),
                        win->world_record_label, 0, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(win->trailer),
                        win->world_record, 1, 3, 1, 1);
        gtk_widget_show(win->world_record);
    }
    gtk_widget_show(win->box);
}

static gboolean urn_app_window_key(GtkWidget *widget,
                                   GdkEventKey *event,
                                   gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    if (event->string && strlen(event->string)) {
        int key = (int)event->string[0];
        switch (key) {
        case 32:
            if (win->timer) {
                if (!win->timer->running) {
                    urn_timer_start(win->timer);
                    urn_app_window_start(win);
                } else {
                    if (urn_timer_split(win->timer)) {
                        urn_app_window_split(win);
                    }
                }
            }
            break;
        case 8:
            if (win->timer) {
                if (win->timer->running) {
                    urn_timer_stop(win->timer);
                } else {
                    urn_timer_reset(win->timer);
                    urn_app_window_clear_game(win);
                    urn_app_window_show_game(win);
                }
            }
            break;
        }
    }
    return TRUE;
}

static gboolean urn_app_window_draw(gpointer data) {
    UrnAppWindow *win = data;
    if (win->timer) {
        int curr;
        char str[256];
        const char *label;

        curr = win->timer->curr_split;
        if (curr == win->game->split_count) {
            --curr;
        }

        // current split
        if (win->timer->split_deltas[curr] > 0) {
            gtk_style_context_add_class(
                gtk_widget_get_style_context(win->split_deltas[curr]),
                "behind");
            if (win->timer->split_info[curr]
                & URN_INFO_LOSING_TIME) {
                gtk_style_context_add_class(
                    gtk_widget_get_style_context(win->split_deltas[curr]),
                    "losing");
            } else {
                gtk_style_context_remove_class(
                    gtk_widget_get_style_context(win->split_deltas[curr]),
                    "losing");
            }
            urn_delta_string(str, win->timer->split_deltas[curr]);
            gtk_label_set_text(GTK_LABEL(win->split_deltas[curr]), str);
        }

        if (win->timer->segment_deltas[curr] > 0) {
            // Live segment
            label = LIVE_SEGMENT;
            gtk_style_context_remove_class(
                gtk_widget_get_style_context(win->previous_segment),
                "best-segment");
            gtk_style_context_add_class(
                gtk_widget_get_style_context(win->previous_segment),
                "behind");
            gtk_style_context_add_class(
                gtk_widget_get_style_context(win->previous_segment),
                "losing");
            urn_delta_string(str, win->timer->segment_deltas[curr]);
            gtk_label_set_text(GTK_LABEL(win->previous_segment), str);
        } else {
            // Previous segment
            label = PREVIOUS_SEGMENT;
            if (win->timer->curr_split) {
                int prev = win->timer->curr_split - 1;
                if (win->timer->segment_deltas[prev]) {
                    if (win->timer->split_info[prev]
                        & URN_INFO_BEST_SEGMENT) {
                        gtk_style_context_add_class(
                            gtk_widget_get_style_context(win->previous_segment),
                            "best-segment");
                    } else {
                        gtk_style_context_remove_class(
                            gtk_widget_get_style_context(win->previous_segment),
                            "best-segment");
                        if (win->timer->segment_deltas[prev] > 0) {
                            gtk_style_context_add_class(
                                gtk_widget_get_style_context(win->previous_segment),
                                "behind");
                            gtk_style_context_add_class(
                                gtk_widget_get_style_context(win->previous_segment),
                                "losing");
                        } else {
                            gtk_style_context_remove_class(
                                gtk_widget_get_style_context(win->previous_segment),
                                "behind");
                            gtk_style_context_remove_class(
                                gtk_widget_get_style_context(win->previous_segment),
                                "losing");
                        }
                    }
                    urn_delta_string(str, win->timer->segment_deltas[prev]);
                    gtk_label_set_text(GTK_LABEL(win->previous_segment), str);
                }
            }
        }
        gtk_label_set_text(GTK_LABEL(win->previous_segment_label), label);

        // running time
        if (curr == win->game->split_count) {
            curr = win->game->split_count - 1;
        }
        if (win->timer->time < 0) {
            gtk_style_context_add_class(gtk_widget_get_style_context(win->time),
                                        "delay");
        } else {
            gtk_style_context_remove_class(gtk_widget_get_style_context(win->time),
                                           "delay");
            if (win->timer->split_info[curr]
                & URN_INFO_BEHIND_TIME) {
                gtk_style_context_add_class(
                    gtk_widget_get_style_context(win->time),
                    "behind");
                if (win->timer->split_info[curr]
                    & URN_INFO_LOSING_TIME) {
                    gtk_style_context_add_class(
                        gtk_widget_get_style_context(win->time),
                        "losing");
                } else {
                    gtk_style_context_remove_class(
                        gtk_widget_get_style_context(win->time),
                        "losing");
                }
            } else {
                gtk_style_context_remove_class(
                    gtk_widget_get_style_context(win->time),
                    "behind");
                if (win->timer->split_info[curr]
                    & URN_INFO_LOSING_TIME) {
                    gtk_style_context_add_class(
                        gtk_widget_get_style_context(win->time),
                        "losing");
                } else {
                    gtk_style_context_remove_class(
                        gtk_widget_get_style_context(win->time),
                        "losing");
                }
            }
        }
        urn_time_string(str, win->timer->time);
        gtk_label_set_text(GTK_LABEL(win->time), str);
    }
    return TRUE;
}


static void urn_app_window_init(UrnAppWindow *win) {
    PangoFontDescription *font;
    GtkWidget *label;

    // Load CSS defaults
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data(
        GTK_CSS_PROVIDER(provider),
        urn_app_window_style, -1, NULL);
    g_object_unref(provider);

    // Load window junk
    gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(win)),
                                "window");
    win->game = 0;
    win->timer = 0;
    
    g_signal_connect(win, "destroy",
                     G_CALLBACK(urn_app_window_destroy), NULL);
    g_signal_connect(win, "key_press_event",
                     G_CALLBACK(urn_app_window_key), win);
    gtk_window_set_default_size(GTK_WINDOW(win), 320, 240);
    
    win->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_left(win->box, 8);
    gtk_widget_set_margin_top(win->box, 4);
    gtk_widget_set_margin_right(win->box, 8);
    gtk_widget_set_margin_bottom(win->box, 8);
    gtk_widget_set_vexpand(win->box, TRUE);
    gtk_container_add(GTK_CONTAINER(win), win->box);
    
    win->title = gtk_label_new(NULL);
    font = pango_font_description_new();
    pango_font_description_set_size(font, 14*PANGO_SCALE);
    gtk_widget_override_font(win->title, font);
    gtk_container_add(GTK_CONTAINER(win->box), win->title);
    gtk_widget_show(win->title);

    win->split_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(win->split_box, TRUE);
    gtk_widget_set_hexpand(win->split_box, TRUE);
    gtk_container_add(GTK_CONTAINER(win->box), win->split_box);
    gtk_widget_show(win->split_box);

    win->time = gtk_label_new(NULL);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(win->time), "timer");
    gtk_widget_set_halign(win->time, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(win->box), win->time);
    gtk_widget_show(win->time);

    win->trailer = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(win->box), win->trailer);
    gtk_widget_show(win->trailer);

    win->previous_segment_label =
        gtk_label_new(PREVIOUS_SEGMENT);
    gtk_widget_set_halign(win->previous_segment_label,
                          GTK_ALIGN_START);
    gtk_widget_set_hexpand(win->previous_segment_label, TRUE);
    gtk_grid_attach(GTK_GRID(win->trailer),
                    win->previous_segment_label, 0, 0, 1, 1);
    gtk_widget_show(win->previous_segment_label);

    win->previous_segment = gtk_label_new(NULL);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(win->previous_segment),
        "split-delta");
    gtk_widget_set_halign(win->previous_segment, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->trailer),
                    win->previous_segment, 1, 0, 1, 1);
    gtk_widget_show(win->previous_segment);
    
    label = gtk_label_new("Sum of bests");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_grid_attach(GTK_GRID(win->trailer),
                    label, 0, 1, 1, 1);
    gtk_widget_show(label);

    win->sum_of_bests = gtk_label_new(NULL);
    gtk_widget_set_halign(win->sum_of_bests, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->trailer),
                    win->sum_of_bests, 1, 1, 1, 1);
    gtk_widget_show(win->sum_of_bests);

    label = gtk_label_new("Personal best");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_grid_attach(GTK_GRID(win->trailer), label, 0, 2, 1, 1);
    gtk_widget_show(label);

    win->personal_best = gtk_label_new(NULL);
    gtk_widget_set_halign(win->personal_best, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->trailer), win->personal_best, 1, 2, 1, 1);
    gtk_widget_show(win->personal_best);

    g_timeout_add(1, urn_app_window_step, win);
    g_timeout_add((int)(1000 / 60.), urn_app_window_draw, win); 
}

static void urn_app_window_class_init(UrnAppWindowClass *class) {
}

static UrnAppWindow *urn_app_window_new(UrnApp *app) {
    return g_object_new(URN_APP_WINDOW_TYPE, "application", app, NULL);
}

static void urn_window_open(UrnAppWindow *win, const char *file) {
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

static void urn_app_init(UrnApp *app) {
}

static void urn_app_activate(GApplication *app) {
    UrnAppWindow *win;
    win = urn_app_window_new(URN_APP(app));
    gtk_window_present(GTK_WINDOW(win));
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
        urn_window_open(win, g_file_get_path(files[i]));
    }
    gtk_window_present(GTK_WINDOW(win));
}

static void open_activated(GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       app) {
    GList *windows;
    UrnAppWindow *win;
    GtkWidget *dialog;
    gint res;
    int i;
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
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        urn_window_open(win, filename);
        g_free(filename);
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
        urn_game_update_splits(win->game, win->timer);
        urn_game_save(win->game);
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
        urn_game_save(win->game);
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
        urn_window_open(win, path);
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

static void urn_app_class_init(UrnAppClass *class) {
    G_APPLICATION_CLASS(class)->activate = urn_app_activate;
    G_APPLICATION_CLASS(class)->open = urn_app_open;
    G_APPLICATION_CLASS(class)->startup = urn_app_startup;
}

UrnApp *urn_app_new(void) {
    g_set_application_name("urn");
    return g_object_new(URN_APP_TYPE,
                        "application-id", "wildmouse.urn",
                        "flags", G_APPLICATION_HANDLES_OPEN,
                        NULL);
}

int main(int argc, char *argv[]) {
    return g_application_run(G_APPLICATION(urn_app_new()), argc, argv);
}
