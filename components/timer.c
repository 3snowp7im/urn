#include "urn-component.h"

typedef struct _UrnTimer {
    UrnComponent base;
    GtkWidget *time;
    GtkWidget *time_seconds;
    GtkWidget *time_millis;
} UrnTimer;
extern UrnComponentOps urn_timer_operations;

UrnComponent *urn_component_timer_new() {
    UrnTimer *self;
    GtkWidget *spacer;

    self = malloc(sizeof(UrnTimer));
    if (!self) return NULL;
    self->base.ops = &urn_timer_operations;

    self->time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(self->time, "timer");
    add_class(self->time, "time");
    gtk_widget_show(self->time);

    spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_container_add(GTK_CONTAINER(self->time), spacer);
    gtk_widget_show(spacer);

    self->time_seconds = gtk_label_new(NULL);
    add_class(self->time_seconds, "timer-seconds");
    gtk_widget_set_valign(self->time_seconds, GTK_ALIGN_BASELINE);
    gtk_container_add(GTK_CONTAINER(self->time), self->time_seconds);
    gtk_widget_show(self->time_seconds);

    spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(spacer, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(self->time), spacer);
    gtk_widget_show(spacer);

    self->time_millis = gtk_label_new(NULL);
    add_class(self->time_millis, "timer-millis");
    gtk_widget_set_valign(self->time_millis, GTK_ALIGN_BASELINE);
    gtk_container_add(GTK_CONTAINER(spacer), self->time_millis);
    gtk_widget_show(self->time_millis);


    return (UrnComponent *)self;
}

// Avoid collision with timer_delete of time.h
static void urn_timer_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *timer_widget(UrnComponent *self) {
    return ((UrnTimer *)self)->time;
}

static void timer_clear_game(UrnComponent *self_) {
    UrnTimer *self = (UrnTimer *)self_;
    gtk_label_set_text(GTK_LABEL(self->time_seconds), "");
    gtk_label_set_text(GTK_LABEL(self->time_millis), "");
    remove_class(self->time, "behind");
    remove_class(self->time, "losing");

}

static void timer_draw(UrnComponent *self_, urn_game *game, urn_timer *timer) {
    UrnTimer *self = (UrnTimer *)self_;
    char str[256], millis[256];
    int curr;

    curr = timer->curr_split;
    if (curr == game->split_count) {
        --curr;
    }

    remove_class(self->time, "delay");
    remove_class(self->time, "behind");
    remove_class(self->time, "losing");
    remove_class(self->time, "best-split");

    if (curr == game->split_count) {
        curr = game->split_count - 1;
    }
    if (timer->time <= 0) {
        add_class(self->time, "delay");
    } else {
        if (timer->curr_split == game->split_count
            && timer->split_info[curr]
               & URN_INFO_BEST_SPLIT) {
            add_class(self->time, "best-split");
        } else{
            if (timer->split_info[curr]
                & URN_INFO_BEHIND_TIME) {
                add_class(self->time, "behind");
            }
            if (timer->split_info[curr]
                & URN_INFO_LOSING_TIME) {
                add_class(self->time, "losing");
            }
        }
    }
    urn_time_millis_string(str, &millis[1], timer->time);
    millis[0] = '.';
    gtk_label_set_text(GTK_LABEL(self->time_seconds), str);
    gtk_label_set_text(GTK_LABEL(self->time_millis), millis);
}

UrnComponentOps urn_timer_operations = {
    .delete = urn_timer_delete,
    .widget = timer_widget,
    .clear_game = timer_clear_game,
    .draw = timer_draw
};
