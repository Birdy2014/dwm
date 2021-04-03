#ifndef LAYOUTS_H
#define LAYOUTS_H

typedef struct Monitor Monitor;

void tile(Monitor *);
void centeredmaster(Monitor *m);
void centeredfloatingmaster(Monitor *m);
void dwindle(Monitor *mon);
void layout_float(Monitor *mon);
void monocle(Monitor *m);

#endif
