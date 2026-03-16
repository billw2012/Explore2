#include "testapp1.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	testapp1 w;
	w.show();
	return a.exec();
}
