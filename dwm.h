#ifndef DWM_H
#define DWM_H

#include "drw.h"
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// clang-format off
/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            (C->tags & C->mon->tagset[C->mon->seltags])
#define ISHIDDEN(C)             (C->hidden && !C->mon->showhidden)
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw + gappx)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw + gappx)
#define TAGMASK                 ((1 << 9) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

/* enums */
enum { CurNormal, CurResizeTopLeft, CurResizeTopRight, CurResizeBottomLeft, CurResizeBottomRight, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel, SchemeHidden, SchemeNotSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkAttach, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
enum { AttachFront, AttachStack, AttachEnd, AttachModes };
// clang-format on

union Arg {
    int i;
    unsigned int ui;
    float f;
    const void* v;
};
typedef union Arg Arg;

typedef struct {
    unsigned int click;
    unsigned int mask;
    unsigned int button;
    void (*func)(const Arg* arg);
    const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
    char name[256];
    float mina, maxa;
    float cfact;
    int fx[10], fy[10], fw[10], fh[10];
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int bw, oldbw;
    unsigned int tags;
    int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
    int hidden;
    Client* next;
    Client* snext;
    Monitor* mon;
    Window win;
};

typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg*);
    const Arg arg;
} Key;

typedef struct {
    const char* symbol;
    void (*arrange)(Monitor*);
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
    char ltsymbol[16];
    float mfact;
    int nmaster;
    int num;
    int by; /* bar geometry */
    int mx, my, mw, mh; /* screen size */
    int wx, wy, ww, wh; /* window area  */
    unsigned int seltags;
    unsigned int tagset[2];
    int showbar;
    int topbar;
    int showhidden;
    Client* clients;
    Client* sel;
    Client* stack;
    Monitor* next;
    Window barwin;
    Pertag* pertag;
    int attachmode;
};

typedef struct {
    const char* window_class;
    const char* instance;
    const char* title;
    unsigned int tags;
    int isfloating;
    int monitor;
} Rule;

struct Pertag {
    unsigned int curtag, prevtag; /* current and previous tag */
    int nmasters[10]; /* number of windows in master area */
    float mfacts[10]; /* mfacts per tag */
    const Layout* layout[10];
};

/* function declarations */
void applyrules(Client* c);
int applysizehints(Client* c, int* x, int* y, int* w, int* h, int interact);
void arrange(Monitor* m);
void arrangemon(Monitor* m);
void attach(Client* c);
void attachstack(Client* c);
void buttonpress(XEvent* e);
void checkotherwm(void);
void cleanup(void);
void cleanupmon(Monitor* mon);
void clientmessage(XEvent* e);
void configure(Client* c);
void configurenotify(XEvent* e);
void configurerequest(XEvent* e);
Monitor* createmon(void);
void destroynotify(XEvent* e);
void detach(Client* c);
void detachstack(Client* c);
Monitor* dirtomon(int dir);
void enternotify(XEvent* e);
void expose(XEvent* e);
void focus(Client* c);
void focusin(XEvent* e);
void focusmon(const Arg* arg);
void focusstack(const Arg* arg);
int getrootptr(int* x, int* y);
long getstate(Window w);
int gettextprop(Window w, Atom atom, char* text, unsigned int size);
void grabbuttons(Client* c, int focused);
void grabkeys(void);
void incnmaster(const Arg* arg);
void keypress(XEvent* e);
void killclient(Client* c);
void killselected(const Arg* arg);
void closewindow(const Arg* arg);
void manage(Window w, XWindowAttributes* wa);
void mappingnotify(XEvent* e);
void maprequest(XEvent* e);
void motionnotify(XEvent* e);
void movemouse(const Arg* arg);
Client* nexttiled(Client* c);
void propertynotify(XEvent* e);
void quit(const Arg* arg);
Monitor* recttomon(int x, int y, int w, int h);
void resize(Client* c, int x, int y, int w, int h, int interact);
void resizeclient(Client* c, int x, int y, int w, int h);
void resizemouse(const Arg* arg);
void restack(Monitor* m);
void run(void);
void scan(void);
int sendevent(Client* c, Atom proto);
void sendmon(Client* c, Monitor* m);
void setattach(const Arg* arg);
void setclientstate(Client* c, long state);
void setfocus(Client* c);
void setfullscreen(Client* c, int fullscreen);
void setlayout(const Arg* arg);
void setcfact(const Arg* arg);
void setmfact(const Arg* arg);
void setup(void);
void seturgent(Client* c, int urg);
void showhide(Client* c);
void sigchld(int unused);
void spawn(const Arg* arg);
void tag(const Arg* arg);
void tagmon(const Arg* arg);
void togglefloating(const Arg* arg);
void togglefullscr(const Arg* arg);
void toggletag(const Arg* arg);
void toggleview(const Arg* arg);
void unfocus(Client* c, int setfocus);
void unmanage(Client* c, int destroyed);
void unmapnotify(XEvent* e);
void updateclientlist(void);
int updategeom(void);
void updatenumlockmask(void);
void updatesizehints(Client* c);
void updatestatus(void);
void updatetitle(Client* c);
void updatewindowtype(Client* c);
void updatewmhints(Client* c);
void view(const Arg* arg);
Client* wintoclient(Window w);
Monitor* wintomon(Window w);
void togglehidden(const Arg* arg);
void toggleshowhidden(const Arg* arg);
int xerror(Display* dpy, XErrorEvent* ee);
int xerrordummy(Display* dpy, XErrorEvent* ee);
int xerrorstart(Display* dpy, XErrorEvent* ee);
void zoom(const Arg* arg);
void movestack(const Arg* arg);

/* variables */
extern const char broken[];
extern char stext[256];
extern int screen;
extern int sw, sh; /* X display screen geometry width, height */
extern int bh, blw; /* bar geometry */
extern int lrpad; /* sum of left and right padding for text */
extern int (*xerrorxlib)(Display*, XErrorEvent*);
extern unsigned int numlockmask;
extern void (*handler[LASTEvent])(XEvent*);
extern Atom wmatom[WMLast], netatom[NetLast];
extern int running;
extern Cur* cursor[CurLast];
extern Clr** scheme;
extern Display* dpy;
extern Drw* drw;
extern Monitor *mons, *selmon;
extern Window root, wmcheckwin;

#endif
