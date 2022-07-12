#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/extensions/XRes.h>
#include <xcb/xproto.h>

void die(const char* msg, ...) {
	va_list args;
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "ERROR: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

pid_t xres_win_pid(Display* dpy, xcb_window_t win) {
	XResClientIdSpec spec;
	spec.client = win;
	spec.mask = XRES_CLIENT_ID_PID_MASK;

	XResClientIdValue* client_ids;
	long num_ids;
	XResQueryClientIds(dpy, 1, &spec, &num_ids, &client_ids);

	pid_t pid = -1;
	for (uint i = 0; i < num_ids; i++) {
		if (client_ids[i].spec.mask == XRES_CLIENT_ID_PID_MASK) {
			pid = XResGetClientPid(&client_ids[i]);
			break;
		}
	}

	XResClientIdsDestroy(num_ids, client_ids);

	return pid;
}

pid_t xprop_win_pid(xcb_connection_t* dpy_con, xcb_window_t win) {
	pid_t pid = -1;

	char* window_pid_atom_name = "_NET_WM_PID";
	xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(
		dpy_con,
		xcb_intern_atom(
			dpy_con, 0, strlen(window_pid_atom_name), window_pid_atom_name
		),
		NULL
	);
	if (atom_reply) {
		xcb_get_property_reply_t* prop_reply = xcb_get_property_reply(
			dpy_con,
			xcb_get_property(
				dpy_con, 0, win, atom_reply->atom, XCB_ATOM_CARDINAL, 0, BUFSIZ
			),
			NULL
		);
		if (prop_reply) {
			int len = xcb_get_property_value_length(prop_reply);
			if (len != 0) {
				pid = *((pid_t*)xcb_get_property_value(prop_reply));
			}
			free(prop_reply);
		}
		free(atom_reply);
	} else {
		fprintf(
			stderr,
			"Failed to intern atom for querying the client list from X: %s",
			window_pid_atom_name
		);
	}

	return pid;
}

void validate_win(
	xcb_connection_t* dpy_con, xcb_screen_t* screen, xcb_window_t win
) {
	char* client_list_atom_name = "_NET_CLIENT_LIST";
	xcb_intern_atom_reply_t* list_wins_atom_reply = xcb_intern_atom_reply(
		dpy_con,
		xcb_intern_atom(
			dpy_con, 0, strlen(client_list_atom_name), client_list_atom_name
		),
		NULL
	);
	if (!list_wins_atom_reply) {
		die(
			"Failed to intern atom for querying the client list from X: %s",
			client_list_atom_name
		);
	}

	xcb_get_property_reply_t* list_wins_reply = xcb_get_property_reply(
		dpy_con,
		xcb_get_property(
			dpy_con,
			0,
			screen->root,
			list_wins_atom_reply->atom,
			XCB_GET_PROPERTY_TYPE_ANY,
			0,
			BUFSIZ
		),
		NULL
	);
	if (!list_wins_reply) {
		die("Failed to query windows list with atom %s", client_list_atom_name);
	}

	int reply_len = xcb_get_property_value_length(list_wins_reply);
	if (reply_len < 0) {
		die("Failed to get reply length for windows list query");
	}

	bool found = false;
	xcb_window_t* wins = xcb_get_property_value(list_wins_reply);
	for (uint i = 0; i < reply_len / sizeof(xcb_window_t); i++) {
		xcb_window_t listed_win = wins[i];
		if (win == listed_win) {
			found = true;
			break;
		}
	}

	if (!found) {
		die("Window could not be found: %u", win);
	}

	free(list_wins_reply);
	free(list_wins_atom_reply);
}

pid_t win_pid(Display* dpy, xcb_connection_t* dpy_con, xcb_window_t win) {
	pid_t pid = xres_win_pid(dpy, win);
	if (pid == -1) {
		pid = xprop_win_pid(dpy_con, win);
	}
	return pid;
}

xcb_window_t parse_window_id(char* str, int input_base) {
	char* endptr;
	xcb_window_t window_id = strtoul(str, &endptr, input_base);
	if (endptr == str || ((window_id == LONG_MAX || window_id == LONG_MIN) && errno == ERANGE)) {
		die("Argument could not be parsed as Window ID: %s", str);
	}
	return window_id;
}

void print_help(char* argv[]) {
	printf(
		"Usage: %s [window_id]\n%s\n",
		argv[0],
		"  [window_id] can be given in decimal (e.g. `123`, base 10) or "
		"hexadecimal form (e.g. `0xffffff`, base 16)."
	);
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		print_help(argv);
		die("Exactly one argument should be provided: the Window ID");
	}

	char* win_id_input = argv[1];
	assert(win_id_input);

	if (strncmp(win_id_input, "--help", strlen(win_id_input)) == 0) {
		print_help(argv);
		exit(0);
	}

	uint win_input_base;
	if (strncmp(win_id_input, "0x", strlen("0x")) == 0) {
		win_input_base = 0;
	} else {
		win_input_base = 10;
	}
	xcb_window_t win = parse_window_id(win_id_input, win_input_base);

	Display* dpy = XOpenDisplay(NULL);
	if (!dpy) {
		die("Failed to open display");
	}

	xcb_connection_t* dpy_con = XGetXCBConnection(dpy);
	int dpy_con_err = xcb_connection_has_error(dpy_con);
	if (dpy_con_err) {
		die("Failed to open connection to display");
	}

	xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(dpy_con)).data;

	validate_win(dpy_con, screen, win);

	pid_t pid = win_pid(dpy, dpy_con, win);
	if (pid == -1) {
		die("Window PID could not be found for %u", win);
	}

	printf("%d\n", pid);

	return 0;
}
