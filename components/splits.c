#include "urn-component.h"

typedef struct _UrnSplits {
    UrnComponent base;
    int split_count;
    GtkWidget *container;
    GtkWidget *splits;
    GtkWidget *split_last;
    GtkAdjustment *split_adjust;
    GtkWidget *split_scroller;
    GtkWidget *split_viewport;
    GtkWidget **split_rows;
    GtkWidget **split_titles;
    GtkWidget **split_deltas;
    GtkWidget **split_times;
} UrnSplits;
extern UrnComponentOps urn_splits_operations;

UrnComponent *urn_component_splits_new() {
    UrnSplits *self;

    self = malloc(sizeof(UrnSplits));
    if (!self) return NULL;
    self->base.ops = &urn_splits_operations;

    self->split_adjust = gtk_adjustment_new(0., 0., 0., 0., 0., 0.);

    self->split_scroller = gtk_scrolled_window_new(NULL, self->split_adjust);
    gtk_widget_set_vexpand(self->split_scroller, TRUE);
    gtk_widget_set_hexpand(self->split_scroller, TRUE);
    gtk_widget_show(self->split_scroller);
    gtk_widget_add_events(self->split_scroller, GDK_SCROLL_MASK);

    self->split_viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(self->split_scroller),
            self->split_viewport);
    gtk_widget_show(self->split_viewport);

    self->splits = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(self->splits, "splits");
    gtk_widget_set_hexpand(self->splits, TRUE);
    gtk_container_add(GTK_CONTAINER(self->split_viewport), self->splits);
    gtk_widget_show(self->splits);
    
    self->split_last = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(self->split_last, "split-last");
    gtk_widget_set_hexpand(self->split_last, TRUE);
    gtk_widget_show(self->split_last);

    self->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(self->container), self->split_scroller);
    gtk_container_add(GTK_CONTAINER(self->container), self->split_last);
    gtk_widget_show(self->container);
    return (UrnComponent *)self;
}

static void splits_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *splits_widget(UrnComponent *self) {
    return ((UrnSplits *)self)->container;
}

static void splits_trailer(UrnComponent *self_) {
    UrnSplits *self = (UrnSplits *)self_;
    int height, split_h, last = self->split_count - 1;
    double curr_scroll = gtk_adjustment_get_value(self->split_adjust);
    double scroll_max = gtk_adjustment_get_upper(self->split_adjust);
    double page_size = gtk_adjustment_get_page_size(self->split_adjust);
    g_object_ref(self->split_rows[last]);
    split_h = gtk_widget_get_allocated_height(self->split_titles[last]);
    height = gtk_widget_get_allocated_height(self->splits);
    if (gtk_widget_get_parent(self->split_rows[last]) == self->splits) {
        if (curr_scroll + page_size < scroll_max) {
            // move last split to split_last
            gtk_container_remove(GTK_CONTAINER(self->splits),
                                 self->split_rows[last]);
            gtk_container_add(GTK_CONTAINER(self->split_last),
                              self->split_rows[last]);
            gtk_widget_show(self->split_last);
        }
    } else {
        if (curr_scroll + page_size == scroll_max) {
            // move last split to split box
            gtk_container_remove(GTK_CONTAINER(self->split_last),
                                 self->split_rows[last]);
            gtk_container_add(GTK_CONTAINER(self->splits),
                              self->split_rows[last]);
            gtk_adjustment_set_upper(self->split_adjust,
                                     scroll_max + height);
            gtk_adjustment_set_value(self->split_adjust,
                                     curr_scroll + split_h);
            gtk_widget_hide(self->split_last);
        }
    }
    g_object_unref(self->split_rows[last]);
}

static void splits_show_game(UrnComponent *self_, urn_game *game,
        urn_timer *timer) {
    UrnSplits *self = (UrnSplits *)self_;
    char str[256];
    int i;
    self->split_count = game->split_count;
    self->split_rows = calloc(self->split_count, sizeof(GtkWidget *));
    self->split_titles = calloc(self->split_count, sizeof(GtkWidget *));
    self->split_deltas = calloc(self->split_count, sizeof(GtkWidget *));
    self->split_times = calloc(self->split_count, sizeof(GtkWidget *));

    for (i = 0; i < self->split_count; ++i) {
        self->split_rows[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        add_class(self->split_rows[i], "split");
        gtk_widget_set_hexpand(self->split_rows[i], TRUE);
        gtk_container_add(GTK_CONTAINER(self->splits),
                          self->split_rows[i]);
        
        self->split_titles[i] = gtk_label_new(game->split_titles[i]);
        add_class(self->split_titles[i], "split-title");
        gtk_widget_set_halign(self->split_titles[i], GTK_ALIGN_START);
        gtk_widget_set_hexpand(self->split_titles[i], TRUE);
        gtk_container_add(GTK_CONTAINER(self->split_rows[i]),
                          self->split_titles[i]);

        self->split_deltas[i] = gtk_label_new(NULL);
        add_class(self->split_deltas[i], "split-delta");
        gtk_widget_set_size_request(self->split_deltas[i], 1, -1);
        gtk_container_add(GTK_CONTAINER(self->split_rows[i]),
                          self->split_deltas[i]);

        self->split_times[i] = gtk_label_new(NULL);
        add_class(self->split_times[i], "split-time");
        gtk_widget_set_halign(self->split_times[i], GTK_ALIGN_END);
        gtk_container_add(GTK_CONTAINER(self->split_rows[i]),
                          self->split_times[i]);

        if (game->split_times[i]) {
            urn_split_string(str, game->split_times[i]);
            gtk_label_set_text(GTK_LABEL(self->split_times[i]), str);
        }

        if (game->split_titles[i]
            && strlen(game->split_titles[i])) {
            char *c = &str[12];
            strcpy(str, "split-title-");
            strcpy(c, game->split_titles[i]);
            do {
                if (!isalnum(*c)) {
                    *c = '-';
                } else {
                    *c = tolower(*c);
                }
            } while (*++c != '\0');
            add_class(self->split_rows[i], str);
        }

        gtk_widget_show_all(self->split_rows[i]);
    }

    gtk_widget_show(self->splits);
    splits_trailer(self_);
}

static void splits_clear_game(UrnComponent *self_) {
    UrnSplits *self = (UrnSplits *)self_;
    int i;
    gtk_widget_hide(self->splits);
    gtk_widget_hide(self->split_last);
    for (i = self->split_count - 1; i >= 0; --i) {
        gtk_container_remove(
                GTK_CONTAINER(gtk_widget_get_parent(self->split_rows[i])),
                self->split_rows[i]);
    }
    gtk_adjustment_set_value(self->split_adjust, 0);
    free(self->split_rows);
    free(self->split_titles);
    free(self->split_deltas);
    free(self->split_times);
    self->split_count = 0;
}

#define SHOW_DELTA_THRESHOLD (-30 * 1000000LL)
static void splits_draw(UrnComponent *self_, urn_game *game, urn_timer *timer) {
    UrnSplits *self = (UrnSplits *)self_;
    char str[256];
    int i;
    for (i = 0; i < self->split_count; ++i) {
        if (i == timer->curr_split
            && timer->start_time) {
            add_class(self->split_rows[i], "current-split");
        } else {
            remove_class(self->split_rows[i], "current-split");
        }
        
        remove_class(self->split_times[i], "time");
        remove_class(self->split_times[i], "done");
        gtk_label_set_text(GTK_LABEL(self->split_times[i]), "-");
        if (i < timer->curr_split) {
            add_class(self->split_times[i], "done");
            if (timer->split_times[i]) {
                add_class(self->split_times[i], "time");
                urn_split_string(str, timer->split_times[i]);
                gtk_label_set_text(GTK_LABEL(self->split_times[i]), str);
            }
        } else if (game->split_times[i]) {
            add_class(self->split_times[i], "time");
            urn_split_string(str, game->split_times[i]);
            gtk_label_set_text(GTK_LABEL(self->split_times[i]), str);
        }
        
        remove_class(self->split_deltas[i], "best-split");
        remove_class(self->split_deltas[i], "best-segment");
        remove_class(self->split_deltas[i], "behind");
        remove_class(self->split_deltas[i], "losing");
        remove_class(self->split_deltas[i], "delta");
        gtk_label_set_text(GTK_LABEL(self->split_deltas[i]), "");
        if (i < timer->curr_split
            || timer->split_deltas[i] >= SHOW_DELTA_THRESHOLD) {
            if (timer->split_info[i] & URN_INFO_BEST_SPLIT) {
                add_class(self->split_deltas[i], "best-split");
            }
            if (timer->split_info[i] & URN_INFO_BEST_SEGMENT) {
                add_class(self->split_deltas[i], "best-segment");
            }
            if (timer->split_info[i] & URN_INFO_BEHIND_TIME) {
                add_class(self->split_deltas[i], "behind");
                if (timer->split_info[i]
                    & URN_INFO_LOSING_TIME) {
                    add_class(self->split_deltas[i], "losing");
                }
            } else {
                remove_class(self->split_deltas[i], "behind");
                if (timer->split_info[i]
                    & URN_INFO_LOSING_TIME) {
                    add_class(self->split_deltas[i], "losing");
                }
            }
            if (timer->split_deltas[i]) {
                add_class(self->split_deltas[i], "delta");
                urn_delta_string(str, timer->split_deltas[i]);
                gtk_label_set_text(GTK_LABEL(self->split_deltas[i]), str);
            }
        }
    }

    // keep split sizes in sync
    if (self->split_count) {
        int width;
        int time_width = 0, delta_width = 0;
        for (i = 0; i < self->split_count; ++i) {
            width = gtk_widget_get_allocated_width(self->split_deltas[i]);
            if (width > delta_width) {
                delta_width = width;
            }
            width = gtk_widget_get_allocated_width(self->split_times[i]);
            if (width > time_width) {
                time_width = width;
            }
        }
        for (i = 0; i < self->split_count; ++i) {
            if (delta_width) {
                gtk_widget_set_size_request(
                    self->split_deltas[i], delta_width, -1);
            }
            if (time_width) {
                width = gtk_widget_get_allocated_width(
                    self->split_times[i]);
                gtk_widget_set_margin_start(self->split_times[i],
                    /*WINDOW_PAD*/ 8 * 2 + (time_width - width));
            }
        }
    }

    splits_trailer(self_);
}

static void splits_scroll_to_split(UrnComponent *self_, urn_timer *timer) {
    UrnSplits *self = (UrnSplits *)self_;
    int split_x, split_y;
    int split_h;
    int scroller_h;
    double curr_scroll;
    double min_scroll, max_scroll;
    int prev = timer->curr_split - 1;
    int curr = timer->curr_split;
    int next = timer->curr_split + 1;
    if (prev < 0) {
        prev = 0;
    }
    if (curr >= self->split_count) {
        curr = self->split_count - 1;
    }
    if (next >= self->split_count) {
        next = self->split_count - 1;
    }
    curr_scroll = gtk_adjustment_get_value(self->split_adjust);
    gtk_widget_translate_coordinates(
        self->split_titles[prev],
        self->split_viewport,
        0, 0, &split_x, &split_y);
    scroller_h = gtk_widget_get_allocated_height(self->split_scroller);
    split_h = gtk_widget_get_allocated_height(self->split_titles[prev]);
    if (curr != next && curr != prev) {
        split_h += gtk_widget_get_allocated_height(self->split_titles[curr]);
    }
    if (next != prev) {
        int h = gtk_widget_get_allocated_height(self->split_titles[next]);
        if (split_h + h < scroller_h) {
            split_h += h;
        }
    }
    min_scroll = split_y + curr_scroll - scroller_h + split_h;
    max_scroll = split_y + curr_scroll;
    if (curr_scroll > max_scroll) {
        gtk_adjustment_set_value(self->split_adjust, max_scroll);
    } else if (curr_scroll < min_scroll) {
        gtk_adjustment_set_value(self->split_adjust, min_scroll);
    }
}

void splits_start_split(UrnComponent *self, urn_timer *timer)
{
    splits_scroll_to_split(self, timer);
}

void splits_skip(UrnComponent *self, urn_timer *timer)
{
    splits_scroll_to_split(self, timer);
}

void splits_unsplit(UrnComponent *self, urn_timer *timer)
{
    splits_scroll_to_split(self, timer);
}

UrnComponentOps urn_splits_operations = {
    .delete = splits_delete,
    .widget = splits_widget,
    .show_game = splits_show_game,
    .clear_game = splits_clear_game,
    .draw = splits_draw,
    .start_split = splits_start_split,
    .skip = splits_skip,
    .unsplit = splits_unsplit
};
