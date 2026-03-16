#ifndef TESTAPP1_H
#define TESTAPP1_H

#include <QtWidgets/QMainWindow>
#include "ui_testapp1.h"

class testapp1 : public QMainWindow
{
	Q_OBJECT

public:
	testapp1(QWidget *parent = 0);
	~testapp1();

private:
	Ui::testapp1Class ui;
};

#endif // TESTAPP1_H
