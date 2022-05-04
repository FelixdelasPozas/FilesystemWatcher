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
#include <QClipboard>
#include <QGuiApplication>
#include <QDateTime>
#include <QTextBlock>

// C++
#include <atomic>

const QString GEOMETRY = "Geometry";
const QString LAST_DIRECTORY = "Last used directory";
const QString ALARM_VOLUME = "Alarm volume";
const QString DEFAULT_ALARMS = "Default alarms";

Q_DECLARE_METATYPE(std::wstring);
Q_DECLARE_METATYPE(Events);

//-----------------------------------------------------------------------------
FilesystemWatcher::FilesystemWatcher(QWidget *p, Qt::WindowFlags f)
: QDialog(p,f)
, m_trayIcon{new QSystemTrayIcon(QIcon(":/FilesystemWatcher/eye-1.svg"), this)}
, m_needsExit{false}
, m_alarmSound{nullptr}
, m_soundFile{nullptr}
, m_lastDir{QDir::home()}
, m_alarmVolume{100}
{
  qRegisterMetaType<std::wstring>();
  qRegisterMetaType<Events>();

  setupUi(this);

  m_objectsTable->setModel(new ObjectsTableModel());
  m_objectsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_objectsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
  m_objectsTable->setContextMenuPolicy(Qt::CustomContextMenu);

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
  connect(m_quit,         SIGNAL(clicked(bool)), this, SLOT(quitApplication()));
  connect(m_about,        SIGNAL(clicked(bool)), this, SLOT(onAboutButtonClicked()));
  connect(m_addObject,    SIGNAL(clicked(bool)), this, SLOT(onAddObjectButtonClicked()));
  connect(m_copy,         SIGNAL(clicked(bool)), this, SLOT(onCopyButtonClicked()));
  connect(m_stopButton,   SIGNAL(clicked(bool)), this, SLOT(stopAlarms()));
  connect(m_reset,        SIGNAL(clicked(bool)), this, SLOT(onResetButtonClicked()));
  connect(m_removeObject, SIGNAL(clicked(bool)), this, SLOT(onRemoveButtonClicked()));
  connect(m_mute,         SIGNAL(toggled(bool)), this, SLOT(onMuteActionClicked()));

  connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this,       SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));
  connect(m_trayIcon, SIGNAL(messageClicked()),
          this,       SLOT(stopAlarms()));

  connect(m_objectsTable->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this,                             SLOT(onObjectSelected(const QModelIndex &, const QModelIndex &)));

  connect(m_objectsTable, SIGNAL(customContextMenuRequested(const QPoint &)),
          this,           SLOT(onCustomMenuRequested(const QPoint &)));
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

  auto muteAction = new QAction(tr("Mute"));
  connect(muteAction, SIGNAL(triggered(bool)), this, SLOT(onMuteActionClicked()));

  auto aboutAction = new QAction(tr("About..."));
  connect(aboutAction, SIGNAL(triggered(bool)), this, SLOT(onAboutButtonClicked()));

  auto quitAction = new QAction(tr("Quit"));
  connect(quitAction, SIGNAL(triggered(bool)), this, SLOT(quitApplication()));

  menu->addAction(showAction);
  menu->addAction(m_stopAction);
  menu->addAction(muteAction);
  menu->addSeparator();
  menu->addAction(addFile);
  menu->addSeparator();
  menu->addAction(aboutAction);
  menu->addSeparator();
  menu->addAction(quitAction);

  m_trayIcon->setContextMenu(menu);
  m_trayIcon->setToolTip(tr("Ready to watch"));
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

  m_lastDir = QDir{settings.value(LAST_DIRECTORY, QDir::home().absolutePath()).toString()};
  m_alarmVolume = static_cast<unsigned char>(settings.value(ALARM_VOLUME, 100).toInt());
  m_alarmFlags = static_cast<AlarmFlags>(settings.value(DEFAULT_ALARMS, 7).toInt());
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::saveSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "FilesystemWatcher");

  settings.setValue(GEOMETRY, saveGeometry());
  settings.setValue(LAST_DIRECTORY, m_lastDir.absolutePath());
  settings.setValue(ALARM_VOLUME, m_alarmVolume);
  settings.setValue(DEFAULT_ALARMS, static_cast<int>(m_alarmFlags));
  settings.sync();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onObjectSelected(const QModelIndex &selected, const QModelIndex &deselected)
{
  if(selected.isValid())
  {
    auto distance = selected.row();
    auto &data = m_objects.at(distance);
    m_reset->setEnabled(data.eventsNumber != 0);
  }

  m_removeObject->setEnabled(selected.isValid() && !m_objects.empty());
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onAddObjectButtonClicked()
{
  AddObjectDialog dialog(m_lastDir, m_alarmVolume, m_alarmFlags, m_objects, this);

  if(QDialog::Accepted == dialog.exec())
  {
    const auto obj = dialog.objectPath();
    const auto objectPath = std::filesystem::path(obj.toStdWString());

    auto equalPath = [&objectPath](const struct Object &o) { return o.path.compare(objectPath) == 0; };
    auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), equalPath);
    if(it != m_objects.cend())
    {
      const auto message = tr("Object '%1' is already being watched.").arg(QString::fromStdString(objectPath.string()));
      QMessageBox::information(this, tr("Add object"), message, QMessageBox::Ok);
      return;
    }

    m_alarmVolume = dialog.alarmVolume();
    m_alarmFlags = dialog.objectAlarms();

    auto thread = new WatchThread(objectPath, dialog.objectEvents(), dialog.isRecursive());

    m_objects.push_back(Object{objectPath, m_alarmFlags, dialog.alarmColor(), m_alarmVolume, dialog.objectEvents(), thread});

    connect(thread, SIGNAL(error(const QString)),
            this,   SLOT(onWatcherError(const QString)));

    connect(thread, SIGNAL(modified(const std::wstring, const Events)),
            this,   SLOT(onModification(const std::wstring, const Events)));

    connect(thread, SIGNAL(renamed(const std::wstring, const std::wstring)),
            this,   SLOT(onRename(const std::wstring, const std::wstring)));

    auto objectsModel = qobject_cast<ObjectsTableModel*>(m_objectsTable->model());
    connect(thread,       SIGNAL(modified(const std::wstring, const Events)),
            objectsModel, SLOT(modification(const std::wstring, const Events)));
    connect(thread, SIGNAL(renamed(const std::wstring, const std::wstring)),
            objectsModel, SLOT(rename(const std::wstring, const std::wstring)));

    thread->start();

    objectsModel->addObject(obj, dialog.alarmColor());

    const auto objectsNum = m_objects.size();

    if(objectsNum == 1) updateTrayIcon();

    m_trayIcon->setToolTip(tr("Watching %1 object%2").arg(objectsNum).arg(objectsNum > 1 ? "s":""));
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onCopyButtonClicked()
{
  auto clipboard = QGuiApplication::clipboard();
  clipboard->setText(m_log->document()->toPlainText());
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
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
  if(m_trayIcon->isVisible() && reason == QSystemTrayIcon::DoubleClick)
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

    e->accept();
  }
  else
  {
    QDialog::closeEvent(e);
    QApplication::exit(0);
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onWatcherError(const QString message)
{
  QMessageBox::critical(this, tr("Watcher error"), message, QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onModification(const std::wstring object, const Events e)
{
  const auto qObject = QString::fromStdWString(object);

  auto matchObj = [&qObject](const Object &o)
  {
    const auto obj = QString::fromStdWString(o.path.wstring());
    if(std::filesystem::is_directory(o.path))
    {
      return qObject.startsWith(obj, Qt::CaseInsensitive);
    }
    else
    {
      return qObject.compare(obj, Qt::CaseInsensitive) == 0;
    }
  };
  auto it = std::find_if(m_objects.begin(), m_objects.end(), matchObj);

  if(it != m_objects.end())
  {
    auto &data = *it;
    data.eventsNumber += 1;

    if(!m_mute->isChecked())
    {
      if((data.alarms & AlarmFlags::SOUND) != AlarmFlags::NONE && !m_alarmSound && !m_soundFile)
      {
        data.setIsInAlarm(true);

        m_alarmSound = new QSoundEffect(this);
        m_soundFile = QTemporaryFile::createLocalFile(":/FilesystemWatcher/Beeper.wav");
        m_alarmSound->setSource(QUrl::fromLocalFile(m_soundFile->fileName()));
        m_alarmSound->setLoopCount(QSoundEffect::Infinite);
        m_alarmSound->setVolume(static_cast<double>(data.volume)/100.0);
        m_alarmSound->play();
      }

      if((data.alarms & AlarmFlags::LIGHTS) != AlarmFlags::NONE)
      {
        data.setIsInAlarm(true);

        if(!data.color.isValid()) data.color = QColor(255,255,255);
        LogiLED::getInstance().setColor(data.color.red(), data.color.green(), data.color.blue());
      }

      m_stopAction->setVisible(data.isInAlarm());
      m_stopButton->setEnabled(data.isInAlarm());
    }

    QString suffix = tr(" <b>'%1'</b>.").arg(qObject);
    QString message;
    switch(e)
    {
      case Events::ADDED:
        message = tr("Added %2").arg(suffix);
        break;
      case Events::MODIFIED:
        message = tr("Modified %2").arg(suffix);
        break;
      case Events::REMOVED:
        message = tr("Removed %2").arg(suffix);
        break;
      case Events::RENAMED_NEW:
        message = tr("Renamed a file to %2").arg(suffix);
        break;
      case Events::RENAMED_OLD:
      // no break
      default:
        break;
    }

    if(!message.isEmpty())
    {
      log(message);

      if(!m_mute->isChecked() && (data.alarms & AlarmFlags::MESSAGE) != AlarmFlags::NONE)
      {
        const auto title = QString::fromStdWString(data.path.wstring());
        if(isVisible())
        {
          if(showMessage(title, message))
          {
            stopAlarms();
          }
        }
        else
        {
          const auto icon  = QIcon(":/FilesystemWatcher/eye-1.svg");
          message.remove("<b>").remove("</b>");
          m_trayIcon->showMessage(title, message, icon, 1500);
        }
      }
    }
  }

  m_copy->setEnabled(true);
  m_reset->setEnabled(true);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onRename(const std::wstring oldName, const std::wstring newName)
{
  auto qOldName = QString::fromStdWString(oldName);

  auto findName = [&qOldName](Object &o)
  {
    const auto name = QString::fromStdWString(o.path.wstring());
    return (name.compare(qOldName, Qt::CaseInsensitive) == 0);
  };
  auto it = std::find_if(m_objects.begin(), m_objects.end(), findName);

  if(it != m_objects.end())
  {
    auto &data = *it;
    data.path = std::filesystem::path{newName};
    data.eventsNumber += 1;

    // no need to alarm user because a rename also triggers an additional
    // modification event (modification of last modified time?) that will
    // trigger the alarms.

    auto message = tr("File <b>'%2'</b> renamed to <b>'%3'</b>.").arg(QString::fromStdWString(oldName)).arg(QString::fromStdWString(newName));
    log(message);

    if(!m_mute->isChecked() && ((data.alarms & AlarmFlags::MESSAGE) != AlarmFlags::NONE))
    {
      const auto title = QString::fromStdWString(data.path.wstring());
      const auto icon  = QIcon(":/FilesystemWatcher/eye-1.svg");
      if(isVisible())
      {
        if(showMessage(title, message))
        {
          stopAlarms();
        }
      }
      else
      {
        message.remove("<b>").remove("</b>");
        m_trayIcon->showMessage(title, message, icon, 1500);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::updateTrayIcon()
{
  const std::vector<QIcon> FRAMES = { QIcon(":/FilesystemWatcher/eye-1.svg"), QIcon(":/FilesystemWatcher/eye-2.svg"),
                                      QIcon(":/FilesystemWatcher/eye-1.svg"), QIcon(":/FilesystemWatcher/eye-0.svg") };

  static int index = 0;

  QIcon icon(":/FilesystemWatcher/eye-1.svg");

  if(m_mute->isChecked())
  {
    icon = QIcon(":/FilesystemWatcher/eye-disabled.svg");
  }
  else
  {
    if(m_objects.empty())
    {
      index = 0;
    }
    else
    {
      index = (index + 1) % FRAMES.size();
      icon = FRAMES.at(index);
      if(!m_needsExit) QTimer::singleShot(1000, this, SLOT(updateTrayIcon()));
    }
  }

  m_trayIcon->setIcon(icon);
  setWindowIcon(icon);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::stopAlarms()
{
  static std::atomic<bool> inUse = false;

  if(!inUse.exchange(true))
  {
    if(LogiLED::getInstance().isInUse()) LogiLED::getInstance().stopLights();
    if(m_alarmSound && m_soundFile)
    {
      m_alarmSound->stop();
      delete m_alarmSound;
      m_alarmSound = nullptr;
      delete m_soundFile;
      m_soundFile = nullptr;
    }

    m_stopAction->setVisible(false);
    m_stopButton->setEnabled(false);

    inUse = false;

    auto stopAlarm = [](Object &o){ o.setIsInAlarm(false); };
    std::for_each(m_objects.begin(), m_objects.end(), stopAlarm);
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onResetButtonClicked()
{
  auto index = m_objectsTable->selectionModel()->currentIndex();
  if(static_cast<unsigned int>(index.row()) < m_objects.size())
  {
    auto &data = m_objects.at(index.row());
    data.eventsNumber = 0;

    auto objectsModel = qobject_cast<ObjectsTableModel*>(m_objectsTable->model());
    objectsModel->resetObject(data.path.wstring());

    m_reset->setEnabled(false);

    if(data.isInAlarm()) stopAlarms();
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onRemoveButtonClicked()
{
  auto index = m_objectsTable->selectionModel()->currentIndex();
  if(static_cast<unsigned int>(index.row()) < m_objects.size())
  {
    auto &data = m_objects.at(index.row());
    auto objectsModel = qobject_cast<ObjectsTableModel*>(m_objectsTable->model());
    objectsModel->removeObject(data.path.wstring());

    if(data.isInAlarm()) stopAlarms();

    m_objects.erase(m_objects.begin() + index.row());

    const auto objectsNum = m_objects.size();

    m_removeObject->setEnabled(objectsNum != 0);

    if(objectsNum == 0)
    {
      m_trayIcon->setToolTip(tr("Ready to watch"));
    }
    else
    {
      m_trayIcon->setToolTip(tr("Watching %1 object%2").arg(objectsNum).arg(objectsNum > 1 ? "s":""));
    }
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onCustomMenuRequested(const QPoint &p)
{
  QMenu menu;
  auto removeAction = new QAction("Remove");
  auto resetAction  = new QAction("Reset");

  menu.addAction(removeAction);
  menu.addAction(resetAction);
  menu.addSeparator();
  menu.addAction(new QAction("Cancel"));

  auto selectedAction = menu.exec(m_objectsTable->viewport()->mapToGlobal(p));
  const auto idx = m_objectsTable->indexAt(p);
  m_objectsTable->selectionModel()->blockSignals(true);
  m_objectsTable->setCurrentIndex(idx);
  m_objectsTable->selectionModel()->blockSignals(false);

  if(selectedAction == removeAction)
  {
    onRemoveButtonClicked();
  }
  else
  {
    if(selectedAction == resetAction)
    {
      onResetButtonClicked();
    }
  }
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::log(const QString &message)
{
  const auto prefix = QDateTime::currentDateTime().toString("hh:mm:ss");
  m_log->append(tr("%1 - %2").arg(prefix).arg(message));
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onMuteActionClicked()
{
  bool state = m_mute->isChecked();
  auto action = qobject_cast<QAction *>(sender());
  if(action)
  {
    // this will call onMuteActionClicked again.
    m_mute->setChecked(!state);
    return;
  }

  action = m_trayIcon->contextMenu()->actions().at(2);
  if(state)
  {
    action->setText(tr("Unmute"));
    stopAlarms();
  }
  else
  {
    action->setText(tr("Mute"));
  }

  updateTrayIcon();
}

//-----------------------------------------------------------------------------
bool FilesystemWatcher::showMessage(const QString title, const QString message)
{
  static std::atomic<bool> inUse = false;

  if(!inUse.exchange(true))
  {
    inUse = true;
    const auto icon  = QIcon(":/FilesystemWatcher/eye-1.svg");
    QMessageBox msgBox(this);
    msgBox.setWindowIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    inUse = false;
    return true;
  }

  return false;
}
