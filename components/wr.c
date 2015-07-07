#include "urn-component.h"

typedef struct _UrnWr {
    UrnComponent base;
    GtkWidget *container;
    GtkWidget *world_record_label;
    GtkWidget *world_record;
} UrnWr;
extern UrnComponentOps urn_wr_operations;

#define WORLD_RECORD "World record"

UrnComponent *urn_component_wr_new() {
    UrnWr *self;

    self = malloc(sizeof(UrnWr));
    if (!self) return NULL;
    self->base.ops = &urn_wr_operations;

    self->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(self->container, "footer"); /* hack */
    gtk_widget_show(self->container);

    self->world_record_label = gtk_label_new(WORLD_RECORD);
    add_class(self->world_record_label, "world-record-label");
    gtk_container_add(GTK_CONTAINER(self->container), self->world_record_label);

    self->world_record = gtk_label_new(NULL);
    add_class(self->world_record, "world-record");
    add_class(self->world_record, "time");
    gtk_widget_set_halign(self->world_record, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(self->container), self->world_record);

    return (UrnComponent *)self;
}

static void wr_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *wr_widget(UrnComponent *self) {
    return ((UrnWr *)self)->container;
}

static void wr_show_game(UrnComponent *self_,
        urn_game *game, urn_timer *timer) {
    UrnWr *self = (UrnWr *)self_;
    gtk_widget_set_halign(self->world_record_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(self->world_record_label, TRUE);
    if (game->world_record) {
        char str[256];
        urn_time_string(str, game->world_record);
        gtk_label_set_text(GTK_LABEL(self->world_record), str);
        gtk_widget_show(self->world_record);
        gtk_widget_show(self->world_record_label);
    }
}

static void wr_clear_game(UrnComponent *self_) {
    UrnWr *self = (UrnWr *)self_;
    gtk_widget_hide(self->world_record_label);
    gtk_widget_hide(self->world_record);
}

static void wr_draw(UrnComponent *self_, urn_game *game,
        urn_timer *timer) {
    UrnWr *self = (UrnWr *)self_;
    char str[256];
    if (timer->curr_split == game->split_count
        && game->world_record) {
        if (timer->split_times[game->split_count - 1]
            && timer->split_times[game->split_count - 1]
            < game->world_record) {
            urn_time_string(str, timer->split_times[
                                game->split_count - 1]);
        } else {
            urn_time_string(str, game->world_record);
        }
        gtk_label_set_text(GTK_LABEL(self->world_record), str);
    }
}

UrnComponentOps urn_wr_operations = {
    .delete = wr_delete,
    .widget = wr_widget,
    .show_game = wr_show_game,
    .clear_game = wr_clear_game,
    .draw = wr_draw
};
