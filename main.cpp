#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include "mainwindow.h"

static void setResourceDirectory()
{
    QStringList candidates;
    candidates << QCoreApplication::applicationDirPath() + "/../Resources";
#ifdef WOW_SOURCE_DIR
    candidates << QStringLiteral(WOW_SOURCE_DIR);
#endif
    candidates << QDir::currentPath();

    foreach(const QString &candidate, candidates) {
        QDir dir(candidate);
        if(QFileInfo::exists(dir.filePath("maps_wow.txt"))) {
            QDir::setCurrent(dir.absolutePath());
            qDebug() << "Using resource directory" << QDir::currentPath();
            return;
        }
    }

    qWarning() << "Unable to locate game resources from" << QDir::currentPath();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    setResourceDirectory();
    MainWindow w;
    return a.exec();
}
