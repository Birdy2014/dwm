/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */

#include "dwm.h"
#include "drw.h"
#include "layouts.h"
#include "util.h"
#include "bar.h"

const char broken[] = "broken";
char stext[256];
int scanner;
int screen;
int sw, sh; /* X display screen geometry width, height */
int bh, blw = 0; /* bar geometry */
int lrpad; /* sum of left and right padding for text */
int (*xerrorxlib)(Display*, XErrorEvent*);
unsigned int numlockmask            = 0;
void (*handler[LASTEvent])(XEvent*) = {
    [ButtonPress]      = buttonpress,
    [ClientMessage]    = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify]  = configurenotify,
    [DestroyNotify]    = destroynotify,
    [EnterNotify]      = enternotify,
    [Expose]           = expose,
    [FocusIn]          = focusin,
    [KeyPress]         = keypress,
    [MappingNotify]    = mappingnotify,
    [MapRequest]       = maprequest,
    [MotionNotify]     = motionnotify,
    [PropertyNotify]   = propertynotify,
    [UnmapNotify]      = unmapnotify
};
Atom wmatom[WMLast], netatom[NetLast];
int running = 1;
Cur* cursor[CurLast];
Clr** scheme;
Display* dpy;
Drw* drw;
Monitor *mons, *selmon;
Window root, wmcheckwin;

/* configuration, allows nested code to access above variables */
#include "config.h"

/* function implementations */
void applyrules(Client* c) {
    const char* class, *instance;
    unsigned int i;
    const Rule* r;
    Monitor* m;
    XClassHint ch = { NULL, NULL };

    /* rule matching */
    c->isfloating = 0;
    c->tags       = 0;
    XGetClassHint(dpy, c->win, &ch);
    class    = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name ? ch.res_name : broken;

    for (i = 0; i < nrules; i++) {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
            && (!r->window_class || strstr(class, r->window_class))
            && (!r->instance || strstr(instance, r->instance))) {
            c->isfloating = r->isfloating;
            c->tags |= r->tags;
            for (m = mons; m && m->num != r->monitor; m = m->next)
                ;
            if (m)
                c->mon = m;
        }
    }
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
    c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : (c->mon->tagset[c->mon->seltags] ? c->mon->tagset[c->mon->seltags] : 1);
}

int applysizehints(Client* c, int* x, int* y, int* w, int* h, int interact) {
    int baseismin;
    Monitor* m = c->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if (interact) {
        if (*x > sw)
            *x = sw - WIDTH(c);
        if (*y > sh)
            *y = sh - HEIGHT(c);
        if (*x + *w + 2 * c->bw < 0)
            *x = 0;
        if (*y + *h + 2 * c->bw < 0)
            *y = 0;
    } else {
        if (*x >= m->wx + m->ww)
            *x = m->wx + m->ww - WIDTH(c);
        if (*y >= m->wy + m->wh)
            *y = m->wy + m->wh - HEIGHT(c);
        if (*x + *w + 2 * c->bw <= m->wx)
            *x = m->wx;
        if (*y + *h + 2 * c->bw <= m->wy)
            *y = m->wy;
    }
    if (*h < bh)
        *h = bh;
    if (*w < bh)
        *w = bh;
    if (resizehints || c->isfloating || c->mon->pertag->layout[c->mon->pertag->curtag]->arrange == layout_float) {
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = c->basew == c->minw && c->baseh == c->minh;
        if (!baseismin) { /* temporarily remove base dimensions */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for aspect limits */
        if (c->mina > 0 && c->maxa > 0) {
            if (c->maxa < (float)*w / *h)
                *w = *h * c->maxa + 0.5;
            else if (c->mina < (float)*h / *w)
                *h = *w * c->mina + 0.5;
        }
        if (baseismin) { /* increment calculation requires this */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for increment value */
        if (c->incw)
            *w -= *w % c->incw;
        if (c->inch)
            *h -= *h % c->inch;
        /* restore base dimensions */
        *w = MAX(*w + c->basew, c->minw);
        *h = MAX(*h + c->baseh, c->minh);
        if (c->maxw)
            *w = MIN(*w, c->maxw);
        if (c->maxh)
            *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void arrange(Monitor* m) {
    if (m)
        showhide(m->stack);
    else
        for (m = mons; m; m = m->next)
            showhide(m->stack);
    if (m) {
        arrangemon(m);
        restack(m);
    } else
        for (m = mons; m; m = m->next)
            arrangemon(m);
}

void arrangemon(Monitor* m) {
    strncpy(m->ltsymbol, m->pertag->layout[m->pertag->curtag]->symbol, sizeof m->ltsymbol);
    if (m->pertag->layout[m->pertag->curtag]->arrange)
        m->pertag->layout[m->pertag->curtag]->arrange(m);
}

void attach(Client* c) {
    Client* prev;
    int i;
    int attachmode = nexttiled(selmon->clients) ? selmon->attachmode : AttachFront;
    switch (attachmode) {
    case AttachFront:
        c->next         = c->mon->clients;
        c->mon->clients = c;
        break;
    case AttachStack:
        i = 1;
        for (prev = nexttiled(selmon->clients); prev->next && i < selmon->nmaster; prev = nexttiled(prev->next)) {
            i++;
        }
        c->next    = prev->next;
        prev->next = c;
        break;
    case AttachEnd:
        for (prev = nexttiled(selmon->clients); nexttiled(prev->next); prev = nexttiled(prev->next))
            ;
        c->next    = prev->next;
        prev->next = c;
        break;
    }
}

void attachstack(Client* c) {
    c->snext      = c->mon->stack;
    c->mon->stack = c;
}

void buttonpress(XEvent* e) {
    unsigned int i, x, click;
    Arg arg = { 0 };
    Client* c;
    Monitor* m;
    XButtonPressedEvent* ev = &e->xbutton;
    unsigned int attachw    = TEXTW(attachsymbols[selmon->attachmode]);

    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    if (ev->window == selmon->barwin) {
        i = x = 0;
        do
            x += TEXTW(tags[i]);
        while (ev->x >= x && ++i < ntags);
        if (i < ntags) {
            click  = ClkTagBar;
            arg.ui = 1 << i;
        } else if (ev->x < x + blw)
            click = ClkLtSymbol;
        else if (ev->x < x + blw + attachw)
            click = ClkAttach;
        else if (ev->x > selmon->ww - TEXTW(stext))
            click = ClkStatusText;
        else {
            x += blw + attachw;
            c = m->clients;

            if (c) {
                do {
                    if (!ISVISIBLE(c))
                        continue;
                    else
                        x += TEXTW(c->name) + TEXTW(separator);
                } while (ev->x > x && (c = c->next));

                click = ClkWinTitle;
                arg.v = c;
            }
        }
    } else if ((c = wintoclient(ev->window))) {
        focus(c);
        restack(selmon);
        if (c->isfloating || selmon->pertag->layout[selmon->pertag->curtag]->arrange == &layout_float)
            XRaiseWindow(dpy, c->win);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < nbuttons; i++)
        if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
            && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
            buttons[i].func((click == ClkTagBar || click == ClkWinTitle) && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void checkotherwm(void) {
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XSync(dpy, False);
}

void cleanup(void) {
    Arg a = { .ui = ~0 };
    Monitor* m;
    size_t i;

    view(&a);
    for (m = mons; m; m = m->next)
        while (m->stack)
            unmanage(m->stack, 0);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons)
        cleanupmon(mons);
    for (i = 0; i < CurLast; i++)
        drw_cur_free(drw, cursor[i]);
    for (i = 0; i < ncolors; i++)
        free(scheme[i]);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void cleanupmon(Monitor* mon) {
    Monitor* m;

    if (mon == mons)
        mons = mons->next;
    else {
        for (m = mons; m && m->next != mon; m = m->next)
            ;
        m->next = mon->next;
    }
    XUnmapWindow(dpy, mon->barwin);
    XDestroyWindow(dpy, mon->barwin);
    free(mon);
}

void clientmessage(XEvent* e) {
    XClientMessageEvent* cme = &e->xclient;
    Client* c                = wintoclient(cme->window);

    if (!c)
        return;
    if (cme->message_type == netatom[NetWMState]) {
        if (cme->data.l[1] == netatom[NetWMFullscreen]
            || cme->data.l[2] == netatom[NetWMFullscreen])
            setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
                                 || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
    } else if (cme->message_type == netatom[NetActiveWindow]) {
        if (!ISVISIBLE(c) && !c->isurgent)
            seturgent(c, 1);
    }
}

void configure(Client* c) {
    XConfigureEvent ce;

    ce.type              = ConfigureNotify;
    ce.display           = dpy;
    ce.event             = c->win;
    ce.window            = c->win;
    ce.x                 = c->x;
    ce.y                 = c->y;
    ce.width             = c->w;
    ce.height            = c->h;
    ce.border_width      = c->bw;
    ce.above             = None;
    ce.override_redirect = False;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent*)&ce);
}

void configurenotify(XEvent* e) {
    Monitor* m;
    Client* c;
    XConfigureEvent* ev = &e->xconfigure;
    int dirty;

    /* TODO: updategeom handling sucks, needs to be simplified */
    if (ev->window == root) {
        dirty = (sw != ev->width || sh != ev->height);
        sw    = ev->width;
        sh    = ev->height;
        if (updategeom() || dirty) {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next) {
                for (c = m->clients; c; c = c->next)
                    if (c->isfullscreen)
                        resizeclient(c, m->mx, m->my, m->mw, m->mh);
                XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
            }
            focus(NULL);
            arrange(NULL);
        }
    }
}

void configurerequest(XEvent* e) {
    Client* c;
    Monitor* m;
    XConfigureRequestEvent* ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if ((c = wintoclient(ev->window))) {
        if (ev->value_mask & CWBorderWidth)
            c->bw = ev->border_width;
        else if (c->isfloating || !selmon->pertag->layout[selmon->pertag->curtag]->arrange) {
            m = c->mon;
            if (ev->value_mask & CWX) {
                c->oldx = c->x;
                c->x    = m->mx + ev->x;
            }
            if (ev->value_mask & CWY) {
                c->oldy = c->y;
                c->y    = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth) {
                c->oldw = c->w;
                c->w    = ev->width;
            }
            if (ev->value_mask & CWHeight) {
                c->oldh = c->h;
                c->h    = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
                c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
            if ((c->y + c->h) > m->my + m->mh && c->isfloating)
                c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
            if ((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                configure(c);
            if (ISVISIBLE(c))
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        } else
            configure(c);
    } else {
        wc.x            = ev->x;
        wc.y            = ev->y;
        wc.width        = ev->width;
        wc.height       = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling      = ev->above;
        wc.stack_mode   = ev->detail;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
    XSync(dpy, False);
}

Monitor*
createmon(void) {
    Monitor* m;
    unsigned int i;

    m            = ecalloc(1, sizeof(Monitor));
    m->tagset[0] = m->tagset[1] = 1;
    m->mfact                    = mfact;
    m->nmaster                  = nmaster;
    m->showbar                  = showbar;
    m->topbar                   = topbar;
    strncpy(m->ltsymbol, layouts[deflt[0]].symbol, sizeof m->ltsymbol);
    m->pertag         = ecalloc(1, sizeof(Pertag));
    m->pertag->curtag = m->pertag->prevtag = 1;

    for (i = 0; i <= ntags; i++) {
        m->pertag->nmasters[i] = m->nmaster;
        m->pertag->mfacts[i]   = m->mfact;

        m->pertag->layout[i] = &layouts[deflt[i]];
    }

    m->attachmode = AttachEnd;

    return m;
}

void destroynotify(XEvent* e) {
    Client* c;
    XDestroyWindowEvent* ev = &e->xdestroywindow;

    if ((c = wintoclient(ev->window)))
        unmanage(c, 1);
}

void detach(Client* c) {
    Client** tc;

    for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next)
        ;
    *tc = c->next;
}

void detachstack(Client* c) {
    Client **tc, *t;

    for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext)
        ;
    *tc = c->snext;

    if (c == c->mon->sel) {
        for (t = c->mon->stack; t && (!ISVISIBLE(t) || ISHIDDEN(t)); t = t->snext)
            ;
        c->mon->sel = t;
    }
}

Monitor*
dirtomon(int dir) {
    Monitor* m = NULL;

    if (dir > 0) {
        if (!(m = selmon->next))
            m = mons;
    } else if (selmon == mons)
        for (m = mons; m->next; m = m->next)
            ;
    else
        for (m = mons; m->next != selmon; m = m->next)
            ;
    return m;
}

void enternotify(XEvent* e) {
    Client* c;
    Monitor* m;
    XCrossingEvent* ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
        return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
    } else if (!c || c == selmon->sel)
        return;
    focus(c);
}

void expose(XEvent* e) {
    Monitor* m;
    XExposeEvent* ev = &e->xexpose;

    if (ev->count == 0 && (m = wintomon(ev->window)))
        drawbar(m);
}

void focus(Client* c) {
    if (!c || !ISVISIBLE(c) || ISHIDDEN(c))
        for (c = selmon->stack; c && (!ISVISIBLE(c) || ISHIDDEN(c)); c = c->snext)
            ;
    if (selmon->sel && selmon->sel != c)
        unfocus(selmon->sel, 0);
    if (c) {
        if (c->mon != selmon)
            selmon = c->mon;
        detachstack(c);
        attachstack(c);
        grabbuttons(c, 1);
        XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
        setfocus(c);
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
    drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent* e) {
    XFocusChangeEvent* ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
}

void focusmon(const Arg* arg) {
    Monitor* m;

    if (!mons->next)
        return;
    if ((m = dirtomon(arg->i)) == selmon)
        return;
    unfocus(selmon->sel, 0);
    selmon = m;
    focus(NULL);
}

void focusstack(const Arg* arg) {
    Client *c = NULL, *i;

    if (!selmon->sel)
        return;
    if (arg->i > 0) {
        for (c = selmon->sel->next; c && (!ISVISIBLE(c) || ISHIDDEN(c)); c = c->next)
            ;
        if (!c)
            for (c = selmon->clients; c && (!ISVISIBLE(c) || ISHIDDEN(c)); c = c->next)
                ;
    } else {
        for (i = selmon->clients; i != selmon->sel; i = i->next)
            if (ISVISIBLE(i) && !ISHIDDEN(i))
                c = i;
        if (!c)
            for (; i; i = i->next)
                if (ISVISIBLE(i) && !ISHIDDEN(i))
                    c = i;
    }
    if (c) {
        focus(c);
        restack(selmon);
        if (c->isfloating || selmon->pertag->layout[selmon->pertag->curtag]->arrange == &layout_float)
            XRaiseWindow(dpy, c->win);
    }
}

Atom getatomprop(Client* c, Atom prop) {
    int di;
    unsigned long dl;
    unsigned char* p = NULL;
    Atom da, atom = None;

    if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
            &da, &di, &dl, &dl, &p)
            == Success
        && p) {
        atom = *(Atom*)p;
        XFree(p);
    }
    return atom;
}

int getrootptr(int* x, int* y) {
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
    int format;
    long result      = -1;
    unsigned char* p = NULL;
    unsigned long n, extra;
    Atom real;

    if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
            &real, &format, &n, &extra, (unsigned char**)&p)
        != Success)
        return -1;
    if (n != 0)
        result = *p;
    XFree(p);
    return result;
}

int gettextprop(Window w, Atom atom, char* text, unsigned int size) {
    char** list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0)
        return 0;
    text[0] = '\0';
    if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
        return 0;
    if (name.encoding == XA_STRING)
        strncpy(text, (char*)name.value, size - 1);
    else {
        if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
            strncpy(text, *list, size - 1);
            XFreeStringList(list);
        }
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

void grabbuttons(Client* c, int focused) {
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | LockMask };
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        if (!focused)
            XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
                BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        for (i = 0; i < nbuttons; i++)
            if (buttons[i].click == ClkClientWin)
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabButton(dpy, buttons[i].button,
                        buttons[i].mask | modifiers[j],
                        c->win, False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
    }
}

void grabkeys(void) {
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | LockMask };
        KeyCode code;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for (i = 0; i < nkeys; i++)
            if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
                        True, GrabModeAsync, GrabModeAsync);
    }
}

void incnmaster(const Arg* arg) {
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
    arrange(selmon);
}

static int
isuniquegeom(XineramaScreenInfo* unique, size_t n, XineramaScreenInfo* info) {
    while (n--)
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
            && unique[n].width == info->width && unique[n].height == info->height)
            return 0;
    return 1;
}

void keypress(XEvent* e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent* ev;

    ev     = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
    for (i = 0; i < nkeys; i++)
        if (keysym == keys[i].keysym
            && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
            && keys[i].func)
            keys[i].func(&(keys[i].arg));
}

void killclient(Client* c) {
    if (!sendevent(c, wmatom[WMDelete])) {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, c->win);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
}

void killselected(const Arg* arg) {
    if (!selmon->sel)
        return;
    killclient(selmon->sel);
}

void closewindow(const Arg* arg) {
    if (!selmon->sel)
        return;
    unsigned int newtags = selmon->sel->tags ^ selmon->tagset[selmon->seltags];
    if (newtags != 0) {
        selmon->sel->tags = newtags;
        focus(NULL);
        arrange(selmon);
    } else {
        killclient(selmon->sel);
    }
}

void manage(Window w, XWindowAttributes* wa) {
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    c      = ecalloc(1, sizeof(Client));
    c->win = w;
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw       = wa->border_width;
    c->cfact       = 1.0;

    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
        c->mon  = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = selmon;
        applyrules(c);
    }

    if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
        c->x = c->mon->mx + c->mon->mw - WIDTH(c);
    if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
        c->y = c->mon->my + c->mon->mh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->mx);
    /* only fix client y-offset, if the client center might cover the bar */
    c->y  = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx) && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
    c->bw = borderpx;

    for (unsigned int i = 0; i < 10; i++) {
        c->fx[i] = c->x;
        c->fy[i] = c->y;
        c->fw[i] = c->w;
        c->fh[i] = c->h;
    }

    wc.border_width = c->bw;
    XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);
    grabbuttons(c, 0);
    if (!c->isfloating)
        c->isfloating = c->oldstate = trans != None || c->isfixed;
    if (c->isfloating)
        XRaiseWindow(dpy, c->win);
    attach(c);
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
        (unsigned char*)&(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
    setclientstate(c, NormalState);
    if (c->mon == selmon)
        unfocus(selmon->sel, 0);
    c->mon->sel = c;
    arrange(c->mon);
    XMapWindow(dpy, c->win);
    focus(NULL);
}

void mappingnotify(XEvent* e) {
    XMappingEvent* ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if (ev->request == MappingKeyboard)
        grabkeys();
}

void maprequest(XEvent* e) {
    static XWindowAttributes wa;
    XMapRequestEvent* ev = &e->xmaprequest;

    if (!XGetWindowAttributes(dpy, ev->window, &wa))
        return;
    if (wa.override_redirect)
        return;
    if (!wintoclient(ev->window))
        manage(ev->window, &wa);
}

void motionnotify(XEvent* e) {
    static Monitor* mon = NULL;
    Monitor* m;
    XMotionEvent* ev = &e->xmotion;

    if (ev->window != root)
        return;
    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    mon = m;
}

void movemouse(const Arg* arg) {
    int x, y, ocx, ocy, nx, ny;
    Client* c;
    Monitor* m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
        return;
    restack(selmon);
    XRaiseWindow(dpy, c->win);
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
            None, cursor[CurMove]->cursor, CurrentTime)
        != GrabSuccess)
        return;
    if (!getrootptr(&x, &y))
        return;
    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type) {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if ((ev.xmotion.time - lasttime) <= (1000 / 60))
                continue;
            lasttime = ev.xmotion.time;

            nx = ocx + (ev.xmotion.x - x);
            ny = ocy + (ev.xmotion.y - y);
            if (abs(selmon->wx - nx) < snap)
                nx = selmon->wx;
            else if ((selmon->wx + selmon->ww + gappx) - (nx + WIDTH(c)) < snap)
                nx = selmon->wx + selmon->ww + gappx - WIDTH(c);
            if (abs(selmon->wy - ny) < snap)
                ny = selmon->wy;
            else if ((selmon->wy + selmon->wh + gappx) - (ny + HEIGHT(c)) < snap)
                ny = selmon->wy + selmon->wh + gappx - HEIGHT(c);
            if (!c->isfloating && selmon->pertag->layout[selmon->pertag->curtag]->arrange != layout_float
                && (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
                togglefloating(NULL);
            if (selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float || c->isfloating)
                resize(c, nx, ny, c->w, c->h, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        int x = c->x;
        int y = c->y;
        sendmon(c, m);
        selmon = m;
        if (selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float || c->isfloating)
            resize(c, x, y, c->w, c->h, 1); // Reset fx and fy
        focus(NULL);
    }
}

Client*
nexttiled(Client* c) {
    for (; c && (c->isfloating || !ISVISIBLE(c) || ISHIDDEN(c)); c = c->next)
        ;
    return c;
}

void propertynotify(XEvent* e) {
    Client* c;
    Window trans;
    XPropertyEvent* ev = &e->xproperty;

    if ((ev->window == root) && (ev->atom == XA_WM_NAME))
        updatestatus();
    else if (ev->state == PropertyDelete)
        return; /* ignore */
    else if ((c = wintoclient(ev->window))) {
        switch (ev->atom) {
        default:
            break;
        case XA_WM_TRANSIENT_FOR:
            if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) && (c->isfloating = (wintoclient(trans)) != NULL))
                arrange(c->mon);
            if (c->isfloating && c == selmon->sel)
                XRaiseWindow(dpy, c->win);
            break;
        case XA_WM_NORMAL_HINTS:
            updatesizehints(c);
            break;
        case XA_WM_HINTS:
            updatewmhints(c);
            drawbars();
            break;
        }
        if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
            updatetitle(c);
            if (c == c->mon->sel)
                drawbar(c->mon);
        }
        if (ev->atom == netatom[NetWMWindowType])
            updatewindowtype(c);
    }
}

void quit(const Arg* arg) {
    running = 0;
}

Monitor*
recttomon(int x, int y, int w, int h) {
    Monitor *m, *r = selmon;
    int a, area    = 0;

    for (m = mons; m; m = m->next)
        if ((a = INTERSECT(x, y, w, h, m)) > area) {
            area = a;
            r    = m;
        }
    return r;
}

void resize(Client* c, int x, int y, int w, int h, int interact) {
    if (applysizehints(c, &x, &y, &w, &h, interact))
        resizeclient(c, x, y, w, h);
}

void resizeclient(Client* c, int x, int y, int w, int h) {
    XWindowChanges wc;
    unsigned int n;
    unsigned int gapoffset;
    unsigned int gapincr;
    Client* nbc;

    wc.border_width = c->bw;

    /* Get number of clients for the selected monitor */
    for (n = 0, nbc = nexttiled(c->mon->clients); nbc; nbc = nexttiled(nbc->next), n++)
        ;

    /* Do nothing if layout is floating */
    if (c->isfloating || c->mon->pertag->layout[c->mon->pertag->curtag]->arrange == layout_float) {
        gapincr = gapoffset = 0;
    } else {
        /* Remove border and gap if layout is monocle or only one client */
        if (c->mon->pertag->layout[c->mon->pertag->curtag]->arrange == monocle || n <= 1) {
            gapoffset       = 0;
            gapincr         = -2 * borderpx;
            wc.border_width = 0;
        } else {
            gapoffset = gappx;
            gapincr   = 2 * gappx;
        }
    }

    c->oldx = c->x;
    c->x = wc.x = x + gapoffset;
    c->oldy     = c->y;
    c->y = wc.y = y + gapoffset;
    c->oldw     = c->w;
    c->w = wc.width = w - gapincr;
    c->oldh         = c->h;
    c->h = wc.height = h - gapincr;

    if (selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float) {
        c->fx[selmon->pertag->curtag] = c->x;
        c->fy[selmon->pertag->curtag] = c->y;
        c->fw[selmon->pertag->curtag] = c->w;
        c->fh[selmon->pertag->curtag] = c->h;
    }

    XConfigureWindow(dpy, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
    configure(c);
    XSync(dpy, False);
}

void resizemouse(const Arg* arg) {
    int ocx, ocy, nw, nh;
    int ocx2, ocy2, nx, ny;
    Client* c;
    Monitor* m;
    XEvent ev;
    int horizcorner, vertcorner;
    int di;
    unsigned int dui;
    Window dummy;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
        return;
    restack(selmon);
    XRaiseWindow(dpy, c->win);
    ocx  = c->x;
    ocy  = c->y;
    ocx2 = c->x + c->w;
    ocy2 = c->y + c->h;
    if (!XQueryPointer(dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
        return;
    horizcorner  = nx < c->w / 2;
    vertcorner   = ny < c->h / 2;
    int cursorNr = CurResizeBottomLeft;
    if (horizcorner && vertcorner)
        cursorNr = CurResizeTopLeft;
    else if (!horizcorner && vertcorner)
        cursorNr = CurResizeTopRight;
    else if (horizcorner && !vertcorner)
        cursorNr = CurResizeBottomLeft;
    else if (!horizcorner && !vertcorner)
        cursorNr = CurResizeBottomRight;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
            None, cursor[cursorNr]->cursor, CurrentTime)
        != GrabSuccess)
        return;
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
        horizcorner ? (-c->bw) : (c->w + c->bw - 1),
        vertcorner ? (-c->bw) : (c->h + c->bw - 1));

    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type) {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if ((ev.xmotion.time - lasttime) <= (1000 / 60))
                continue;
            lasttime = ev.xmotion.time;

            nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
            nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
            nx = horizcorner ? ev.xmotion.x : c->x;
            ny = vertcorner ? ev.xmotion.y : c->y;
            nw = MAX(horizcorner ? (ocx2 - nx) : (ev.xmotion.x - ocx - 2 * c->bw + 1), 1);
            nh = MAX(vertcorner ? (ocy2 - ny) : (ev.xmotion.y - ocy - 2 * c->bw + 1), 1);

            if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
                && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh) {
                if (!c->isfloating && selmon->pertag->layout[selmon->pertag->curtag]->arrange != layout_float
                    && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
                    togglefloating(NULL);
            }
            if (selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float || c->isfloating)
                resize(c, nx, ny, nw, nh, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
        horizcorner ? (-c->bw) : (c->w + c->bw - 1),
        vertcorner ? (-c->bw) : (c->h + c->bw - 1));
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
        ;
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}

void restack(Monitor* m) {
    Client* c;
    XEvent ev;
    XWindowChanges wc;

    drawbar(m);
    if (!m->sel)
        return;
    if (m->pertag->layout[m->pertag->curtag]->arrange != &layout_float) {
        wc.stack_mode = Below;
        wc.sibling    = m->barwin;
        for (c = m->stack; c; c = c->snext)
            if (!c->isfloating && ISVISIBLE(c) && !ISHIDDEN(c)) {
                XConfigureWindow(dpy, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
    }
    XSync(dpy, False);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
        ;
}

void run(void) {
    XEvent ev;
    /* main event loop */
    XSync(dpy, False);
    while (running && !XNextEvent(dpy, &ev))
        if (handler[ev.type])
            handler[ev.type](&ev); /* call handler */
}

void scan(void) {
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
        for (i = 0; i < num; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
                continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
                manage(wins[i], &wa);
        }
        for (i = 0; i < num; i++) { /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa))
                continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
                manage(wins[i], &wa);
        }
        if (wins)
            XFree(wins);
    }
}

void sendmon(Client* c, Monitor* m) {
    if (c->mon == m)
        return;
    for (unsigned int tag = 0; tag < 9; tag++) {
        c->fx[tag] = c->fx[tag] - c->mon->mx + m->mx;
        c->fy[tag] = c->fy[tag] - c->mon->my + m->my;
    }
    unfocus(c, 1);
    detach(c);
    detachstack(c);
    c->mon  = m;
    c->tags = (m->tagset[m->seltags] ? m->tagset[m->seltags] : 1); /* assign tags of target monitor */
    attach(c);
    attachstack(c);
    focus(c);
    arrange(NULL);
}

void setattach(const Arg* arg) {
    int attachmode = selmon->attachmode + 1;
    if (arg->i >= 0)
        attachmode = arg->i;
    selmon->attachmode = attachmode % AttachModes;
    drawbars();
}

void setclientstate(Client* c, long state) {
    long data[] = { state, None };

    XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
        PropModeReplace, (unsigned char*)data, 2);
}

int sendevent(Client* c, Atom proto) {
    int n;
    Atom* protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
        while (!exists && n--)
            exists = protocols[n] == proto;
        XFree(protocols);
    }
    if (exists) {
        ev.type                 = ClientMessage;
        ev.xclient.window       = c->win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format       = 32;
        ev.xclient.data.l[0]    = proto;
        ev.xclient.data.l[1]    = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &ev);
    }
    return exists;
}

void setfocus(Client* c) {
    if (!c->neverfocus) {
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char*)&(c->win), 1);
    }
    setclientstate(c, NormalState);
    sendevent(c, wmatom[WMTakeFocus]);
}

void setfullscreen(Client* c, int fullscreen) {
    if (fullscreen && !c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
            PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
        c->isfullscreen = 1;
        c->oldstate     = c->isfloating;
        c->oldbw        = c->bw;
        c->bw           = 0;
        c->isfloating   = 1;
        resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
        XRaiseWindow(dpy, c->win);
    } else if (!fullscreen && c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
            PropModeReplace, (unsigned char*)0, 0);
        c->isfullscreen = 0;
        c->isfloating   = c->oldstate;
        c->bw           = c->oldbw;
        c->x            = c->oldx;
        c->y            = c->oldy;
        c->w            = c->oldw;
        c->h            = c->oldh;
        resizeclient(c, c->x, c->y, c->w, c->h);
        arrange(c->mon);
    }
}

void setlayout(const Arg* arg) {
    if (!arg || !arg->v || arg->v != selmon->pertag->layout[selmon->pertag->curtag])
        selmon->pertag->layout[selmon->pertag->curtag] = &layouts[deflt[selmon->pertag->curtag]];
    if (arg && arg->v)
        selmon->pertag->layout[selmon->pertag->curtag] = (Layout*)arg->v;
    strncpy(selmon->ltsymbol, selmon->pertag->layout[selmon->pertag->curtag]->symbol, sizeof selmon->ltsymbol);
    if (selmon->sel)
        arrange(selmon);
    else
        drawbar(selmon);
}

void setcfact(const Arg* arg) {
    float f;
    Client* c;

    c = selmon->sel;

    if (!arg || !c || selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float)
        return;
    f = arg->f + c->cfact;
    if (arg->f == 0.0)
        f = 1.0;
    else if (f < 0.2 || f > 5.0)
        return;
    c->cfact = f;
    arrange(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void setmfact(const Arg* arg) {
    float f;

    if (!arg || selmon->pertag->layout[selmon->pertag->curtag]->arrange == layout_float)
        return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.1 || f > 0.9)
        return;
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
    arrange(selmon);
}

void setup(void) {
    int i;
    XSetWindowAttributes wa;
    Atom utf8string;

    /* clean up any zombies immediately */
    sigchld(0);

    /* init screen */
    screen = DefaultScreen(dpy);
    sw     = DisplayWidth(dpy, screen);
    sh     = DisplayHeight(dpy, screen);
    root   = RootWindow(dpy, screen);
    drw    = drw_create(dpy, screen, root, sw, sh);
    if (!drw_fontset_create(drw, fonts, nfonts))
        die("no fonts could be loaded.");
    lrpad = drw->fonts->h;
    for (struct Fnt* f = drw->fonts; f != NULL; f = f->next)
        if (bh < f->h)
            bh = f->h;
    bh += 2;
    updategeom();
    /* init atoms */
    utf8string                     = XInternAtom(dpy, "UTF8_STRING", False);
    wmatom[WMProtocols]            = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WMDelete]               = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WMState]                = XInternAtom(dpy, "WM_STATE", False);
    wmatom[WMTakeFocus]            = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow]       = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported]          = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMName]             = XInternAtom(dpy, "_NET_WM_NAME", False);
    netatom[NetWMState]            = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NetWMCheck]            = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMFullscreen]       = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMWindowType]       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList]         = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    /* init cursors */
    cursor[CurNormal]            = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResizeTopLeft]     = drw_cur_create(drw, XC_top_left_corner);
    cursor[CurResizeTopRight]    = drw_cur_create(drw, XC_top_right_corner);
    cursor[CurResizeBottomLeft]  = drw_cur_create(drw, XC_bottom_left_corner);
    cursor[CurResizeBottomRight] = drw_cur_create(drw, XC_bottom_right_corner);
    cursor[CurMove]              = drw_cur_create(drw, XC_fleur);
    /* init appearance */
    scheme = ecalloc(ncolors, sizeof(Clr*));
    for (i = 0; i < ncolors; i++)
        scheme[i] = drw_scm_create(drw, colors[i], 3);
    /* init bars */
    updatebars();
    updatestatus();
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
        PropModeReplace, (unsigned char*)&wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
        PropModeReplace, (unsigned char*)"dwm", 3);
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
        PropModeReplace, (unsigned char*)&wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
        PropModeReplace, (unsigned char*)netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor     = cursor[CurNormal]->cursor;
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | ButtonPressMask | PointerMotionMask | EnterWindowMask
        | LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}

void seturgent(Client* c, int urg) {
    if (c->isurgent == urg)
        return;

    XWMHints* wmh;

    c->isurgent = urg;
    if (!(wmh = XGetWMHints(dpy, c->win)))
        return;
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, c->win, wmh);
    XFree(wmh);
}

void showhide(Client* c) {
    if (!c)
        return;
    if (ISVISIBLE(c) && !ISHIDDEN(c)) {
        /* show clients top down */
        XMoveWindow(dpy, c->win, c->x, c->y);
        if ((c->mon->pertag->layout[c->mon->pertag->curtag]->arrange == layout_float || c->isfloating) && !c->isfullscreen)
            resize(c, c->x, c->y, c->w, c->h, 0);
        showhide(c->snext);
    } else {
        /* hide clients bottom up */
        showhide(c->snext);
        XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
    }
}

void sigchld(int unused) {
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("can't install SIGCHLD handler:");
    while (0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void spawn(const Arg* arg) {
    if (arg->v == dmenucmd)
        dmenumon[0] = '0' + selmon->num;
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char**)arg->v)[0], (char**)arg->v);
        fprintf(stderr, "dwm: execvp %s", ((char**)arg->v)[0]);
        perror(" failed");
        exit(EXIT_SUCCESS);
    }
}

void tag(const Arg* arg) {
    if (selmon->sel && arg->ui & TAGMASK) {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        arrange(selmon);
    }
}

void tagmon(const Arg* arg) {
    if (!selmon->sel || !mons->next)
        return;
    sendmon(selmon->sel, dirtomon(arg->i));
}

void togglefloating(const Arg* arg) {
    if (!selmon->sel)
        return;
    if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
        return;
    selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
    if (selmon->sel->isfloating) {
        resize(selmon->sel, selmon->sel->x, selmon->sel->y, selmon->sel->w, selmon->sel->h, 0);
        XRaiseWindow(dpy, selmon->sel->win);
    }
    arrange(selmon);
}

void togglefullscr(const Arg* arg) {
    if (selmon->sel)
        setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

void toggletag(const Arg* arg) {
    unsigned int newtags;

    if (!selmon->sel)
        return;
    newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
    if (newtags) {
        selmon->sel->tags = newtags;
        focus(NULL);
        arrange(selmon);
    }
}

void toggleview(const Arg* arg) {
    unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
    int i;

    selmon->tagset[selmon->seltags] = newtagset;

    if (newtagset == ~0) {
        selmon->pertag->prevtag = selmon->pertag->curtag;
        selmon->pertag->curtag  = 0;
    }

    /* test if the user did not select the same tag */
    if (!(newtagset & 1 << (selmon->pertag->curtag - 1)) && newtagset) {
        selmon->pertag->prevtag = selmon->pertag->curtag;
        for (i = 0; !(newtagset & 1 << i); i++)
            ;
        selmon->pertag->curtag = i + 1;
    }

    /* apply settings for this view */
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
    selmon->mfact   = selmon->pertag->mfacts[selmon->pertag->curtag];

    // Update urgent status
    for (Client* c = selmon->clients; c; c = c->next)
        if (ISVISIBLE(c) && c->isurgent)
            seturgent(c, 0);

    focus(NULL);
    arrange(selmon);
}

void unfocus(Client* c, int setfocus) {
    if (!c)
        return;
    grabbuttons(c, 0);
    XSetWindowBorder(dpy, c->win, scheme[c->hidden ? SchemeHidden : SchemeNorm][ColBorder].pixel);
    if (setfocus) {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void unmanage(Client* c, int destroyed) {
    Monitor* m = c->mon;
    XWindowChanges wc;

    detach(c);
    detachstack(c);
    if (!destroyed) {
        wc.border_width = c->oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        setclientstate(c, WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    free(c);
    focus(NULL);
    updateclientlist();
    arrange(m);
}

void unmapnotify(XEvent* e) {
    Client* c;
    XUnmapEvent* ev = &e->xunmap;

    if ((c = wintoclient(ev->window))) {
        if (ev->send_event)
            setclientstate(c, WithdrawnState);
        else
            unmanage(c, 0);
    }
}

void updateclientlist() {
    Client* c;
    Monitor* m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            XChangeProperty(dpy, root, netatom[NetClientList],
                XA_WINDOW, 32, PropModeAppend,
                (unsigned char*)&(c->win), 1);
}

int updategeom(void) {
    int dirty = 0;

    if (XineramaIsActive(dpy)) {
        int i, j, n, nn;
        Client* c;
        Monitor* m;
        XineramaScreenInfo* info   = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo* unique = NULL;

        for (n = 0, m = mons; m; m = m->next, n++)
            ;
        /* only consider unique geometries as separate screens */
        unique = ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; i++)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
        XFree(info);
        nn = j;
        if (n <= nn) { /* new monitors available */
            for (i = 0; i < (nn - n); i++) {
                for (m = mons; m && m->next; m = m->next)
                    ;
                if (m)
                    m->next = createmon();
                else
                    mons = createmon();
            }
            for (i = 0, m = mons; i < nn && m; m = m->next, i++)
                if (i >= n
                    || unique[i].x_org != m->mx || unique[i].y_org != m->my
                    || unique[i].width != m->mw || unique[i].height != m->mh) {
                    dirty  = 1;
                    m->num = i;
                    m->mx = m->wx = unique[i].x_org;
                    m->my = m->wy = unique[i].y_org;
                    m->mw = m->ww = unique[i].width;
                    m->mh = m->wh = unique[i].height;
                    updatebarpos(m);
                }
        } else { /* less monitors available nn < n */
            for (i = nn; i < n; i++) {
                for (m = mons; m && m->next; m = m->next)
                    ;
                while ((c = m->clients)) {
                    dirty      = 1;
                    m->clients = c->next;
                    detachstack(c);
                    c->mon = mons;
                    attach(c);
                    attachstack(c);
                }
                if (m == selmon)
                    selmon = mons;
                cleanupmon(m);
            }
        }
        free(unique);
    } else { /* default monitor setup */
        if (!mons)
            mons = createmon();
        if (mons->mw != sw || mons->mh != sh) {
            dirty    = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty) {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void updatenumlockmask(void) {
    unsigned int i, j;
    XModifierKeymap* modmap;

    numlockmask = 0;
    modmap      = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

void updatesizehints(Client* c) {
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PBaseSize) {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    } else
        c->basew = c->baseh = 0;
    if (size.flags & PResizeInc) {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    } else
        c->incw = c->inch = 0;
    if (size.flags & PMaxSize) {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    } else
        c->maxw = c->maxh = 0;
    if (size.flags & PMinSize) {
        c->minw = size.min_width;
        c->minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        c->minw = size.base_width;
        c->minh = size.base_height;
    } else
        c->minw = c->minh = 0;
    if (size.flags & PAspect) {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else
        c->maxa = c->mina = 0.0;
    c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void updatestatus(void) {
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        strcpy(stext, "dwm");
    drawbar(selmon);
}

void updatetitle(Client* c) {
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if (c->name[0] == '\0') /* hack to mark broken clients */
        strcpy(c->name, broken);
}

void updatewindowtype(Client* c) {
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen])
        setfullscreen(c, 1);
    if (wtype == netatom[NetWMWindowTypeDialog])
        c->isfloating = 1;
}

void updatewmhints(Client* c) {
    XWMHints* wmh;

    if ((wmh = XGetWMHints(dpy, c->win))) {
        if (c == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        } else
            c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        if (wmh->flags & InputHint)
            c->neverfocus = !wmh->input;
        else
            c->neverfocus = 0;
        XFree(wmh);
    }
}

void view(const Arg* arg) {
    int i;
    unsigned int tmptag;

    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
        return;
    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK) {
        selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
        selmon->pertag->prevtag         = selmon->pertag->curtag;

        if (arg->ui == ~0)
            selmon->pertag->curtag = 0;
        else {
            for (i = 0; !(arg->ui & 1 << i); i++)
                ;
            selmon->pertag->curtag = i + 1;
        }
    } else {
        tmptag                  = selmon->pertag->prevtag;
        selmon->pertag->prevtag = selmon->pertag->curtag;
        selmon->pertag->curtag  = tmptag;
    }

    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
    selmon->mfact   = selmon->pertag->mfacts[selmon->pertag->curtag];

    // Update urgent status
    for (Client* c = selmon->clients; c; c = c->next)
        if (ISVISIBLE(c) && c->isurgent)
            seturgent(c, 0);

    focus(NULL);
    arrange(selmon);
}

Client*
wintoclient(Window w) {
    Client* c;
    Monitor* m;

    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            if (c->win == w)
                return c;
    return NULL;
}

Monitor*
wintomon(Window w) {
    int x, y;
    Client* c;
    Monitor* m;

    if (w == root && getrootptr(&x, &y))
        return recttomon(x, y, 1, 1);
    for (m = mons; m; m = m->next)
        if (w == m->barwin)
            return m;
    if ((c = wintoclient(w)))
        return c->mon;
    return selmon;
}

void togglehidden(const Arg* arg) {
    Client* c;

    if (arg->v)
        c = (Client*)arg->v;
    else
        c = selmon->sel;
    if (!c)
        return;
    c->hidden = !c->hidden;
    XSetWindowBorder(dpy, c->win, scheme[c->hidden ? SchemeHidden : SchemeNorm][ColBorder].pixel);
    focus(NULL);
    arrange(c->mon);
}

void toggleshowhidden(const Arg* arg) {
    selmon->showhidden = !selmon->showhidden;
    focus(NULL);
    arrange(selmon);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int xerror(Display* dpy, XErrorEvent* ee) {
    if (ee->error_code == BadWindow
        || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
        || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
        || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
        || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
        || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
        || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
        ee->request_code, ee->error_code);
    return xerrorxlib(dpy, ee); /* may call exit */
}

int xerrordummy(Display* dpy, XErrorEvent* ee) {
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display* dpy, XErrorEvent* ee) {
    die("dwm: another window manager is already running");
    return -1;
}

void zoom(const Arg* arg) {
    Client* c = (Client*)arg->v;
    if (!c)
        c = selmon->sel;

    if (!selmon->pertag->layout[selmon->pertag->curtag]->arrange
        || (selmon->sel && selmon->sel->isfloating))
        return;
    if (c == nexttiled(selmon->clients))
        if (!c || !(c = nexttiled(c->next)))
            return;
    int attachmode     = selmon->attachmode;
    detach(c);
    selmon->attachmode = AttachFront;
    attach(c);
    selmon->attachmode = attachmode;
    focus(c);
    arrange(c->mon);
}

int main(int argc, char* argv[]) {
    if (argc == 2 && !strcmp("-v", argv[1]))
        die("dwm");
    else if (argc != 1)
        die("usage: dwm [-v]");
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("warning: no locale support\n", stderr);
    if (!(dpy = XOpenDisplay(NULL)))
        die("dwm: cannot open display");
    checkotherwm();
    setup();
#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1)
        die("pledge");
#endif /* __OpenBSD__ */
    scan();
    run();
    cleanup();
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}

void movestack(const Arg* arg) {
    Client *c = NULL, *p = NULL, *pc = NULL, *i;

    if (arg->i > 0) {
        /* find the client after selmon->sel */
        for (c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating || ISHIDDEN(c)); c = c->next)
            ;
        if (!c)
            for (c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating || ISHIDDEN(c)); c = c->next)
                ;

    } else {
        /* find the client before selmon->sel */
        for (i = selmon->clients; i != selmon->sel; i = i->next)
            if (ISVISIBLE(i) && !i->isfloating && !ISHIDDEN(i))
                c = i;
        if (!c)
            for (; i; i = i->next)
                if (ISVISIBLE(i) && !i->isfloating && !ISHIDDEN(i))
                    c = i;
    }
    /* find the client before selmon->sel and c */
    for (i = selmon->clients; i && (!p || !pc); i = i->next) {
        if (i->next == selmon->sel)
            p = i;
        if (i->next == c)
            pc = i;
    }

    /* swap c and selmon->sel selmon->clients in the selmon->clients list */
    if (c && c != selmon->sel) {
        Client* temp      = selmon->sel->next == c ? selmon->sel : selmon->sel->next;
        selmon->sel->next = c->next == selmon->sel ? c : c->next;
        c->next           = temp;

        if (p && p != c)
            p->next = c;
        if (pc && pc != selmon->sel)
            pc->next = selmon->sel;

        if (selmon->sel == selmon->clients)
            selmon->clients = c;
        else if (c == selmon->clients)
            selmon->clients = selmon->sel;

        arrange(selmon);
    }
}
