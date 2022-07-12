# Introduction

`wpid-by-wid` is program which outputs a PID for a given X Window ID.

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
