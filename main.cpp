/*
 File: main.cpp
 Created on: 03/02/2021
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include "FilesystemWatcher.h"

// Qt
#include <QApplication>
#include <QSharedMemory>
#include <QObject>
#include <QMessageBox>
#include <QString>
#include <QIcon>

// C++
#include <iostream>

//-----------------------------------------------------------------
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  const char symbols[] =
  { 'I', 'E', '!', 'X' };
//  QString output = QString("[%1] %2 (%3:%4 -> %5)").arg( symbols[type] ).arg( msg ).arg(context.file).arg(context.line).arg(context.function);
  QString output = QString("[%1] %2").arg(symbols[type]).arg(msg);
  std::cerr << output.toStdString() << std::endl;
  if (type == QtFatalMsg) abort();
}

//-----------------------------------------------------------------
int main(int argc, char **argv)
{
  qInstallMessageHandler(myMessageOutput);

  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  // allow only one instance running
  QSharedMemory guard;
  guard.setKey("FilesystemWatcher");

  if (!guard.create(1))
  {
    QMessageBox msgbox;
    msgbox.setWindowIcon(QIcon(":/FilesystemWatcher/application.ico"));
    msgbox.setWindowTitle(QObject::tr("Filesystem Watcher"));
    msgbox.setIcon(QMessageBox::Information);
    msgbox.setText(QObject::tr("An instance is already running!"));
    msgbox.setStandardButtons(QMessageBox::Ok);
    msgbox.exec();

    return 0;
  }

  auto watcher = new FilesystemWatcher();
  watcher->showNormal();

  auto returnVal = app.exec();

  delete watcher;

  return returnVal;
}
