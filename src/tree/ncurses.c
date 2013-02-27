/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */

#include "ncurses.h"
#include <ncurses.h>


void nc_init(void) {
	initscr();
	
	/* pass characters immediately, but not control characters */
	cbreak();
	
	/* don't echo characters entered by the user */
	noecho();
	
	/* give us function keypresses */
	keypad(stdscr, true);
}

void nc_done(void) {
	endwin();
}

void nc_loop(void) {
	bool done = false;
	
	do {
		int ch = getch();
		
		switch (ch) {
		case 'q':
			done = true;
			break;
		default:
			/* don't refresh */
			continue;
		}
		
		refresh();
	} while (!done);
}
