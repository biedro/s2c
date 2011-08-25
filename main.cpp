#include <QtGui/QApplication>
#include <QDeclarativeContext>

#include <QDebug>

#include "qmlapplicationviewer.h"
#include "myengine.h"

int main(int argc, char *argv[]) {
     QApplication app(argc, argv);

     QDeclarativeView view;
     MyEngine myEngine;
     view.rootContext()->setContextProperty("myObject", &myEngine);

     view.setSource(QUrl::fromLocalFile("qml/demo_speech/main.qml"));
     view.show();

     return app.exec();
}
