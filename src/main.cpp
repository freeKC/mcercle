/**
  This file is a part of mcercle
  Copyright (C) 2010-2012 Cyril FRAUSTI

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include <QtGui/QApplication>
#include <QDesktopWidget>
#include <QTextCodec>
#include "mainwindow.h"
#include <QMessageBox>
#include <QPlastiqueStyle>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow m_win;

    //Plastique style
    //QApplication::setStyle(new QPlastiqueStyle);

    //Traduction des chaines de la lib Qt
    QString locale = QLocale::system().name();
    QTranslator translator;
    translator.load(QString("qt_") + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&translator);

    m_win.init();
    m_win.show();

    return app.exec();
}
