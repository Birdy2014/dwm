#include "layouts.h"
#include "config.h"
#include "dwm.h"

void tile(Monitor* m) {
    unsigned int i, n, h, mw, my, ty;
    float mfacts = 0, sfacts = 0;
    Client* c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
        if (n < m->nmaster)
            mfacts += c->cfact;
        else
            sfacts += c->cfact;
    }
    if (n == 0)
        return;

    if (n > m->nmaster)
        mw = m->nmaster ? m->ww * m->mfact : 0;
    else
        mw = m->ww;
    for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            h = (m->wh - my) * (c->cfact / mfacts);
            resize(c, m->wx, m->wy + my, mw - (2 * c->bw), h - (2 * c->bw), 0);
            my += HEIGHT(c);
            mfacts -= c->cfact;
        } else {
            h = (m->wh - ty) * (c->cfact / sfacts);
            resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2 * c->bw), h - (2 * c->bw), 0);
            ty += HEIGHT(c);
            sfacts -= c->cfact;
        }
}

void centeredmaster(Monitor* m) {
    unsigned int i, n, h, mw, mx, my, oty, ety, tw;
    float mfacts = 0, lfacts = 0, rfacts = 0;
    Client* c;

    /* count number of clients in the selected monitor */
    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
        if (n < m->nmaster)
            mfacts += c->cfact;
        else if ((n - m->nmaster) % 2)
            lfacts += c->cfact;
        else
            rfacts += c->cfact;
    }
    if (n == 0)
        return;
    if (n == 1) {
        c = nexttiled(m->clients);
        resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
        return;
    }

    /* initialize areas */
    mw = m->ww;
    mx = 0;
    my = 0;
    tw = mw;

    if (n > m->nmaster) {
        /* go mfact box in the center if more than nmaster clients */
        mw = m->nmaster ? m->ww * m->mfact : 0;
        tw = m->ww - mw;

        if (n - m->nmaster > 1) {
            /* only one client */
            mx = (m->ww - mw) / 2;
            tw = (m->ww - mw) / 2;
        }
    }

    oty = 0;
    ety = 0;
    for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            /* nmaster clients are stacked vertically, in the center
		 * of the screen */
            h = (m->wh - my) * (c->cfact / mfacts);
            resize(c, m->wx + mx, m->wy + my, mw - 2 * c->bw,
                h - 2 * c->bw, 0);
            if (my + HEIGHT(c) < m->mh)
                my += HEIGHT(c);
            mfacts -= c->cfact;
        } else {
            /* stack clients are stacked vertically */
            if ((i - m->nmaster) % 2) {
                h = (m->wh - ety) * (c->cfact / lfacts);
                if (m->nmaster == 0)
                    resize(c, m->wx, m->wy + ety, tw - 2 * c->bw,
                        h - 2 * c->bw, 0);
                else
                    resize(c, m->wx, m->wy + ety, tw - 2 * c->bw,
                        h - 2 * c->bw, 0);
                if (ety + HEIGHT(c) < m->mh)
                    ety += HEIGHT(c);
                lfacts -= c->cfact;
            } else {
                h = (m->wh - oty) * (c->cfact / rfacts);
                resize(c, m->wx + mx + mw, m->wy + oty,
                    tw - 2 * c->bw, h - 2 * c->bw, 0);
                if (oty + HEIGHT(c) < m->mh)
                    oty += HEIGHT(c);
                rfacts -= c->cfact;
            }
        }
}

void centeredfloatingmaster(Monitor* m) {
    unsigned int i, n, w, mh, mw, mx, mxo, my, myo, tx;
    float mfacts = 0, sfacts = 0;
    Client* c;

    /* count number of clients in the selected monitor */
    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
        if (n < m->nmaster)
            mfacts += c->cfact;
        else
            sfacts += c->cfact;
    }
    if (n == 0)
        return;
    if (n == 1) {
        c = nexttiled(m->clients);
        resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
        return;
    }

    /* initialize nmaster area */
    if (n > m->nmaster) {
        /* go mfact box in the center if more than nmaster clients */
        if (m->ww > m->wh) {
            mw = m->nmaster ? m->ww * m->mfact : 0;
            mh = m->nmaster ? m->wh * 0.9 : 0;
        } else {
            mh = m->nmaster ? m->wh * m->mfact : 0;
            mw = m->nmaster ? m->ww * 0.9 : 0;
        }
        mx = mxo = (m->ww - mw) / 2;
        my = myo = (m->wh - mh) / 2;
    } else {
        /* go fullscreen if all clients are in the master area */
        mh = m->wh;
        mw = m->ww;
        mx = mxo = 0;
        my = myo = 0;
    }

    for (i = tx = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            /* nmaster clients are stacked horizontally, in the center
		 * of the screen */
            w = (mw + mxo - mx) * (c->cfact / mfacts);
            resize(c, m->wx + mx, m->wy + my, w - 2 * c->bw,
                mh - 2 * c->bw, 0);
            if (mx + WIDTH(c) < m->mw)
                mx += WIDTH(c);
            mfacts -= c->cfact;
        } else {
            /* stack clients are stacked horizontally */
            w = (m->ww - tx) * (c->cfact / sfacts);
            resize(c, m->wx + tx, m->wy, w - 2 * c->bw,
                m->wh - 2 * c->bw, 0);
            if (tx + WIDTH(c) < m->mw)
                tx += WIDTH(c);
            sfacts -= c->cfact;
        }
}

void dwindle(Monitor* mon) {
    unsigned int i, n, nx, ny, nw, nh, ow, oh, my = 0;
    float mfacts = 0;
    Client* c;

    for (n = 0, c = nexttiled(mon->clients); c; c = nexttiled(c->next), n++)
        if (n < mon->nmaster)
            mfacts += c->cfact;
    if (n == 0)
        return;

    nx = mon->wx;
    ny = mon->wy;
    nw = mon->ww;
    nh = mon->wh;

    for (i = 0, c = nexttiled(mon->clients); c; c = nexttiled(c->next)) {
        if (n > 1 && mon->nmaster > 0 && i == mon->nmaster) {
            // reset values for first client in stack
            nw = mon->ww * (1 - mon->mfact);
            nh = mon->wh;
            ny = mon->wy;
        }
        ow = nw;
        oh = nh;
        if (n > 1 && i < mon->nmaster) {
            // master
            // TODO kaputt für mehrere master
            nw = mon->ww * mon->mfact;
            nh = (mon->wh - my) * (c->cfact / mfacts);
            nx = mon->wx;
            ny = mon->wy + my;
            my += nh;
            mfacts -= c->cfact;
        } else if (n > 1 && i < n - 1) {
            // stack
            if ((i - mon->nmaster) % 2)
                nw *= (c->cfact / 2);
            else
                nh *= (c->cfact / 2);
        }
        resize(c, nx, ny, nw - 2 * c->bw, nh - 2 * c->bw, False);
        if ((i - mon->nmaster) % 2) {
            nx += nw;
            nw = ow - nw;
        } else {
            ny += nh;
            nh = oh - nh;
        }
        i++;
    }
}

void layout_float(Monitor* mon) {
    Client* c;
    unsigned int curtag = selmon->pertag->curtag;

    for (c = nexttiled(mon->clients); c; c = nexttiled(c->next)) {
        resize(c, c->fx[curtag], c->fy[curtag], c->fw[curtag], c->fh[curtag], 1);
    }
}

void monocle(Monitor* m) {
    unsigned int n = 0;
    Client* c;

    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
        n++;
    if (n > 0) /* override layout symbol */
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
        resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

void deck(Monitor* m) {
    unsigned int i, n, h, mw, my;
    float mfacts = 0;
    Client* c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
        if (n < m->nmaster)
            mfacts += c->cfact;
    }
    if (n == 0)
        return;

    if (n > m->nmaster) {
        mw = m->nmaster ? m->ww * m->mfact : 0;
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n - m->nmaster);
    } else
        mw = m->ww;
    for (i = my = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            h = (m->wh - my) * (c->cfact / mfacts);
            resize(c, m->wx, m->wy + my, mw - (2 * c->bw), h - (2 * c->bw), False);
            if (my + HEIGHT(c) < m->wh)
                my += HEIGHT(c);
            mfacts -= c->cfact;
        } else
            resize(c, m->wx + mw, m->wy, m->ww - mw - (2 * c->bw), m->wh - (2 * c->bw), False);
}
