#include <QApplication>
#include "explore2remote.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::addLibraryPath("plugins");
	Explore2Remote w;
	w.show();
	return a.exec();
}
