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
#include <FilesystemWatcher.h>
#include <AboutDialog.h>
#include <ObjectsTableModel.h>
#include <LogiLED.h>

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
const QString DEFAULT_EVENTS = "Default events";

Q_DECLARE_METATYPE(std::wstring);
Q_DECLARE_METATYPE(Events);

static std::atomic<bool> hasTrayMessage = false;

const QString INI_FILENAME{"FilesystemWatcher.ini"};

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
  m_objectsTable->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
  m_objectsTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

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
  };
  std::for_each(m_objects.begin(), m_objects.end(), stopThread);
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::connectSignals()
{
  connect(m_quit,         SIGNAL(clicked(bool)), this, SLOT(quitApplication()));
  connect(m_about,        SIGNAL(clicked(bool)), this, SLOT(onAboutButtonClicked()));
  connect(m_minimize,     SIGNAL(clicked(bool)), this, SLOT(close()));
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

  connect(m_objectsTable->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this,                             SLOT(onSelectionChanged()));

  connect(m_objectsTable, SIGNAL(customContextMenuRequested(const QPoint &)),
          this,           SLOT(onCustomMenuRequested(const QPoint &)));
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::setupTrayIcon()
{
  auto menu = new QMenu(tr("Menu"));

  auto showAction = new QAction(QIcon(":/FilesystemWatcher/maximize.svg"), tr("Restore..."));
  connect(showAction, SIGNAL(triggered(bool)), this, SLOT(onTrayActivated()));

  m_stopAction = new QAction(QIcon(":/FilesystemWatcher/alarm.svg"), tr("Stop alarms"));
  connect(m_stopAction, SIGNAL(triggered(bool)), this, SLOT(stopAlarms()));
  m_stopAction->setVisible(false);

  auto addFile = new QAction(QIcon(":/FilesystemWatcher/eye-1.svg"), tr("Watch object..."));
  connect(addFile, SIGNAL(triggered(bool)), this, SLOT(onAddObjectButtonClicked()));

  auto muteAction = new QAction(QIcon(":/FilesystemWatcher/eye-disabled.svg"), tr("Mute"));
  connect(muteAction, SIGNAL(triggered(bool)), this, SLOT(onMuteActionClicked()));

  auto aboutAction = new QAction(QIcon(":/FilesystemWatcher/info.svg"), tr("About..."));
  connect(aboutAction, SIGNAL(triggered(bool)), this, SLOT(onAboutButtonClicked()));

  auto quitAction = new QAction(QIcon(":/FilesystemWatcher/exit.svg"), tr("Quit"));
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
  const auto settings = applicationSettings();

  if(settings->contains(GEOMETRY))
  {
    auto geometry = settings->value(GEOMETRY).toByteArray();
    restoreGeometry(geometry);
  }

  m_lastDir = QDir{settings->value(LAST_DIRECTORY, QDir::home().absolutePath()).toString()};
  m_alarmVolume = static_cast<unsigned char>(settings->value(ALARM_VOLUME, 100).toInt());
  m_alarmFlags = static_cast<AlarmFlags>(settings->value(DEFAULT_ALARMS, 7).toInt());
  m_events = static_cast<Events>(settings->value(DEFAULT_EVENTS, 63).toInt());
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::saveSettings()
{
  auto settings = applicationSettings();

  settings->setValue(GEOMETRY, saveGeometry());
  settings->setValue(LAST_DIRECTORY, m_lastDir.absolutePath());
  settings->setValue(ALARM_VOLUME, m_alarmVolume);
  settings->setValue(DEFAULT_ALARMS, static_cast<int>(m_alarmFlags));
  settings->setValue(DEFAULT_EVENTS, static_cast<int>(m_events));
  settings->sync();
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onSelectionChanged()
{
  const auto indexes = m_objectsTable->selectionModel()->selectedRows();
  bool resetEnabled = false;

  for(auto index: indexes)
  {
    if(index.isValid())
    {
      auto distance = index.row();
      auto &data = m_objects.at(distance);
      resetEnabled |= (data.eventsNumber != 0);
    }
  }

  m_reset->setEnabled(resetEnabled);
  m_removeObject->setEnabled(!indexes.empty() && !m_objects.empty());
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onAddObjectButtonClicked()
{
  AddObjectDialog dialog(m_lastDir, m_alarmVolume, m_alarmFlags, m_events, m_objects, this);

  if(QDialog::Accepted == dialog.exec())
  {
    const auto obj = dialog.objectPath();
    const auto objectPath = std::filesystem::path(obj.toStdWString());
    if(!std::filesystem::exists(objectPath))
    {
      const auto message = tr("Cannot find object '%1'.").arg(obj);
      QMessageBox::information(this, tr("Add object"), message, QMessageBox::Ok);
      return;
    }

    auto equalPath = [&objectPath](const struct Object &o) { return o.path.compare(objectPath) == 0; };
    auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), equalPath);
    if(it != m_objects.cend())
    {
      const auto message = tr("Object '%1' is already being watched.").arg(obj);
      QMessageBox::information(this, tr("Add object"), message, QMessageBox::Ok);
      return;
    }

    m_alarmVolume = dialog.alarmVolume();
    m_alarmFlags = dialog.objectAlarms();
    m_events = dialog.objectEvents();

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

    const auto message = tr("Watching object \"%1\".").arg(obj);
    log(message);
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
  if(this->isVisible())
    close();
  else
    closeEvent(nullptr);
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
    if(e) QDialog::closeEvent(e);
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

    const bool hasSound  = (data.alarms & AlarmFlags::SOUND) != AlarmFlags::NONE;
    const bool hasLights = (data.alarms & AlarmFlags::LIGHTS) != AlarmFlags::NONE;
    const bool hasMessage = (data.alarms & AlarmFlags::MESSAGE) != AlarmFlags::NONE;

    if(!m_mute->isChecked() && (hasSound || hasLights || hasMessage))
    {
      soundAlarms(hasSound, hasLights, hasMessage, data, e);
    }

    m_copy->setEnabled(true);
    m_reset->setEnabled(true);
  }    
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

    auto message = tr("File <b>'%2'</b> renamed to <b>'%3'</b>.").arg(QString::fromStdWString(oldName)).arg(QString::fromStdWString(newName));
    log(message);

    const bool hasSound  = (data.alarms & AlarmFlags::SOUND) != AlarmFlags::NONE;
    const bool hasLights = (data.alarms & AlarmFlags::LIGHTS) != AlarmFlags::NONE;
    const bool hasMessage = (data.alarms & AlarmFlags::MESSAGE) != AlarmFlags::NONE;

    if(!m_mute->isChecked() && (hasSound || hasLights || hasMessage))
    {
      soundAlarms(hasSound, hasLights, hasMessage, data, Events::RENAMED_OLD);
    }

    m_copy->setEnabled(true);
    m_reset->setEnabled(true);
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

  hasTrayMessage = false;
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onResetButtonClicked()
{
  auto indexes = m_objectsTable->selectionModel()->selectedRows();

  auto moreThan = [](const QModelIndex &lhs, const QModelIndex &rhs)
  { return lhs.row() > rhs.row(); };

  std::sort(indexes.begin(), indexes.end(), moreThan);

  for(const auto index: indexes)
  {
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
}

//-----------------------------------------------------------------------------
void FilesystemWatcher::onRemoveButtonClicked()
{
  auto indexes = m_objectsTable->selectionModel()->selectedIndexes();
  
  auto moreThan = [](const QModelIndex &lhs, const QModelIndex &rhs)
  { return lhs.row() > rhs.row(); };

  std::sort(indexes.begin(), indexes.end(), moreThan);

  for(const auto index: indexes)
  {
    if(static_cast<unsigned int>(index.row()) < m_objects.size())
    {
      auto &data = m_objects.at(index.row());
      const auto message = tr("Stopped watching object \"%1\".").arg(QString::fromStdWString(data.path.wstring()));
      auto objectsModel = qobject_cast<ObjectsTableModel*>(m_objectsTable->model());
      objectsModel->removeObject(data.path.wstring());

      if(data.isInAlarm()) stopAlarms();
      
      data.thread->abort();

      m_objects.erase(m_objects.begin() + index.row());

      log(message);
    }
  }

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

//-----------------------------------------------------------------------------
void FilesystemWatcher::onCustomMenuRequested(const QPoint &p)
{
  QMenu menu;
  auto removeAction = new QAction(QIcon(":/FilesystemWatcher/remove.svg"), tr("Remove"));
  auto resetAction  = new QAction(QIcon(":/FilesystemWatcher/reset.svg"), tr("Reset"));

  menu.addAction(removeAction);
  menu.addAction(resetAction);
  menu.addSeparator();
  menu.addAction(new QAction("Cancel"));

  const auto idx = m_objectsTable->indexAt(p);
  m_objectsTable->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
  Q_ASSERT(m_objectsTable->selectionModel()->selectedIndexes().size() == 1);

  auto selectedAction = menu.exec(m_objectsTable->viewport()->mapToGlobal(p));
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
    m_mute->setToolTip(tr("Unmute alarms."));
    stopAlarms();
  }
  else
  {
    action->setText(tr("Mute"));
    m_mute->setToolTip(tr("Mute alarms."));
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

//-----------------------------------------------------------------------------
void FilesystemWatcher::soundAlarms(bool hasSound, bool hasLights, bool hasMessage, Object &obj, const Events type)
{
  if(m_mute->isChecked()) return;

  if(hasSound && !m_alarmSound && !m_soundFile)
  {
    obj.setIsInAlarm(true);

    m_alarmSound = new QSoundEffect(this);
    m_soundFile = QTemporaryFile::createNativeFile(":/FilesystemWatcher/Beeper.wav");
    m_alarmSound->setSource(QUrl::fromLocalFile(m_soundFile->fileName()));
    m_alarmSound->setLoopCount(QSoundEffect::Infinite);
    m_alarmSound->setVolume(static_cast<double>(obj.volume)/100.0);
    m_alarmSound->play();
  }

  if(hasLights)
  {
    obj.setIsInAlarm(true);

    if(!obj.color.isValid()) obj.color = QColor(255,255,255);
    LogiLED::getInstance().setColor(obj.color.red(), obj.color.green(), obj.color.blue());
  }

  m_stopAction->setVisible(obj.isInAlarm());
  m_stopButton->setEnabled(obj.isInAlarm());

  if(hasMessage)
  {
    const auto qObject = QString::fromStdWString(obj.path.wstring());
    const QString suffix = tr(" <b>'%1'</b>.").arg(qObject);
      
    QString message;
    switch(type)
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

      if(!m_mute->isChecked() && hasMessage)
      {
        if(isVisible())
        {
          if(showMessage(qObject, message))
          {
            stopAlarms();
          }
        }
        else
        {
          if(!hasTrayMessage.exchange(true))
          {
            const auto icon  = QIcon(":/FilesystemWatcher/eye-1.svg");
            message.remove("<b>").remove("</b>");
            m_trayIcon->showMessage(qObject, message, icon, 1500);
          }
        }
      }
    }
  }

}

//-----------------------------------------------------------------------------
std::unique_ptr<QSettings> FilesystemWatcher::applicationSettings() const
{
  QDir applicationDir{QCoreApplication::applicationDirPath()};
  if(applicationDir.exists(INI_FILENAME))
  {
    return std::make_unique<QSettings>(INI_FILENAME, QSettings::IniFormat);
  }

  return std::make_unique<QSettings>("Felix de las Pozas Alvarez", "FilesystemWatcher");
}
