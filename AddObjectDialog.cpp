/*
 File: AddObjectDialog.cpp
 Created on: 08/02/2021
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
#include "AddObjectDialog.h"
#include "FilesystemWatcher.h"
#include "LogiLED.h"

// Qt
#include <QFileDialog>
#include <QPixmap>
#include <QIcon>
#include <QColorDialog>
#include <QSoundEffect>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QMessageBox>

// C++
#include <minwindef.h>
#include <cstdlib>
#include <time.h>
#include <set>

//-----------------------------------------------------------------------------
AddObjectDialog::AddObjectDialog(QDir &lastDir, const int alarmVolume, const AlarmFlags flags, const Events events,
                                 const std::vector<Object> &objects, QWidget *p, Qt::WindowFlags f)
: QDialog(p,f)
, m_dir(lastDir)
, m_alarmFlags{flags}
, m_events{events}
, m_objects{objects}
{
  setupUi(this);

  createSoundFile();

  const auto value = std::min(100,std::max(1,alarmVolume));
  m_volumeSlider->setValue(value);
  m_volumeNumber->setText(tr("%1%").arg(value));
  m_sound->setVolume(static_cast<float>(value)/100.);

  m_useKeyboardLights->setChecked((m_alarmFlags & AlarmFlags::LIGHTS) != AlarmFlags::NONE);
  m_useTrayMessage->setChecked((m_alarmFlags & AlarmFlags::MESSAGE) != AlarmFlags::NONE);
  m_soundAlarm->setChecked((m_alarmFlags & AlarmFlags::SOUND) != AlarmFlags::NONE);
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  connectSignals();

  if(!LogiLED::isAvailable())
  {
    m_useKeyboardLights->setEnabled(false);
    m_lightButton->setEnabled(false);
  }
  else
  {
    generateColor();
    updateColorButton();
  }
}

//-----------------------------------------------------------------------------
AddObjectDialog::~AddObjectDialog()
{
  if(m_useKeyboardLights->isChecked()) stopKeyboardColors();

  if(m_sound->isPlaying()) m_sound->stop();
  if(m_sound)
  {
    delete m_sound;
    m_sound = nullptr;
  }
  if(m_soundFile)
  {
    delete m_soundFile;
    m_soundFile = nullptr;
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::connectSignals()
{
  connect(m_addFile,           SIGNAL(clicked()),         this, SLOT(onAddFileClicked()));
  connect(m_addFolder,         SIGNAL(clicked()),         this, SLOT(onAddFolderClicked()));
  connect(m_lightButton,       SIGNAL(clicked()),         this, SLOT(onColorButtonClicked()));
  connect(m_useKeyboardLights, SIGNAL(stateChanged(int)), this, SLOT(onKeyboardCheckStateChange(int)));
  connect(m_soundAlarm,        SIGNAL(stateChanged(int)), this, SLOT(onSoundAlarmCheckStateChanged(int)));
  connect(m_volumeSlider,      SIGNAL(valueChanged(int)), this, SLOT(onSoundVolumeChanged(int)));
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onAddFileClicked()
{
  m_object->setText("");
  m_alarmGroup->setEnabled(false);
  m_propertiesGroup->setEnabled(false);

  auto filename = QFileDialog::getOpenFileName(this, tr("Select file to watch"), m_dir.absolutePath());

  if(!filename.isEmpty())
  {
    const auto objectPath = std::filesystem::path(filename.toStdWString());
    if(!std::filesystem::exists(objectPath)) return;

    auto equalPath = [&objectPath](const struct Object &o) { return o.getPath().compare(objectPath) == 0; };
    auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), equalPath);
    if(it != m_objects.cend())
    {
      const auto message = tr("Object '%1' is already being watched.").arg(filename);
      QMessageBox::information(this, tr("Add object"), message, QMessageBox::Ok);
      return;
    }

    filename = QDir::toNativeSeparators(filename);
    m_object->setText(filename);

    updateWidgets(false);

    m_dir = QFileInfo{filename}.absoluteDir();

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onAddFolderClicked()
{
  m_object->setText("");
  m_alarmGroup->setEnabled(false);
  m_propertiesGroup->setEnabled(false);

  auto folder = QFileDialog::getExistingDirectory(this, tr("Select folder to watch"), m_dir.absolutePath());

  if(!folder.isEmpty())
  {
    const auto objectPath = std::filesystem::path(folder.toStdWString());
    if(!std::filesystem::exists(objectPath)) return;

    auto equalPath = [&objectPath](const struct Object &o) { return o.getPath().compare(objectPath) == 0; };
    auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), equalPath);
    if(it != m_objects.cend())
    {
      const auto message = tr("Object '%1' is already being watched.").arg(folder);
      QMessageBox::information(this, tr("Add object"), message, QMessageBox::Ok);
      return;
    }

    folder = QDir::toNativeSeparators(folder);
    m_object->setText(folder);

    updateWidgets(true);

    m_dir = QDir{folder}.absolutePath();

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
QString AddObjectDialog::objectPath() const
{
  return m_object->text();
}

//-----------------------------------------------------------------------------
Events AddObjectDialog::objectEvents() const
{
  Events result = Events::NONE;

  if(m_createProp->isChecked()) result |= Events::ADDED;
  if(m_modifyProp->isChecked()) result |= Events::MODIFIED;
  if(m_deleteProp->isChecked()) result |= Events::REMOVED;
  if(m_renameProp->isChecked()) result |= Events::RENAMED_NEW|Events::RENAMED_OLD;
  if(m_recursiveProp->isChecked()) result |= Events::RECURSIVE;

  return result;
}

//-----------------------------------------------------------------------------
AlarmFlags AddObjectDialog::objectAlarms() const
{
  AlarmFlags flags = AlarmFlags::NONE;

  if(m_useTrayMessage->isChecked())    flags |= AlarmFlags::MESSAGE;
  if(m_useKeyboardLights->isChecked()) flags |= AlarmFlags::LIGHTS;
  if(m_soundAlarm->isChecked())        flags |= AlarmFlags::SOUND;

  return flags;
}

//-----------------------------------------------------------------------------
int AddObjectDialog::alarmVolume() const
{
  return m_volumeSlider->value();
}

//-----------------------------------------------------------------------------
QColor AddObjectDialog::alarmColor() const
{
  return m_useKeyboardLights->isChecked() ? m_color : QColor();
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onColorButtonClicked()
{
  QColorDialog dialog(this);
  dialog.setCurrentColor(m_color);
  dialog.setWindowIcon(QIcon(":/FilesystemWatcher/eye-1.svg"));
  dialog.setWindowTitle(tr("Select keyboard lights color"));
  dialog.setModal(true);

  connect(&dialog, SIGNAL(currentColorChanged(const QColor &)), this, SLOT(setKeyboardColor(const QColor &)));

  if(QDialog::Accepted == dialog.exec())
  {
    m_color = dialog.selectedColor();
    updateColorButton();
  }

  setKeyboardColor(m_color);
}

//-----------------------------------------------------------------------------
void AddObjectDialog::updateColorButton()
{
  if(!LogiLED::isAvailable()) return;

  QPixmap pixmap(QSize{24,24});
  pixmap.fill(m_color);

  QIcon icon(pixmap);

  m_lightButton->setIcon(icon);
  m_lightButton->setEnabled(m_useKeyboardLights->isChecked());
}

//-----------------------------------------------------------------------------
void AddObjectDialog::setKeyboardColor(const QColor &color)
{
  if(LogiLED::isAvailable())
  {
    LogiLED::getInstance().setColor(color.red(), color.green(), color.blue());
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::stopKeyboardColors()
{
  if(LogiLED::isAvailable())
  {
    LogiLED::getInstance().stopLights();
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onKeyboardCheckStateChange(int state)
{
  if(!LogiLED::isAvailable()) return;

  if(state == Qt::Checked)
  {
    setKeyboardColor(m_color);
  }
  else
  {
    stopKeyboardColors();
  }

  m_lightButton->setEnabled(state == Qt::Checked);
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onSoundAlarmCheckStateChanged(int state)
{
  m_volumeNumber->setEnabled(state == Qt::Checked);
  m_volumeSlider->setEnabled(state == Qt::Checked);

  if(state == Qt::Checked && !m_sound->isPlaying())
  {
    m_sound->play();
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::updateWidgets(bool isDirectory)
{
  const bool hasAddFlag = (m_events & Events::ADDED) != Events::NONE;
  const bool hasModifyFlag = (m_events & Events::MODIFIED) != Events::NONE;
  const bool hasRemoveFlag = (m_events & Events::REMOVED) != Events::NONE;
  const bool hasRenameFlag = (m_events & (Events::RENAMED_NEW|Events::RENAMED_OLD)) != Events::NONE;
  const bool hasRecursive = (m_events & Events::RECURSIVE) != Events::NONE;

  for(auto &cbox: {m_modifyProp, m_deleteProp, m_renameProp})
    cbox->setEnabled(true);

  m_createProp->setChecked(hasAddFlag);
  m_createProp->setEnabled(isDirectory);

  m_modifyProp->setChecked(hasModifyFlag);
  m_deleteProp->setChecked(hasRemoveFlag);
  m_renameProp->setChecked(hasRenameFlag);

  m_recursiveProp->setChecked(hasRecursive);
  m_recursiveProp->setEnabled(isDirectory);

  m_alarmGroup->setEnabled(true);
  m_propertiesGroup->setEnabled(true);

  onKeyboardCheckStateChange(m_useKeyboardLights->isChecked() ? Qt::Checked : Qt::Unchecked);
  onSoundAlarmCheckStateChanged(m_soundAlarm->isChecked() ? Qt::Checked : Qt::Unchecked);
}

//-----------------------------------------------------------------------------
void AddObjectDialog::createSoundFile()
{
  m_sound = new QSoundEffect(this);
  m_soundFile = QTemporaryFile::createLocalFile(":/FilesystemWatcher/Beeper.wav");
  m_sound->setSource(QUrl::fromLocalFile(m_soundFile->fileName()));
  m_sound->setLoopCount(3);
  m_sound->setVolume(1);
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onSoundVolumeChanged(int value)
{
  m_sound->setVolume(static_cast<double>(value)/100);
  m_volumeNumber->setText(tr("%1%").arg(value));
  if(!m_sound->isPlaying()) m_sound->play();
}

//-----------------------------------------------------------------------------
bool AddObjectDialog::isRecursive() const
{
  return m_recursiveProp->isEnabled() && m_recursiveProp->isChecked();
}

//-----------------------------------------------------------------------------
void AddObjectDialog::generateColor()
{
  srand(time(nullptr));

  if(m_objects.empty())
  {
    m_color = QColor::fromHsv(rand() % 360, 255, 255).toRgb();
    return;
  }

  int increment = 180;
  bool valid = false;
  int hue = 0;
  std::set<int> usedValues;

  auto addValue = [&usedValues](const Object &o)
                  { usedValues.insert(o.getColor().hue()); };

  std::for_each(m_objects.cbegin(), m_objects.cend(), addValue);


  auto testValue = [&usedValues, &increment](const Object &o)
                   { return usedValues.count((o.getColor().hue() + increment) % 360) == 0; };

  while (!valid && increment > 1)
  {
    auto it = std::find_if(m_objects.cbegin(), m_objects.cend(), testValue);
    if(it != m_objects.cend())
    {
      hue = ((*it).getColor().hue() + increment) % 360;
      valid = true;
    }
    else
    {
      increment /= 2;
    }
  }

  m_color = QColor::fromHsv(hue, 255, 255).toRgb();
}
