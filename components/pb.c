#include "urn-component.h"

typedef struct _UrnPb {
    UrnComponent base;
    GtkWidget *container;
    GtkWidget *personal_best;
} UrnPb;
extern UrnComponentOps urn_pb_operations;

#define PERSONAL_BEST "Personal best"

UrnComponent *urn_component_pb_new() {
    UrnPb *self;
    GtkWidget *label;

    self = malloc(sizeof(UrnPb));
    if (!self) return NULL;
    self->base.ops = &urn_pb_operations;

    self->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(self->container, "footer"); /* hack */
    gtk_widget_show(self->container);

    label = gtk_label_new(PERSONAL_BEST);
    add_class(label, "personal-best-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_container_add(GTK_CONTAINER(self->container), label);
    gtk_widget_show(label);

    self->personal_best = gtk_label_new(NULL);
    add_class(self->personal_best, "personal-best");
    gtk_widget_set_halign(self->personal_best, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(self->container), self->personal_best);
    gtk_widget_show(self->personal_best);

    return (UrnComponent *)self;
}

static void pb_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *pb_widget(UrnComponent *self) {
    return ((UrnPb *)self)->container;
}

static void pb_show_game(UrnComponent *self_,
        urn_game *game, urn_timer *timer) {
    UrnPb *self = (UrnPb *)self_;
    char str[256];
    if (game->split_count && game->split_times[game->split_count - 1]) {
        if (game->split_times[game->split_count - 1]) {
            urn_time_string(
                str, game->split_times[game->split_count - 1]);
            gtk_label_set_text(GTK_LABEL(self->personal_best), str);
        }
    }

}

static void pb_clear_game(UrnComponent *self_) {
    UrnPb *self = (UrnPb *)self_;
    gtk_label_set_text(GTK_LABEL(self->personal_best), "");
}

static void pb_draw(UrnComponent *self_, urn_game *game,
        urn_timer *timer) {
    UrnPb *self = (UrnPb *)self_;
    char str[256];
    remove_class(self->personal_best, "time");
    gtk_label_set_text(GTK_LABEL(self->personal_best), "-");
    if (timer->curr_split == game->split_count
        && timer->split_times[game->split_count - 1]
        && (!game->split_times[game->split_count - 1]
            || (timer->split_times[game->split_count - 1]
                < game->split_times[game->split_count - 1]))) {
        add_class(self->personal_best, "time");
        urn_time_string(
            str, timer->split_times[game->split_count - 1]);
        gtk_label_set_text(GTK_LABEL(self->personal_best), str);
    } else if (game->split_times[game->split_count - 1]) {
        add_class(self->personal_best, "time");
        urn_time_string(
            str, game->split_times[game->split_count - 1]);
        gtk_label_set_text(GTK_LABEL(self->personal_best), str);
    }
}

UrnComponentOps urn_pb_operations = {
    .delete = pb_delete,
    .widget = pb_widget,
    .show_game = pb_show_game,
    .clear_game = pb_clear_game,
    .draw = pb_draw
};
