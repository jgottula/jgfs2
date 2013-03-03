/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */
 

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include "../../lib/jgfs2.h"


int main(int argc, char **argv) {
	QApplication app(argc, argv);
	
	QWidget window;
	window.resize(800, 600);
	window.setWindowTitle(argv[0]);
	window.show();
	
	return app.exec();
}
