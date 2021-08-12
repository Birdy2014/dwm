#pragma once

typedef struct Monitor Monitor;
typedef union Arg Arg;

void drawbar(Monitor* m);
void drawbars(void);
void togglebar(const Arg* arg);
void updatebars(void);
void updatebarpos(Monitor* m);

void bar_closewindow(const Arg* arg);
void bar_focusclient(const Arg* arg);
