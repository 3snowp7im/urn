#include "urn-component.h"

typedef struct _UrnTitle {
    UrnComponent base;
    GtkWidget *header;
    GtkWidget *title;
    GtkWidget *attempt_count;
} UrnTitle;
extern UrnComponentOps urn_title_operations; // defined at the end of the file

UrnComponent *urn_component_title_new() {
    UrnTitle *self;

    self = malloc(sizeof(UrnTitle));
    if (!self) return NULL;
    self->base.ops = &urn_title_operations;

    self->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(self->header, "header");
    gtk_widget_show(self->header);

    self->title = gtk_label_new(NULL);
    add_class(self->title, "title");
    gtk_label_set_justify(GTK_LABEL(self->title), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(self->title), TRUE);
    gtk_widget_set_hexpand(self->title, TRUE);
    gtk_container_add(GTK_CONTAINER(self->header), self->title);

    self->attempt_count = gtk_label_new(NULL);
    add_class(self->attempt_count, "attempt-count");
    gtk_widget_set_margin_start(self->attempt_count, 8);
    gtk_widget_set_valign(self->attempt_count, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(self->header), self->attempt_count);
    gtk_widget_show(self->attempt_count);

    return (UrnComponent *)self;
}

static void title_delete(UrnComponent *self) {
    free(self);
}

static GtkWidget *title_widget(UrnComponent *self) {
    return ((UrnTitle *)self)->header;
}

static void title_resize(UrnComponent *self_, int win_width, int win_height) {
    GdkRectangle rect;
    int attempt_count_width;
    int title_width;
    UrnTitle *self = (UrnTitle *)self_;

    gtk_widget_hide(self->title);
    gtk_widget_get_allocation(self->attempt_count, &rect);
    attempt_count_width = rect.width;
    title_width = win_width - attempt_count_width;
    rect.width = title_width;
    gtk_widget_show(self->title);
    gtk_widget_set_allocation(self->title, &rect);
}

static void title_show_game(UrnComponent *self_, urn_game *game,
        urn_timer *timer) {
    char str[64];
    UrnTitle *self = (UrnTitle *)self_;
    gtk_label_set_text(GTK_LABEL(self->title), game->title);
    sprintf(str, "#%d", game->attempt_count);
    gtk_label_set_text(GTK_LABEL(self->attempt_count), str);
}

static void title_draw(UrnComponent *self_, urn_game *game, urn_timer *timer) {
    char str[64];
    UrnTitle *self = (UrnTitle *)self_;
    sprintf(str, "#%d", game->attempt_count);
    gtk_label_set_text(GTK_LABEL(self->attempt_count), str);
}

UrnComponentOps urn_title_operations = {
    .delete = title_delete,
    .widget = title_widget,
    .resize = title_resize,
    .show_game = title_show_game,
    .draw = title_draw
};
