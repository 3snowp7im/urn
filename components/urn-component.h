#ifndef __urn_component_h__
#define __urn_component_h__

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "../urn.h"

typedef struct _UrnComponent UrnComponent;
typedef struct _UrnComponentOps UrnComponentOps;
typedef struct _UrnComponentAvailable UrnComponentAvailable;

struct _UrnComponent {
    UrnComponentOps *ops;
};

struct _UrnComponentOps {
    void (*delete)(UrnComponent *self);
    GtkWidget *(*widget)(UrnComponent *self);

    void (*resize)(UrnComponent *self, int win_width, int win_height);
    void (*show_game)(UrnComponent *self, urn_game *game, urn_timer *timer);
    void (*clear_game)(UrnComponent *self);
    void (*draw)(UrnComponent *self, urn_game *game, urn_timer *timer);

    void (*start_split)(UrnComponent *self, urn_timer *timer);
    void (*skip)(UrnComponent *self, urn_timer *timer);
    void (*unsplit)(UrnComponent *self, urn_timer *timer);
    void (*stop_reset)(UrnComponent *self, urn_timer *timer);
    void (*cancel_run)(UrnComponent *self, urn_timer *timer);
};

struct _UrnComponentAvailable {
    char *name;
    UrnComponent *(*new)();
};

// A NULL-terminated array of all available components
extern UrnComponentAvailable urn_components[];

// Utility functions
void add_class(GtkWidget *widget, const char *class);

void remove_class(GtkWidget *widget, const char *class);

#endif
