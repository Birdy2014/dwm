/* See LICENSE file for copyright and license details. */

#ifndef CONFIG_H
#define CONFIG_H

#include "dwm.h"

/* appearance */
extern const unsigned int borderpx;
extern const unsigned int gappx;
extern const unsigned int snap;
extern const int showbar;
extern const int topbar;
extern const char* fonts[];
extern const int nfonts;
extern const char dmenufont[];
extern const char* colors[][3];
extern const int ncolors;
extern const char* separator;
extern const char* attachsymbols[];

/* tagging */
extern const char* tags[];
extern const int deflt[];
extern const int ntags;

extern const Rule rules[];
extern const int nrules;

/* layout(s) */
extern const float mfact;
extern const int nmaster;
extern const int resizehints;

extern const Layout layouts[];

// clang-format off
/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
// clang-format on

/* commands */
extern char dmenumon[2];
extern const char* dmenucmd[];
extern const char* termcmd[];

extern const Key keys[];
extern const int nkeys;

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
extern const Button buttons[];
extern const int nbuttons;

#endif
