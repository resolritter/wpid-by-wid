# Introduction

`wpid-by-wid` is program which outputs a PID for a given X Window ID.

This tool serves the same purpose as `xdotool getwindowpid [id]` except that `wpid-by-wid` is more precise; while `xdotool` [only inspects `_NET_WM_PID`](https://github.com/jordansissel/xdotool/blob/b5d8d5c412b61e46b2c8c68f99bce2dcfddfa625/xdo.c#L1757), which might unset for a given window or perhaps set incorrectly, `wpid-by-id` [first searches the PID by using the XRes extension, only falling back to `_NET_WM_PID` in case the former somehow doesn't work](https://github.com/resolritter/wpid-by-wid/blob/b31a6df2cf76823967b9ab4663752969446d13f6/main.c#L146-L148) (we expect it to always work).

# Installing

First build the application by running `./build`. A `./wpid-by-wid` executable
will be compiled from that command, which you can then run as:

`wpid-by-wid [window_id]`

The `[window_id]` argument is given in decimal (e.g. `123`, base 10) or
hexadecimal (e.g. `0xffffff`, base 16). You're able to figure out the window ID
of a given application through a variety of ways such as `$WINDOWID`, `xprop`,
`xwininfo`, `wmctrl -l`, window manager events, X events, etcetera.

# References

- https://github.com/freedesktop/xorg-xwininfo
- https://stackoverflow.com/questions/52986307/x11-get-the-list-of-main-windows-using-xcb
- https://gitlab.gnome.org/GNOME/metacity/-/merge_requests/13/diffs
