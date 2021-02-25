/* See LICENSE file for copyright and license details. */
#include <X11/XF86keysym.h>

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int gappx     = 5;        /* gaps between windows */
static const unsigned int snap      = 5;        /* snap pixel */
static const int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = {
                                        "Font Awesome 5 Free:size=10",
                                        "Font Awesome 5 Brands:size=10",
                                        "Jetbrains Mono:size=10",
                                        "monospace:size=10"
                                        };
static const char dmenufont[]         = "Jetbrains Mono:size=10";
static const char col_gray1[]         = "#222222";
static const char col_gray2[]         = "#444444";
static const char col_gray3[]         = "#bbbbbb";
static const char col_gray4[]         = "#eeeeee";
static const char col_cyan[]          = "#005577";
static const char col_green[]         = "#10893E";
static const char col_gruvbox_dark0[] = "#282828";
static const char col_gruvbox_dark2[] = "#504945";
static const char col_gruvbox_dark3[] = "#665c54";
static const char col_gruvbox_dark4[] = "#7c6f64";
static const char col_gruvbox_green[] = "#98971a";
static const char *colors[][3]      = {
	/*                 fg         bg                 border   */
	[SchemeNorm]   = { col_gray3, col_gruvbox_dark0, col_gray2  },
	[SchemeHidden] = { col_gray3, col_gray1,         col_cyan   },
	[SchemeSel]    = { col_gray4, col_gruvbox_dark2, col_gruvbox_green  },
};

/* tagging */
static const char *tags[] = { "", "", "", "", "", "", "", "8", "9" };
static const int deflt[] = { 1, 1,  0,   0,   0,   0,   0,   0,   0,   0  };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class         instance title           tags mask     isfloating    isterminal noswallow   monitor */
	{ "Steam",          0,    0,              1 << 4,       0,            0,         0,          0 },
	{ "discord",        0,    0,              1 << 6,       0,            0,         0,          -1 },
	{ "Element",        0,    0,              1 << 6,       0,            0,         0,          -1 },
	{ "code-oss",       0,    0,              1 << 2,       0,            0,         0,          0 },
	{ "Thunderbird",    0,    0,              1 << 5,       0,            0,         0,          0 },
	{ "st-256color",    0,    0,              0,            0,            1,         0,          -1 },
	{ 0,                0,    "Event Tester", 0,            0,            0,         1,          -1 },
	{ "steam_app",      0,    "Origin",       1 << 4,       1,            0,         0,          -1 },
	{ "zoom",           0,    0,              0,            1,            0,         0,          -1 },
	{ "Birdy3d",        0,    0,              0,            1,            0,         1,          -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      layout_float },
	{ "[M]",      monocle },
	{ "|M|",      centeredmaster },
	{ ">M>",      centeredfloatingmaster },
 	{ "[\\]",     dwindle },
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
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "st", NULL };

#include "movestack.c"
static Key keys[] = {
	/* modifier                     key                       function          argument */
    // applications
	{ MODKEY,                       XK_d,                     spawn,            {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return,                spawn,            {.v = termcmd } },
	{ MODKEY,                       XK_b,                     togglebar,        {0} },
	{ MODKEY,                       XK_w,                     spawn,            SHCMD("firefox -p default-release") },
	{ MODKEY,                       XK_y,                     spawn,            SHCMD("firefox -p Youtube") },
	{ MODKEY,                       XK_n,                     spawn,            SHCMD("st -e ranger") },
    { MODKEY,                       XK_c,                     spawn,            SHCMD("code") },
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
	{ MODKEY|ShiftMask,             XK_q,                     killclient,       {0} },
	{ MODKEY,                       XK_q,                     closewindow,      {0} },
    { MODKEY,                       XK_v,                     togglehidden,     {0} },
    { MODKEY|ShiftMask,             XK_v,                     toggleshowhidden, {0} },
    // layouts
	{ MODKEY,                       XK_t,                     setlayout,        {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,                     setlayout,        {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,                     setlayout,        {.v = &layouts[2]} },
	{ MODKEY,                       XK_u,                     setlayout,        {.v = &layouts[3]} },
	{ MODKEY,                       XK_o,                     setlayout,        {.v = &layouts[4]} },
	{ MODKEY,                       XK_r,                     setlayout,        {.v = &layouts[5]} },
	{ MODKEY,                       XK_space,                 togglefloating,   {0} },
	{ MODKEY|ShiftMask,             XK_f,                     togglefullscr,    {0} },
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
    { 0,                            XF86XK_MonBrightnessDown, spawn,            SHCMD("xbacklight -dec 10") },
    { 0,                            XF86XK_MonBrightnessUp,   spawn,            SHCMD("xbacklight -inc 10") },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

