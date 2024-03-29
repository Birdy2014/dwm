#include "config.h"

#include "dwm.h"
#include "bar.h"
#include "layouts.h"
#include <X11/XF86keysym.h>

// clang-format off

/* appearance */
const unsigned int borderpx  = 1;        /* border pixel of windows */
const unsigned int gappx     = 5;        /* gaps between windows */
const unsigned int snap      = 5;        /* snap pixel */
const int showbar            = 1;        /* 0 means no bar */
const int topbar             = 1;        /* 0 means bottom bar */
const char *fonts[]          = { "JetBrains Mono Nerd Font:size=10" };
const int nfonts = LENGTH(fonts);
const char dmenufont[]         = "Jetbrains Mono:size=10";
const char col_gray1[]         = "#202020";
const char col_gray2[]         = "#444444";
const char col_gray3[]         = "#bbbbbb";
const char col_gray4[]         = "#eeeeee";
const char col_cyan[]          = "#005577";
const char col_green[]         = "#10893E";
const char col_gruvbox_dark0_hard[] = "#1d2021";
const char col_gruvbox_dark0[] = "#282828";
const char col_gruvbox_dark1[] = "#3c3836";
const char col_gruvbox_dark2[] = "#504945";
const char col_gruvbox_dark3[] = "#665c54";
const char col_gruvbox_dark4[] = "#7c6f64";
const char col_gruvbox_green[] = "#98971a";
const char *colors[][3]      = {
	/*                 fg                 bg                 border   */
	[SchemeNorm]   = { col_gray3,         col_gruvbox_dark0_hard, col_gray2  },
    [SchemeNotSel] = { col_gray3,         col_gruvbox_dark0, col_gray2  },
	[SchemeHidden] = { col_gruvbox_dark4, col_gray1,         col_cyan   },
	[SchemeSel]    = { col_gray4,         col_gruvbox_dark2, col_gruvbox_green  },
};
const int ncolors = LENGTH(colors);
const char *separator = "│";
const char *attachsymbols[] = {
    [AttachFront] = "front",
    [AttachStack] = "stack",
    [AttachEnd]   = "end  ",
};

/* tagging */
const char *tags[] = { "", "", "", "", "", "", "", "8", "9" };
const int deflt[] = { 1, 1,  0,   0,   0,   0,   0,   0,   0,   0  };
const int ntags = LENGTH(tags);

const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class         instance title           tags mask     isfloating    monitor */
	{ "Steam",          0,    0,              1 << 4,       0,            0 },
	{ "discord",        0,    0,              1 << 6,       0,            -1 },
	{ "Element",        0,    0,              1 << 6,       0,            -1 },
	{ "code-oss",       0,    0,              1 << 2,       0,            0 },
	{ "Thunderbird",    0,    0,              1 << 5,       0,            0 },
	{ "steam_app",      0,    "Origin",       1 << 4,       1,            -1 },
	{ "zoom",           0,    0,              0,            1,            -1 },
	{ "Birdy3d",        0,    0,              0,            1,            -1 },
};
const int nrules = LENGTH(rules);

/* layout(s) */
const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
const int nmaster     = 1;    /* number of clients in master area */
const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      layout_float },
	{ "[M]",      monocle },
	{ "|M|",      centeredmaster },
	{ ">M>",      centeredfloatingmaster },
 	{ "[\\]",     dwindle },
	{ "[D]",      deck },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
const char *termcmd[]  = { "kitty", NULL };

const Key keys[] = {
	/* modifier                     key                       function          argument */
    // applications
	{ MODKEY,                       XK_d,                     spawn,            {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return,                spawn,            {.v = termcmd } },
	{ MODKEY,                       XK_b,                     togglebar,        {0} },
	{ MODKEY,                       XK_w,                     spawn,            SHCMD("firefox -p default-release") },
	{ MODKEY,                       XK_y,                     spawn,            SHCMD("firefox -p Youtube") },
	//{ MODKEY,                       XK_n,                     spawn,            SHCMD("kitty ranger") },
	{ MODKEY,                       XK_n,                     spawn,            SHCMD("kitty lf") },
    // tiling
	{ MODKEY|ShiftMask,             XK_j,                     movestack,        {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,                     movestack,        {.i = -1 } },
	{ MODKEY,                       XK_j,                     focusstack,       {.i = +1 } },
	{ MODKEY,                       XK_k,                     focusstack,       {.i = -1 } },
	{ MODKEY,                       XK_plus,                  incnmaster,       {.i = +1 } },
	{ MODKEY,                       XK_minus,                 incnmaster,       {.i = -1 } },
	{ MODKEY,                       XK_h,                     setmfact,         {.f = -0.05} },
	{ MODKEY,                       XK_l,                     setmfact,         {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_h,                     setcfact,         {.f = -0.20} },
	{ MODKEY|ShiftMask,             XK_l,                     setcfact,         {.f = +0.20} },
	{ MODKEY|ShiftMask,             XK_o,                     setcfact,         {.f =  0.00} },
	{ MODKEY,                       XK_Return,                zoom,             {0} },
	{ MODKEY|ShiftMask,             XK_q,                     killselected,       {0} },
	{ MODKEY,                       XK_q,                     closewindow,      {0} },
    { MODKEY,                       XK_v,                     togglehidden,     {0} },
    { MODKEY|ShiftMask,             XK_v,                     toggleshowhidden, {0} },
    // layouts
	{ MODKEY,                       XK_t,                     setlayout,        {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,                     setlayout,        {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,                     setlayout,        {.v = &layouts[2]} },
	{ MODKEY,                       XK_u,                     setlayout,        {.v = &layouts[3]} },
	{ MODKEY,                       XK_i,                     setlayout,        {.v = &layouts[4]} },
	{ MODKEY,                       XK_r,                     setlayout,        {.v = &layouts[5]} },
	{ MODKEY,                       XK_o,                     setlayout,        {.v = &layouts[6]} },
	{ MODKEY,                       XK_space,                 togglefloating,   {0} },
	{ MODKEY|ShiftMask,             XK_f,                     togglefullscr,    {0} },
	{ MODKEY,                       XK_a,                     setattach,        {.i = -1} },
    // monitors
	{ MODKEY,                       XK_comma,                 focusmon,         {.i = -1 } },
	{ MODKEY,                       XK_period,                focusmon,         {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,                 tagmon,           {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period,                tagmon,           {.i = +1 } },
    // tags
	{ MODKEY,                       XK_Tab,                   view,             {0} },
	{ MODKEY,                       XK_0,                     view,             {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,                     tag,              {.ui = ~0 } },
	TAGKEYS(                        XK_1,                                       0)
	TAGKEYS(                        XK_2,                                       1)
	TAGKEYS(                        XK_3,                                       2)
	TAGKEYS(                        XK_4,                                       3)
	TAGKEYS(                        XK_5,                                       4)
	TAGKEYS(                        XK_6,                                       5)
	TAGKEYS(                        XK_7,                                       6)
	TAGKEYS(                        XK_8,                                       7)
	TAGKEYS(                        XK_9,                                       8)
    // system
    { MODKEY|Mod1Mask|ControlMask,  XK_s,                     spawn,            SHCMD("systemctl poweroff") },
    { MODKEY|Mod1Mask|ControlMask,  XK_r,                     spawn,            SHCMD("systemctl reboot") },
    { MODKEY|Mod1Mask|ControlMask,  XK_l,                     spawn,            SHCMD("loginctl lock-session") },
	{ MODKEY|Mod1Mask|ControlMask,  XK_q,                     quit,             {0} },
    // audio and backlight
    { 0,                            XF86XK_AudioMute,         spawn,            SHCMD("pamixer -t && dwm-statusbar refresh") },
    { 0,                            XF86XK_AudioLowerVolume,  spawn,            SHCMD("pamixer -d 5 && dwm-statusbar refresh") },
    { 0,                            XF86XK_AudioRaiseVolume,  spawn,            SHCMD("pamixer -i 5 && dwm-statusbar refresh") },
    { 0,                            XF86XK_AudioPlay,         spawn,            SHCMD("playerctl play-pause") },
    { 0,                            XF86XK_AudioPrev,         spawn,            SHCMD("playerctl previous") },
    { 0,                            XF86XK_AudioNext,         spawn,            SHCMD("playerctl next") },
    { 0,                            XF86XK_MonBrightnessDown, spawn,            SHCMD("xbacklight -dec 10") },
    { 0,                            XF86XK_MonBrightnessUp,   spawn,            SHCMD("xbacklight -inc 10") },
	// Dunst
	{ MODKEY|ControlMask,           XK_c,                     spawn,            SHCMD("dunstctl close") },
	{ MODKEY|ControlMask,           XK_h,                     spawn,            SHCMD("dunstctl history-pop") },
};
const int nkeys = LENGTH(keys);

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
    { ClkAttach,            0,              Button1,        setattach,      {.i = -1} },
	{ ClkWinTitle,          0,              Button1,        bar_focusclient, {0} },
	{ ClkWinTitle,          0,              Button3,        togglehidden,   {0} },
	{ ClkWinTitle,          0,              Button2,        bar_closewindow,    {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         0,              8,              resizemouse,    {0} },
	{ ClkClientWin,         0,              9,              movemouse,      {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
const int nbuttons = LENGTH(buttons);
