#include "urn-component.h"

typedef struct _UrnBestSum {
    UrnComponent base;
    GtkWidget *container;
    GtkWidget *sum_of_bests;
} UrnBestSum;
extern UrnComponentOps urn_best_sum_operations;

#define SUM_OF_BEST_SEGMENTS "Sum of best segments"

UrnComponent *urn_component_best_sum_new() {
    UrnBestSum *self;
    GtkWidget *label;

    self = malloc(sizeof(UrnBestSum));
    if (!self) return NULL;
    self->base.ops = &urn_best_sum_operations;

    self->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(self->container, "footer"); /* hack */
    gtk_widget_show(self->container);

    label = gtk_label_new(SUM_OF_BEST_SEGMENTS);
    add_class(label, "sum-of-bests-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_container_add(GTK_CONTAINER(self->container), label);
    gtk_widget_show(label);

    self->sum_of_bests = gtk_label_new(NULL);
    add_class(self->sum_of_bests, "sum-of-bests");
    gtk_widget_set_halign(self->sum_of_bests, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(self->container), self->sum_of_bests);
    gtk_widget_show(self->sum_of_bests);

    return (UrnComponent *)self;
}

static void best_sum_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *best_sum_widget(UrnComponent *self) {
    return ((UrnBestSum *)self)->container;
}

static void best_sum_show_game(UrnComponent *self_,
        urn_game *game, urn_timer *timer) {
    UrnBestSum *self = (UrnBestSum *)self_;
    char str[256];
    if (game->split_count && timer->sum_of_bests) {
        urn_time_string(str, timer->sum_of_bests);
        gtk_label_set_text(GTK_LABEL(self->sum_of_bests), str);
    }
}

static void best_sum_clear_game(UrnComponent *self_) {
    UrnBestSum *self = (UrnBestSum *)self_;
    gtk_label_set_text(GTK_LABEL(self->sum_of_bests), "");
}

static void best_sum_draw(UrnComponent *self_, urn_game *game,
        urn_timer *timer) {
    UrnBestSum *self = (UrnBestSum *)self_;
    char str[256];
    remove_class(self->sum_of_bests, "time");
    gtk_label_set_text(GTK_LABEL(self->sum_of_bests), "-");
    if (timer->sum_of_bests) {
        add_class(self->sum_of_bests, "time");
        urn_time_string(str, timer->sum_of_bests);
        gtk_label_set_text(GTK_LABEL(self->sum_of_bests), str);
    }
}

UrnComponentOps urn_best_sum_operations = {
    .delete = best_sum_delete,
    .widget = best_sum_widget,
    .show_game = best_sum_show_game,
    .clear_game = best_sum_clear_game,
    .draw = best_sum_draw
};
