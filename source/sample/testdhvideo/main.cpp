#include "videotest.h"
#include <QtWidgets/QApplication>
//#include <QtWidgets/QWidget>
//#include <QtWidgets/QMessageBox>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
   videoTest w;
   // QWidget w;
	w.show();
	return a.exec();
}
