/*
 File: FilesystemWatcher.cpp
 Created on: 3 feb. 2021
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
#include "AboutDialog.h"
#include "EventsTableModel.h"
#include "ObjectsTableModel.h"
#include "LogiLED.h"

// Qt
#include <QMenu>
#include <QCloseEvent>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QSoundEffect>
#include <QTemporaryFile>

const QString GEOMETRY = "Geometry";

Q_DECLARE_METATYPE(std::wstring);
Q_DECLARE_METATYPE(WatchThread::Event);

//-----------------------------------------------------------------------------
FilesystemWatcher::FilesystemWatcher(QWidget *p, Qt::WindowFlags f)
: QDialog(p,f)
, m_trayIcon{new QSystemTrayIcon(QIcon(":/FilesystemWatcher/eye-1.svg"), this)}
, m_watching{false}
, m_needsExit{false}
{
  qRegisterMetaType<std::wstring>();
  qRegisterMetaType<WatchThread::Event>();

  setupUi(this);

  m_eventsTable->setModel(new EventsTableModel());
  m_eventsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_eventsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);

  m_objectsTable->setModel(new ObjectsTableModel());
  m_objectsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_objectsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);

  m_stopButton->setVisible(false);

  connectSignals();

  setupTrayIcon();

  loadSettings();

  m_tabWidget->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------
FilesystemWatcher::~FilesystemWatcher()
{
  saveSettings();

  auto stopThread = [](Object &o)
  {
    o.thread->abort();
    o.thread->deleteLater();
  };
  std::for_each(m_objects.begin(), m_objects.end(), stopThread);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::connectSignals()
{
  connect(m_quit,       SIGNAL(clicked(bool)),       this, SLOT(quitApplication()));
  connect(m_about,      SIGNAL(clicked(bool)),       this, SLOT(onAboutButtonClicked()));
  connect(m_addObject,  SIGNAL(clicked(bool)),       this, SLOT(onAddObjectButtonClicked()));
  connect(m_copy,       SIGNAL(clicked(bool)),       this, SLOT(onCopyButtonClicked()));
  connect(m_stopButton, SIGNAL(clicked(bool)),       this, SLOT(stopAlarms()));

  connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));

  connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this,       SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::setupTrayIcon()
{
  auto menu = new QMenu(tr("Menu"));

  auto showAction = new QAction(tr("Restore..."));
  connect(showAction, SIGNAL(triggered(bool)), this, SLOT(onTrayActivated()));

  m_stopAction = new QAction(tr("Stop alarms"));
  connect(m_stopAction, SIGNAL(triggered(bool)), this, SLOT(stopAlarms()));
  m_stopAction->setVisible(false);

  auto addFile = new QAction(tr("Watch object..."));
  connect(addFile, SIGNAL(triggered(bool)), this, SLOT(onAddObjectButtonClicked()));

  auto aboutAction = new QAction(tr("About..."));
  connect(aboutAction, SIGNAL(triggered(bool)), this, SLOT(onAboutButtonClicked()));

  auto quitAction = new QAction(tr("Quit"));
  connect(quitAction, SIGNAL(triggered(bool)), this, SLOT(quitApplication()));

  menu->addAction(showAction);
  menu->addAction(m_stopAction);
  menu->addSeparator();
  menu->addAction(addFile);
  menu->addSeparator();
  menu->addAction(aboutAction);
  menu->addSeparator();
  menu->addAction(quitAction);

  m_trayIcon->setContextMenu(menu);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::loadSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "FilesystemWatcher");

  if(settings.contains(GEOMETRY))
  {
    auto geometry = settings.value(GEOMETRY).toByteArray();
    restoreGeometry(geometry);
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::saveSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "FilesystemWatcher");

  settings.setValue(GEOMETRY, saveGeometry());
  settings.sync();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onTabChanged(int index)
{
  m_addObject->setVisible(index == 0);
  m_copy->setVisible(index == 1);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onAddObjectButtonClicked()
{
  AddObjectDialog dialog(this);

  if(QDialog::Accepted == dialog.exec())
  {
    const auto obj = dialog.objectPath();

    auto thread = new WatchThread(std::filesystem::path(obj.toStdWString()), dialog.objectProperties());

    m_objects.emplace_back(obj.toStdWString(), dialog.objectAlarms(), dialog.alarmColor(), dialog.alarmVolume(), dialog.objectProperties(), thread);

    connect(thread, SIGNAL(error(const QString)),
            this,   SLOT(onWatcherError(const QString)));

    connect(thread, SIGNAL(modified(const std::wstring, const WatchThread::Event)),
            this,   SLOT(onModification(const std::wstring, const WatchThread::Event)));

    auto eventsModel = qobject_cast<EventsTableModel*>(m_eventsTable->model());
    connect(thread,      SIGNAL(modified(const std::wstring, const WatchThread::Event)),
            eventsModel, SLOT(modification(const std::wstring, const WatchThread::Event)));

    auto objectsModel = qobject_cast<ObjectsTableModel*>(m_objectsTable->model());
    connect(thread,       SIGNAL(modified(const std::wstring, const WatchThread::Event)),
            objectsModel, SLOT(modification(const std::wstring, const WatchThread::Event)));

    thread->start();

    objectsModel->addObject(obj);

    if(m_objects.size() == 1) updateTrayIcon();
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onCopyButtonClicked()
{
  auto eventModel = qobject_cast<EventsTableModel *>(m_eventsTable->model());
  eventModel->copyEventsToClipboard();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onAboutButtonClicked()
{
  AboutDialog dialog(this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::quitApplication()
{
  m_needsExit = true;
  close();

  QApplication::quit();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
  if(reason == QSystemTrayIcon::DoubleClick)
  {
    showNormal();
    m_trayIcon->hide();
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::closeEvent(QCloseEvent *e)
{
  if(!m_needsExit)
  {
    hide();
    m_trayIcon->show();

    e->ignore();
  }
  else
  {
    QDialog::closeEvent(e);
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onWatcherError(const QString message)
{
  QMessageBox::critical(this, tr("Watcher error"), message, QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onModification(const std::wstring object, const WatchThread::Event e)
{
  const auto qObject = QString::fromStdWString(object);

  auto matchObj = [&qObject](const Object &o)
  {
    auto obj = QString::fromStdWString(o.path);
    return qObject.startsWith(obj);
  };
  auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), matchObj);

  if(it != m_objects.cend())
  {
    const auto &data = *it;

    if((data.alarms & AlarmFlags::MESSAGE) != 0)
    {
      if(!isVisible())
      {
        QString message;
        switch(e)
        {
          case WatchThread::Event::ADDED:
            message = tr("Added %1").arg(qObject);
            break;
          case WatchThread::Event::MODIFIED:
            message = tr("Modified %1").arg(qObject);
            break;
          case WatchThread::Event::REMOVED:
            message = tr("Removed %1").arg(qObject);
            break;
          case WatchThread::Event::RENAMED_NEW:
            message = tr("Renamed object to %1").arg(qObject);
            break;
          case WatchThread::Event::RENAMED_OLD:
          // no break
          default:
            break;
        }

        if(!message.isEmpty())
        {
          m_trayIcon->showMessage(QString::fromStdWString(data.path), message, QIcon(":/FilesystemWatcher/eye-1.svg"), 1500);
        }
      }
    }

    if((data.alarms & AlarmFlags::SOUND) != 0)
    {
      m_alarmSound = new QSoundEffect(this);
      m_soundFile = QTemporaryFile::createLocalFile(":/FilesystemWatcher/Beeper.wav");
      m_alarmSound->setSource(QUrl::fromLocalFile(m_soundFile->fileName()));
      m_alarmSound->setLoopCount(QSoundEffect::Infinite);
      m_alarmSound->setVolume(static_cast<double>(data.volume)/100.0);
      m_alarmSound->play();
    }

    if((data.alarms & AlarmFlags::LIGHTS) != 0)
    {
      LogiLED::getInstance().setColor(data.color.red(), data.color.green(), data.color.blue());
    }

    if((data.alarms & (AlarmFlags::LIGHTS|AlarmFlags::SOUND)) != 0)
    {
      m_stopAction->setVisible(true);
      m_stopButton->setVisible(true);
    }
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::updateTrayIcon()
{
  const std::vector<QIcon> FRAMES = { QIcon(":/FilesystemWatcher/eye-1.svg"), QIcon(":/FilesystemWatcher/eye-2.svg"),
                                      QIcon(":/FilesystemWatcher/eye-1.svg"), QIcon(":/FilesystemWatcher/eye-0.svg") };

  static int index = 0;

  index = (index + 1) % FRAMES.size();

  if(m_trayIcon->isVisible())
  {
    m_trayIcon->setIcon(FRAMES.at(index));
  }
  else
  {
    setWindowIcon(FRAMES.at(index));
  }

  if(!m_objects.empty()) QTimer::singleShot(1000, this, SLOT(updateTrayIcon()));
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::stopAlarms()
{
  if(LogiLED::getInstance().isInUse()) LogiLED::getInstance().stopLights();

  if(m_alarmSound)
  {
    m_alarmSound->stop();
    delete m_alarmSound;
    m_alarmSound = nullptr;
    delete m_soundFile;
    m_soundFile = nullptr;
  }

  m_stopAction->setVisible(false);
  m_stopButton->setVisible(false);
}
