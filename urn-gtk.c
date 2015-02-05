#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
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

    ".title {\n"
    "  font-size: large;\n"
    "}\n"

    ".attempt-count {\n"
    "  color: #999;\n"
    "}"

    ".timer {\n"
    "  font-size: 300%;\n"
    "  text-shadow: 2px 2px #666;\n"
    "}\n"

    ".timer-millis {\n"
    "  font-size: 75%;\n"
    "}\n"

    ".split-time {\n"
    "  color: #FFF;\n"
    "}\n"

    ".done {\n"
    "  color: #999;\n"
    "}\n"

    ".time, .delta {\n"
    "  font-weight: bold;\n"
    "}\n"

    ".timer, .delta {\n"
    "  color: #0C0;\n"
    "}\n"

    ".losing {\n"
    "  color: #6A6;\n"
    "}\n"

    ".behind {\n"
    "  color: #A66;\n"
    "}\n"

    ".behind.losing {\n"
    "  color: #C00;\n"
    "}\n"

    ".delay {\n"
    "  color: #999;\n"
    "}\n"

    ".best-segment {\n"
    "  color: #F90;\n"
    "}\n"

    ".best-split {\n"
    "  color: #99F;\n"
    "}\n"

    ".current-split {\n"
    "  background-color: rgba(127, 127, 255, 0.3);\n"
    "}\n"

    ".split-last {\n"
    "  border-top: 1px #FFF solid;\n"
    "}\n"
    ;

struct _UrnAppWindow {
    GtkApplicationWindow parent;
    int decorated;
    urn_game *game;
    urn_timer *timer;
    int split_count;
    GdkDisplay *display;
    GtkWidget *box;
    GtkWidget *header;
    GtkWidget *title;
    GtkWidget *attempt_count;
    GtkWidget **splits;
    GtkWidget *split_box;
    GtkWidget *split_last;
    GtkAdjustment *split_adjust;
    GtkWidget *split_scroller;
    GtkWidget *split_viewport;
    GtkWidget **split_titles;
    GtkWidget **split_deltas;
    GtkWidget **split_times;
    GtkWidget *footer;
    GtkWidget *sum_of_bests;
    GtkWidget *previous_segment_label;
    GtkWidget *previous_segment;
    GtkWidget *personal_best;
    GtkWidget *world_record_label;
    GtkWidget *world_record;
    GtkWidget *time;
    GtkWidget *time_seconds;
    GtkWidget *time_millis;
    GtkCssProvider *style;
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

#define PREVIOUS_SEGMENT      "Previous segment"
#define LIVE_SEGMENT          "Live segment"
#define SUM_OF_BEST_SEGMENTS  "Sum of best segments"
#define PERSONAL_BEST         "Personal best"
#define WORLD_RECORD          "World record"

static void add_class(GtkWidget *widget, const char *class) {
    gtk_style_context_add_class(
        gtk_widget_get_style_context(widget), class);
}

static void remove_class(GtkWidget *widget, const char *class) {
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(widget), class);
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
    int i;
    gtk_widget_hide(win->box);
    gtk_widget_hide(win->split_box);
    gtk_widget_hide(win->split_last);
    gtk_widget_hide(win->world_record_label);
    gtk_widget_hide(win->world_record);
    for (i = 0; i < win->split_count; ++i) {
        gtk_container_remove(
            GTK_CONTAINER(gtk_widget_get_parent(win->splits[i])),
            win->splits[i]);
    }
    gtk_adjustment_set_value(win->split_adjust, 0);
    free(win->splits);
    free(win->split_titles);
    free(win->split_deltas);
    free(win->split_times);
    win->split_count = 0;
    gtk_label_set_text(GTK_LABEL(win->time_seconds), "");
    gtk_label_set_text(GTK_LABEL(win->time_millis), "");
    gtk_label_set_text(GTK_LABEL(win->previous_segment_label),
                       PREVIOUS_SEGMENT);
    gtk_label_set_text(GTK_LABEL(win->previous_segment), "");
    gtk_label_set_text(GTK_LABEL(win->sum_of_bests), "");
    gtk_label_set_text(GTK_LABEL(win->personal_best), "");

    // remove game's style
    screen = gdk_display_get_default_screen(win->display);
    gtk_style_context_remove_provider_for_screen(
        screen, GTK_STYLE_PROVIDER(win->style));
    g_object_unref(win->style);
}

static gboolean urn_app_window_step(gpointer data) {
    UrnAppWindow *win = data;
    long long now = urn_time_now();
    static int set_cursor;
    if (!set_cursor) {
        GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(win));
        if (gdk_window) {
            GdkCursor* cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
            gdk_window_set_cursor(gdk_window, cursor);
            set_cursor = 1;
        }
    }
    if (win->timer) {
        urn_timer_step(win->timer, now);
    }
    return TRUE;
}

static void urn_app_window_split_trailer(UrnAppWindow *win) {
    if (win->timer) {
        GdkRectangle rect;
        GtkWidget *last = win->splits[win->split_count - 1];
        double curr_scroll = gtk_adjustment_get_value(win->split_adjust);
        double scroll_max = gtk_adjustment_get_upper(win->split_adjust);
        double page_size = gtk_adjustment_get_page_size(win->split_adjust);
        int split_h;
        g_object_ref(last);
        gtk_widget_get_allocation(last, &rect);
        split_h = rect.height;
        gtk_widget_get_allocation(win->split_box, &rect);
        if (gtk_widget_get_parent(last) == win->split_box) {
            if (curr_scroll + page_size < scroll_max) {
                // move last split to split_last
                add_class(last, "split-last");
                gtk_container_remove(GTK_CONTAINER(win->split_box), last);
                gtk_container_add(GTK_CONTAINER(win->split_last), last);
                gtk_adjustment_set_upper(win->split_adjust,
                                         scroll_max - split_h);
                gtk_widget_show(win->split_last);
            }
        } else {
            if (curr_scroll + page_size == scroll_max) {
                // move last split to split box
                remove_class(last, "split-last");
                gtk_container_remove(GTK_CONTAINER(win->split_last), last);
                gtk_container_add(GTK_CONTAINER(win->split_box), last);
                gtk_adjustment_set_upper(win->split_adjust,
                                         scroll_max + rect.height);
                gtk_adjustment_set_value(win->split_adjust,
                                         curr_scroll + split_h);
                gtk_widget_hide(win->split_last);
            }
        }
        gtk_widget_set_hexpand(last, TRUE);
        g_object_unref(last);
    }
}

static void urn_app_window_show_game(UrnAppWindow *win) {
    GdkScreen *screen;
    char str[256];
    char *ptr;
    int i;
    
    // set dimensions
    if (win->game->width > 0 && win->game->height > 0) {
        gtk_widget_set_size_request(GTK_WIDGET(win),
                                    win->game->width,
                                    win->game->height);
    }

    // set window css provider
    strcpy(str, win->game->path);
    ptr = strrchr(str, '.');
    if (!ptr) {
        ptr = &str[strlen(str)];
    }
    strcpy(ptr, ".css");
    win->style = gtk_css_provider_new();
    screen = gdk_display_get_default_screen(win->display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(win->style),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_path(
        GTK_CSS_PROVIDER(win->style),
        str, NULL);

    gtk_label_set_text(GTK_LABEL(win->title), win->game->title);

    sprintf(str, "#%d", win->game->attempt_count);
    gtk_label_set_text(GTK_LABEL(win->attempt_count), str);

    win->split_count = win->game->split_count;
    win->splits = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_titles = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_deltas = calloc(win->split_count, sizeof(GtkWidget *));
    win->split_times = calloc(win->split_count, sizeof(GtkWidget *));

    for (i = 0; i < win->split_count; ++i) {
        win->splits[i] = gtk_grid_new();
        add_class(win->splits[i], "split");
        gtk_grid_set_column_homogeneous(GTK_GRID(win->splits[i]), TRUE);
        gtk_container_add(GTK_CONTAINER(win->split_box), win->splits[i]);
        gtk_widget_show(win->splits[i]);

        if (win->game->split_titles[i] && strlen(win->game->split_titles[i])) {
            char *c = &str[12];
            strcpy(str, "split-title-");
            strcpy(c, win->game->split_titles[i]);
            do {
                if (!isalnum(*c)) {
                    *c = '-';
                } else {
                    *c = tolower(*c);
                }
            } while (*++c != '\0');
            add_class(win->splits[i], str);
        }

        win->split_titles[i] = gtk_label_new(win->game->split_titles[i]);
        add_class(win->split_titles[i], "split-title");
        gtk_widget_set_halign(win->split_titles[i], GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(win->splits[i]),
                        win->split_titles[i], 0, 0, 4, 1);
        gtk_widget_show(win->split_titles[i]);
        
        win->split_deltas[i] = gtk_label_new(NULL);
        add_class(win->split_deltas[i], "split-delta");
        gtk_grid_attach(GTK_GRID(win->splits[i]),
                        win->split_deltas[i], 3, 0, 1, 1);
        gtk_widget_show(win->split_deltas[i]);
        
        win->split_times[i] = gtk_label_new(NULL);
        add_class(win->split_times[i], "split-time");
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
    
    remove_class(win->previous_segment, "behind");
    remove_class(win->previous_segment, "losing");
    remove_class(win->previous_segment, "best-segment");

    remove_class(win->time, "behind");
    remove_class(win->time, "losing");

    if (win->game->world_record) {
        char str[256];
        gtk_widget_set_halign(win->world_record_label, GTK_ALIGN_START);
        gtk_widget_set_hexpand(win->world_record_label, TRUE);
        urn_time_string(str, win->game->world_record);
        gtk_label_set_text(GTK_LABEL(win->world_record), str);
        gtk_widget_show(win->world_record_label);
        gtk_widget_show(win->world_record);
    }
    gtk_widget_show(win->box);
    gtk_widget_show(win->split_box);

    urn_app_window_split_trailer(win);
}

static void urn_app_window_scroll_to_split(UrnAppWindow *win) {
    GdkRectangle rect;
    int split_x, split_y;
    int split_h;
    int scroller_h;
    double curr_scroll;
    double min_scroll, max_scroll;
    int prev = win->timer->curr_split - 1;
    int curr = win->timer->curr_split;
    int next = win->timer->curr_split + 1;
    if (prev < 0) {
        prev = 0;
    }
    if (curr >= win->split_count) {
        curr = win->split_count - 1;
    }
    if (next >= win->split_count) {
        next = win->split_count - 1;
    }
    curr_scroll = gtk_adjustment_get_value(win->split_adjust);
    gtk_widget_translate_coordinates(
        win->splits[prev],
        win->split_viewport,
        0, 0, &split_x, &split_y);
    gtk_widget_get_allocation(win->splits[prev], &rect);
    split_h = rect.height;
    if (curr != next && curr != prev) {
        gtk_widget_get_allocation(win->splits[curr], &rect);
        split_h += rect.height;
    }
    if (prev != next) {
        gtk_widget_get_allocation(win->splits[next], &rect);
        split_h += rect.height;
    }
    gtk_widget_get_allocation(win->split_scroller, &rect);
    scroller_h = rect.height;
    min_scroll = split_y + curr_scroll - scroller_h + split_h;
    max_scroll = split_y + curr_scroll;
    if (curr_scroll > max_scroll) {
        gtk_adjustment_set_value(win->split_adjust, max_scroll);
    } else if (curr_scroll < min_scroll) {
        gtk_adjustment_set_value(win->split_adjust, min_scroll);
    }
}

static gboolean urn_app_window_scrolled(GtkWidget *widget,
                                        GdkEvent *event,
                                        gpointer data) {
    UrnAppWindow *win = data;
    urn_app_window_split_trailer(win);
    return FALSE;
}

#define WINDOW_PAD (8)

static void resize_window(UrnAppWindow *win,
                          int window_width,
                          int window_height) {
    GdkRectangle rect;
    int attempt_count_width;
    int title_width;
    gtk_widget_hide(win->title);
    gtk_widget_get_allocation(win->attempt_count, &rect);
    attempt_count_width = rect.width;
    title_width = window_width - 2 * WINDOW_PAD - attempt_count_width;
    rect.width = title_width;
    gtk_widget_show(win->title);
    gtk_widget_set_allocation(win->title, &rect);
}

static gboolean urn_app_window_resize(GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    resize_window(win, event->configure.width, event->configure.height);
    return FALSE;
}

static gboolean urn_app_window_key(GtkWidget *widget,
                                   GdkEventKey *event,
                                   gpointer data) {
    UrnAppWindow *win = (UrnAppWindow*)widget;
    switch (event->keyval) {
    case GDK_KEY_space:
        if (win->timer) {
            if (!win->timer->running) {
                if (urn_timer_start(win->timer)) {
                    save_game(win->game);
                }
            } else {
                urn_timer_split(win->timer);
            }
            urn_app_window_scroll_to_split(win);
        }
        break;
    case GDK_KEY_BackSpace:
        if (win->timer) {
            if (win->timer->running) {
                urn_timer_stop(win->timer);
            } else {
                if (urn_timer_reset(win->timer)) {
                    urn_app_window_clear_game(win);
                    urn_app_window_show_game(win);
                    save_game(win->game);
                }
            }
        }
        break;
    case GDK_KEY_Delete:
        if (win->timer) {
            if (urn_timer_cancel(win->timer)) {
                urn_app_window_clear_game(win);
                urn_app_window_show_game(win);
                save_game(win->game);
            }
        }
        break;
    case GDK_KEY_Page_Down:
        if (win->timer) {
            urn_timer_skip(win->timer);
            urn_app_window_scroll_to_split(win);
        }
        break;
    case GDK_KEY_Page_Up:
        if (win->timer) {
            urn_timer_unsplit(win->timer);
            urn_app_window_scroll_to_split(win);
        }
        break;
    case GDK_KEY_Control_R:
        gtk_window_set_decorated(GTK_WINDOW(win), !win->decorated);
        win->decorated = !win->decorated;
        break;
    }
    return TRUE;
}

#define SHOW_DELTA_THRESHOLD (-30 * 1000000L)

static gboolean urn_app_window_draw(gpointer data) {
    UrnAppWindow *win = data;
    if (win->timer) {
        int curr;
        int prev;
        char str[256];
        char millis[256];
        const char *label;
        int i;

        curr = win->timer->curr_split;
        if (curr == win->game->split_count) {
            --curr;
        }

        sprintf(str, "#%d", win->game->attempt_count);
        gtk_label_set_text(GTK_LABEL(win->attempt_count), str);

        // splits
        for (i = 0; i < win->split_count; ++i) {
            if (i == win->timer->curr_split
                && win->timer->start_time) {
                add_class(win->splits[i], "current-split");
            } else {
                remove_class(win->splits[i], "current-split");
            }
            
            remove_class(win->split_times[i], "time");
            remove_class(win->split_times[i], "done");
            gtk_label_set_text(GTK_LABEL(win->split_times[i]), "-");
            if (i < win->timer->curr_split) {
                add_class(win->split_times[i], "done");
                if (win->timer->split_times[i]) {
                    add_class(win->split_times[i], "time");
                    urn_split_string(str, win->timer->split_times[i]);
                    gtk_label_set_text(GTK_LABEL(win->split_times[i]), str);
                }
            } else if (win->game->split_times[i]) {
                add_class(win->split_times[i], "time");
                urn_split_string(str, win->game->split_times[i]);
                gtk_label_set_text(GTK_LABEL(win->split_times[i]), str);
            }
            
            remove_class(win->split_deltas[i], "best-split");
            remove_class(win->split_deltas[i], "best-segment");
            remove_class(win->split_deltas[i], "behind");
            remove_class(win->split_deltas[i], "losing");
            remove_class(win->split_deltas[i], "delta");
            gtk_label_set_text(GTK_LABEL(win->split_deltas[i]), "");
            if (i < win->timer->curr_split
                || win->timer->split_deltas[i] >= SHOW_DELTA_THRESHOLD) {
                if (win->timer->split_info[i] & URN_INFO_BEST_SPLIT) {
                    add_class(win->split_deltas[i], "best-split");
                } else if (win->timer->split_info[i]
                           & URN_INFO_BEST_SEGMENT) {
                    add_class(win->split_deltas[i], "best-segment");
                } else if (win->timer->split_info[i]
                           & URN_INFO_BEHIND_TIME) {
                    add_class(win->split_deltas[i], "behind");
                    if (win->timer->split_info[i]
                        & URN_INFO_LOSING_TIME) {
                        add_class(win->split_deltas[i], "losing");
                    }
                } else {
                    remove_class(win->split_deltas[i], "behind");
                    if (win->timer->split_info[i]
                        & URN_INFO_LOSING_TIME) {
                        add_class(win->split_deltas[i], "losing");
                    }
                }
                if (win->timer->split_deltas[i]) {
                    add_class(win->split_deltas[i], "delta");
                    urn_delta_string(str, win->timer->split_deltas[i]);
                    gtk_label_set_text(GTK_LABEL(win->split_deltas[i]), str);
                }
            }
        }

        // Previous segment
        label = PREVIOUS_SEGMENT;
        remove_class(win->previous_segment, "best-segment");
        remove_class(win->previous_segment, "behind");
        remove_class(win->previous_segment, "losing");
        remove_class(win->previous_segment, "delta");
        gtk_label_set_text(GTK_LABEL(win->previous_segment), "-");
        if (win->timer->segment_deltas[curr] > 0) {
            // Live segment
            label = LIVE_SEGMENT;
            remove_class(win->previous_segment, "best-segment");
            add_class(win->previous_segment, "behind");
            add_class(win->previous_segment, "losing");
            add_class(win->previous_segment, "delta");
            urn_delta_string(str, win->timer->segment_deltas[curr]);
            gtk_label_set_text(GTK_LABEL(win->previous_segment), str);
        } else if (curr) {
            prev = win->timer->curr_split - 1;
            // Previous segment
            if (win->timer->curr_split) {
                int prev = win->timer->curr_split - 1;
                if (win->timer->segment_deltas[prev]) {
                    if (win->timer->split_info[prev]
                        & URN_INFO_BEST_SEGMENT) {
                        add_class(win->previous_segment, "best-segment");
                    } else {
                        remove_class(win->previous_segment, "best-segment");
                        if (win->timer->segment_deltas[prev] > 0) {
                            add_class(win->previous_segment, "behind");
                            add_class(win->previous_segment, "losing");
                        }
                    }
                    add_class(win->previous_segment, "delta");
                    urn_delta_string(str, win->timer->segment_deltas[prev]);
                    gtk_label_set_text(GTK_LABEL(win->previous_segment), str);
                }
            }
        }
        gtk_label_set_text(GTK_LABEL(win->previous_segment_label), label);

        // running time
        remove_class(win->time, "delay");
        remove_class(win->time, "behind");
        remove_class(win->time, "losing");
        remove_class(win->time, "best-split");
        if (curr == win->game->split_count) {
            curr = win->game->split_count - 1;
        }
        if (win->timer->time <= 0) {
            add_class(win->time, "delay");
        } else {
            if (win->timer->curr_split == win->split_count
                && win->timer->split_info[curr]
                   & URN_INFO_BEST_SPLIT) {
                add_class(win->time, "best-split");
            } else{
                if (win->timer->split_info[curr]
                    & URN_INFO_BEHIND_TIME) {
                    add_class(win->time, "behind");
                }
                if (win->timer->split_info[curr]
                    & URN_INFO_LOSING_TIME) {
                    add_class(win->time, "losing");
                }
            }
        }
        urn_time_millis_string(str, &millis[1], win->timer->time);
        millis[0] = '.';
        gtk_label_set_text(GTK_LABEL(win->time_seconds), str);
        gtk_label_set_text(GTK_LABEL(win->time_millis), millis);

        // sum of bests
        remove_class(win->sum_of_bests, "time");
        gtk_label_set_text(GTK_LABEL(win->sum_of_bests), "-");
        if (win->timer->sum_of_bests) {
            add_class(win->sum_of_bests, "time");
            urn_time_string(str, win->timer->sum_of_bests);
            gtk_label_set_text(GTK_LABEL(win->sum_of_bests), str);
        }

        // personal best
        remove_class(win->personal_best, "time");
        gtk_label_set_text(GTK_LABEL(win->personal_best), "-");
        if (win->timer->curr_split == win->game->split_count
            && win->timer->split_times[win->game->split_count - 1]
            && (!win->game->split_times[win->game->split_count - 1]
                || (win->timer->split_times[win->game->split_count - 1]
                    < win->game->split_times[win->game->split_count - 1]))) {
            add_class(win->personal_best, "time");
            urn_time_string(
                str, win->timer->split_times[win->game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(win->personal_best), str);
        } else if (win->game->split_times[win->game->split_count - 1]) {
            add_class(win->personal_best, "time");
            urn_time_string(
                str, win->game->split_times[win->game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(win->personal_best), str);
        }

        //resize_window(win);
        urn_app_window_split_trailer(win);
    }
    return TRUE;
}

static void urn_app_window_init(UrnAppWindow *win) {
    GtkWidget *label;
    GtkWidget *spacer;
    GdkRGBA color;
    GdkPixbuf *pix;
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    char user_style_path[256];
    strcpy(user_style_path, homedir);
    strcat(user_style_path, "/.urn/style.css");
    
    win->display = gdk_display_get_default();
    
    // no winodw border
    win->decorated = 0;
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);

    // Load CSS defaults
    GtkCssProvider *provider;
    GdkScreen *screen;
    provider = gtk_css_provider_new();
    screen = gdk_display_get_default_screen(win->display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data(
        GTK_CSS_PROVIDER(provider),
        urn_app_window_style, -1, NULL);
    g_object_unref(provider);

    // Load user CSS defaults
    provider = gtk_css_provider_new();
    screen = gdk_display_get_default_screen(win->display);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_path(
        GTK_CSS_PROVIDER(provider),
        user_style_path, NULL);
    g_object_unref(provider);

    // Load window junk
    add_class(GTK_WIDGET(win), "window");
    win->game = 0;
    win->timer = 0;
    
    g_signal_connect(win, "destroy",
                     G_CALLBACK(urn_app_window_destroy), NULL);
    g_signal_connect(win, "key_press_event",
                     G_CALLBACK(urn_app_window_key), win);
    g_signal_connect(win, "configure-event",
                     G_CALLBACK(urn_app_window_resize), win);
    
    win->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(win->box, WINDOW_PAD);
    gtk_widget_set_margin_bottom(win->box, WINDOW_PAD);
    gtk_widget_set_vexpand(win->box, TRUE);
    gtk_container_add(GTK_CONTAINER(win), win->box);

    win->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(win->header, "header");
    gtk_widget_set_margin_left(win->header, WINDOW_PAD);
    gtk_widget_set_margin_right(win->header, WINDOW_PAD);
    gtk_container_add(GTK_CONTAINER(win->box), win->header);
    gtk_widget_show(win->header);
    
    win->title = gtk_label_new(NULL);
    add_class(win->title, "title");
    gtk_label_set_justify(GTK_LABEL(win->title), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(win->title), TRUE);
    gtk_widget_set_hexpand(win->title, TRUE);
    gtk_container_add(GTK_CONTAINER(win->header), win->title);

    win->attempt_count = gtk_label_new(NULL);
    add_class(win->attempt_count, "attempt-count");
    gtk_widget_set_margin_left(win->attempt_count, WINDOW_PAD);
    gtk_widget_set_valign(win->attempt_count, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(win->header), win->attempt_count);
    gtk_widget_show(win->attempt_count);

    win->split_adjust = gtk_adjustment_new(0., 0., 0., 0., 0., 0.);

    win->split_scroller = gtk_scrolled_window_new(NULL, win->split_adjust);
    gtk_widget_set_vexpand(win->split_scroller, TRUE);
    gtk_widget_set_hexpand(win->split_scroller, TRUE);
    gtk_container_add(GTK_CONTAINER(win->box), win->split_scroller);
    gtk_widget_show(win->split_scroller);
    gtk_widget_add_events(win->split_scroller, GDK_SCROLL_MASK);

    // hide split scrollbar
    gdk_rgba_parse(&color, "rgba(0,0,0,0)");
    gtk_widget_override_background_color(
        GTK_WIDGET(
            gtk_scrolled_window_get_vscrollbar(
                GTK_SCROLLED_WINDOW(win->split_scroller))),
        GTK_STATE_NORMAL,
        &color);
    
    win->split_viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_margin_left(win->split_viewport, WINDOW_PAD);
    gtk_widget_set_margin_right(win->split_viewport, WINDOW_PAD);
    gtk_container_add(GTK_CONTAINER(win->split_scroller), win->split_viewport);
    gtk_widget_show(win->split_viewport);

    win->split_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(win->split_box, "splits");
    gtk_container_add(GTK_CONTAINER(win->split_viewport), win->split_box);
    gtk_widget_show(win->split_box);
    
    win->split_last = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_left(win->split_last, WINDOW_PAD);
    gtk_widget_set_margin_right(win->split_last, WINDOW_PAD);
    gtk_widget_set_hexpand(win->split_last, TRUE);
    gtk_container_add(GTK_CONTAINER(win->box), win->split_last);
    gtk_widget_show(win->split_last);

    win->time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(win->time, "timer");
    add_class(win->time, "time");
    gtk_widget_set_margin_left(win->time, WINDOW_PAD);
    gtk_widget_set_margin_right(win->time, WINDOW_PAD);
    gtk_container_add(GTK_CONTAINER(win->box), win->time);
    gtk_widget_show(win->time);

    spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_container_add(GTK_CONTAINER(win->time), spacer);
    gtk_widget_show(spacer);

    win->time_seconds = gtk_label_new(NULL);
    add_class(win->time_seconds, "timer-seconds");
    gtk_widget_set_valign(win->time_seconds, GTK_ALIGN_BASELINE);
    gtk_container_add(GTK_CONTAINER(win->time), win->time_seconds);
    gtk_widget_show(win->time_seconds);

    spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(spacer, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(win->time), spacer);
    gtk_widget_show(spacer);

    win->time_millis = gtk_label_new(NULL);
    add_class(win->time_millis, "timer-millis");
    gtk_widget_set_valign(win->time_millis, GTK_ALIGN_BASELINE);
    gtk_container_add(GTK_CONTAINER(spacer), win->time_millis);
    gtk_widget_show(win->time_millis);

    win->footer = gtk_grid_new();
    add_class(win->footer, "footer");
    gtk_widget_set_margin_left(win->footer, WINDOW_PAD);
    gtk_widget_set_margin_right(win->footer, WINDOW_PAD);
    gtk_container_add(GTK_CONTAINER(win->box), win->footer);
    gtk_widget_show(win->footer);

    win->previous_segment_label =
        gtk_label_new(PREVIOUS_SEGMENT);
    add_class(win->previous_segment_label, "prev-segment-label");
    gtk_widget_set_halign(win->previous_segment_label,
                          GTK_ALIGN_START);
    gtk_widget_set_hexpand(win->previous_segment_label, TRUE);
    gtk_grid_attach(GTK_GRID(win->footer),
                    win->previous_segment_label, 0, 0, 1, 1);
    gtk_widget_show(win->previous_segment_label);

    win->previous_segment = gtk_label_new(NULL);
    add_class(win->previous_segment, "prev-segment");
    gtk_widget_set_margin_left(win->previous_segment, WINDOW_PAD);
    gtk_widget_set_halign(win->previous_segment, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->footer),
                    win->previous_segment, 1, 0, 1, 1);
    gtk_widget_show(win->previous_segment);
    
    label = gtk_label_new(SUM_OF_BEST_SEGMENTS);
    add_class(label, "sum-of-bests-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_grid_attach(GTK_GRID(win->footer),
                    label, 0, 1, 1, 1);
    gtk_widget_show(label);

    win->sum_of_bests = gtk_label_new(NULL);
    add_class(win->sum_of_bests, "sum-of-bests");
    gtk_widget_set_margin_left(win->sum_of_bests, WINDOW_PAD);
    gtk_widget_set_halign(win->sum_of_bests, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->footer),
                    win->sum_of_bests, 1, 1, 1, 1);
    gtk_widget_show(win->sum_of_bests);

    label = gtk_label_new(PERSONAL_BEST);
    add_class(label, "personal-best-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_grid_attach(GTK_GRID(win->footer), label, 0, 2, 1, 1);
    gtk_widget_show(label);

    win->personal_best = gtk_label_new(NULL);
    add_class(win->personal_best, "personal-best");
    gtk_widget_set_margin_left(win->personal_best, WINDOW_PAD);
    gtk_widget_set_halign(win->personal_best, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->footer), win->personal_best, 1, 2, 1, 1);
    gtk_widget_show(win->personal_best);

    win->world_record_label = gtk_label_new(WORLD_RECORD);
    add_class(win->world_record_label, "world-record-label");
    gtk_grid_attach(GTK_GRID(win->footer),
                    win->world_record_label, 0, 3, 1, 1);
    
    win->world_record = gtk_label_new(NULL);
    add_class(win->world_record, "world-record");
    add_class(win->world_record, "time");
    gtk_widget_set_margin_left(win->world_record, WINDOW_PAD);
    gtk_widget_set_halign(win->world_record, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(win->footer),
                    win->world_record, 1, 3, 1, 1);

    g_timeout_add(1, urn_app_window_step, win);
    g_timeout_add((int)(1000 / 30.), urn_app_window_draw, win); 
}

static void urn_app_window_class_init(UrnAppWindowClass *class) {
}

static UrnAppWindow *urn_app_window_new(UrnApp *app) {
    return g_object_new(URN_APP_WINDOW_TYPE, "application", app, NULL);
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
        urn_app_window_open(win, filename);
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
