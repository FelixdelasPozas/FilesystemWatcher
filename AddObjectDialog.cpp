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
#include "LogiLED.h"

// Qt
#include <QFileDialog>
#include <QPixmap>
#include <QIcon>
#include <QColorDialog>
#include <QSoundEffect>
#include <QTemporaryFile>

// C++
#include <minwindef.h>

//-----------------------------------------------------------------------------
AddObjectDialog::AddObjectDialog(QWidget *p, Qt::WindowFlags f)
: QDialog(p,f)
, m_color(255,255,255)
{
  setupUi(this);

  connectSignals();

  updateColorButton();

  createSoundFile();

  if(!LogiLED::isAvailable())
  {
    m_useKeyboardLights->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
AddObjectDialog::~AddObjectDialog()
{
  if(m_useKeyboardLights->isChecked()) stopKeyboardColors();

  if(m_sound->isPlaying()) m_sound->stop();
  delete m_sound;
  m_sound = nullptr;
  delete m_soundFile;
  m_soundFile = nullptr;
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
  auto filename = QFileDialog::getOpenFileName(this, tr("Select file to watch"), QDir::currentPath());

  if(!filename.isEmpty())
  {
    filename = QDir::toNativeSeparators(filename);
    m_object->setText(filename);

    m_alarmGroup->setEnabled(true);
    m_propertiesGroup->setEnabled(true);

    updateEventsWidgets(false);
  }
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onAddFolderClicked()
{
  auto folder = QFileDialog::getExistingDirectory(this, tr("Select folder to watch"), QDir::currentPath());

  if(!folder.isEmpty())
  {
    folder = QDir::toNativeSeparators(folder);
    m_object->setText(folder);

    m_alarmGroup->setEnabled(true);
    m_propertiesGroup->setEnabled(true);

    updateEventsWidgets(true);
  }
}

//-----------------------------------------------------------------------------
QString AddObjectDialog::objectPath() const
{
  return m_object->text();
}

//-----------------------------------------------------------------------------
unsigned long AddObjectDialog::objectProperties() const
{
  unsigned long result = 0;

  if(m_createProp->isChecked()) result |= FILE_ACTION_ADDED;
  if(m_modifyProp->isChecked()) result |= FILE_ACTION_MODIFIED;
  if(m_deleteProp->isChecked()) result |= FILE_ACTION_REMOVED;;
  if(m_renameProp->isChecked()) result |= FILE_ACTION_RENAMED_NEW_NAME|FILE_ACTION_RENAMED_OLD_NAME;

  return result;
}

//-----------------------------------------------------------------------------
AlarmFlags AddObjectDialog::objectAlarms() const
{
  AlarmFlags flags = AlarmFlags::NONE;

  if(m_useTrayMessage->isChecked())    flags = flags | AlarmFlags::MESSAGE;
  if(m_useKeyboardLights->isChecked()) flags = flags | AlarmFlags::LIGHTS;
  if(m_soundAlarm->isChecked())        flags = flags | AlarmFlags::SOUND;

  return flags;
}

//-----------------------------------------------------------------------------
int AddObjectDialog::alarmVolume() const
{
  return m_volumeSlider->value() + 1;
}

//-----------------------------------------------------------------------------
QColor AddObjectDialog::alarmColor() const
{
  return m_color.isValid() ? m_color : QColor(255,255,255);
}

//-----------------------------------------------------------------------------
void AddObjectDialog::onColorButtonClicked()
{
  QColorDialog dialog(this);

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
  QPixmap pixmap(QSize{24,24});
  pixmap.fill(m_color);

  QIcon icon(pixmap);

  m_lightButton->setIcon(icon);
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
void AddObjectDialog::updateEventsWidgets(bool isDirectory)
{
  for(auto checkBox: {m_modifyProp, m_renameProp, m_deleteProp})
  {
    checkBox->setEnabled(true);
    checkBox->setChecked(true);
  }

  for(auto checkBox: {m_createProp, m_recursiveProp})
  {
    checkBox->setEnabled(isDirectory);
    checkBox->setChecked(isDirectory);
  }

  if(isDirectory) m_recursiveProp->setChecked(false);
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
  m_sound->setVolume(static_cast<double>(value + 1)/100);
  if(!m_sound->isPlaying()) m_sound->play();
}

//-----------------------------------------------------------------------------
bool AddObjectDialog::isRecursive() const
{
  return m_recursiveProp->isEnabled() && m_recursiveProp->isChecked();
}
