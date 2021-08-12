#include "bar.h"

#include "dwm.h"
#include "config.h"
#include "util.h"
#include "layouts.h"

void drawbar(Monitor* m) {
    int x, w, sw = 0, tw, mw, ew = 0;
    int boxs = drw->fonts->h / 9;
    int boxw = drw->fonts->h / 6 + 2;
    unsigned int i, occ = 0, urg = 0, n = 0;
    Client* c;

    /* draw status first so it can be overdrawn by tags later */
    if (m == selmon) { /* status is only drawn on selected monitor */
        drw_setscheme(drw, scheme[SchemeNorm]);
        sw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
        drw_text(drw, m->ww - sw, 0, sw, bh, 0, stext, 0);
    }

    // Find out on which tags are windows
    for (c = m->clients; c; c = c->next) {
        if (ISVISIBLE(c))
            n++;
        occ |= c->tags;
        if (c->isurgent)
            urg |= c->tags;
    }
    // Draw tags
    x = 0;
    for (i = 0; i < ntags; i++) {
        w = TEXTW(tags[i]);
        drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
        drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
        if (occ & 1 << i)
            drw_rect(drw, x + boxs, boxs, boxw, boxw,
                m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
                urg & 1 << i);
        x += w;
    }
    // Draw layout symbol
    w = blw = TEXTW(m->ltsymbol);
    drw_setscheme(drw, scheme[SchemeNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

    // Draw attach mode
    w = TEXTW(attachsymbols[m->attachmode]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, attachsymbols[m->attachmode], 0);

    // Draw window names
    if ((w = m->ww - sw - x) > bh) {
        if (n > 0) {
            tw = TEXTW(m->sel->name) + lrpad;
            mw = (tw >= w || n == 1) ? 0 : (w - tw - TEXTW(separator)) / (n - 1);

            i = 0;
            for (c = m->clients; c; c = c->next) {
                if (!ISVISIBLE(c) || c == m->sel)
                    continue;
                tw = TEXTW(c->name);
                if (tw < mw)
                    ew += (mw - tw - TEXTW(separator));
                else
                    i++;
            }
            if (i > 0)
                mw += ew / i;

            i = 0;
            for (c = m->clients; c; c = c->next) {
                if (!ISVISIBLE(c))
                    continue;
                tw = MIN(m->sel == c || n == 1 ? w : mw, TEXTW(c->name));

                drw_setscheme(drw, scheme[m->sel == c ? SchemeSel : (c->hidden ? SchemeHidden : SchemeNotSel)]);
                if (tw > 0) /* trap special handling of 0 in drw_text */
                    drw_text(drw, x, 0, tw, bh, lrpad / 2, c->name, 0);
                if (c->isfloating)
                    drw_rect(drw, x + boxs, boxs, boxw, boxw, c->isfixed, 0);

                drw_setscheme(drw, scheme[SchemeNorm]);
                if (i < n - 1 && tw < w) {
                    drw_text(drw, x + tw, 0, TEXTW(separator), bh, lrpad / 2, separator, 0);
                    x += TEXTW(separator);
                    w -= TEXTW(separator);
                }
                x += tw;
                w -= tw;
                i++;
            }
            drw_setscheme(drw, scheme[SchemeNorm]);
            if (w > 0)
                drw_rect(drw, x, 0, w, bh, 1, 1);
        } else {
            drw_setscheme(drw, scheme[SchemeNorm]);
            drw_rect(drw, x, 0, w, bh, 1, 1);
        }
    }
    drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void drawbars(void) {
    Monitor* m;

    for (m = mons; m; m = m->next)
        drawbar(m);
}

void togglebar(const Arg* arg) {
    selmon->showbar = !selmon->showbar;
    updatebarpos(selmon);
    XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
    arrange(selmon);
}

void updatebars(void) {
    Monitor* m;
    XSetWindowAttributes wa = {
        .override_redirect = True,
        .background_pixmap = ParentRelative,
        .event_mask        = ButtonPressMask | ExposureMask
    };
    XClassHint ch = { "dwm", "dwm" };
    for (m = mons; m; m = m->next) {
        if (m->barwin)
            continue;
        m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
            CopyFromParent, DefaultVisual(dpy, screen),
            CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
        XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
        XMapRaised(dpy, m->barwin);
        XSetClassHint(dpy, m->barwin, &ch);
    }
}

void updatebarpos(Monitor* m) {
    m->wy = m->my;
    m->wh = m->mh;
    if (m->showbar) {
        m->wh -= bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh;
        m->wy = m->topbar ? m->wy + bh : m->wy;
    } else
        m->by = -bh;
}

void bar_closewindow(const Arg* arg) {
    Client* client = (Client*) arg->v;
    if (!client)
        return;
    unsigned int newtags = client->tags ^ selmon->tagset[selmon->seltags];
    if (newtags != 0) {
        client->tags = newtags;
        focus(NULL);
        arrange(selmon);
    } else {
        killclient(client);
    }
}

void bar_focusclient(const Arg* arg) {
    if (!arg->v)
        return;

    Client* c = (Client*)arg->v;

    focus(c);
    restack(selmon);
    if (c->isfloating || selmon->pertag->layout[selmon->pertag->curtag]->arrange == &layout_float)
        XRaiseWindow(dpy, c->win);
}
