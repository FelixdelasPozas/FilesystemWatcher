/*
 File: AddObjectDialog.h
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

#ifndef ADDOBJECTDIALOG_H_
#define ADDOBJECTDIALOG_H_

#include "ui_AddObjectDialog.h"

// Qt
#include <QDialog>
#include <QDir>

class QSoundEffect;
class QTemporaryFile;
class Object;

enum AlarmFlags : char
{
  NONE = 0, MESSAGE = 1, LIGHTS = 2, SOUND = 4
};

inline AlarmFlags operator|(AlarmFlags a, AlarmFlags b)
{
  return static_cast<AlarmFlags>(static_cast<int>(a) | static_cast<int>(b));
}

/** \class AddObjectDialog
 * \brief Implements the dialog to add an object to watch.
 *
 */
class AddObjectDialog
: public QDialog
, private Ui::AddObjectDialog
{
    Q_OBJECT
  public:
    /** \brief AddObjectDialog class constructor.
     * \param[in] lastDir Last used directory for opening objects.
     * \param[in] alarmVolume Default volume of the sound alarm.
     * \param[in] objects List of current wathed objects.
     * \param[in] p Raw pointer of the object parent of this one.
     * \param[in] f Dialog flags.
     *
     */
    explicit AddObjectDialog(QDir &lastDir, const int alarmVolume, const std::vector<Object> &objects,
                             QWidget *p = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief AddObjectDialog class virtual destructor.
     *
     */
    virtual ~AddObjectDialog();

    /** \brief Returns the object's path.
     *
     */
    QString objectPath() const;

    /** \brief Returns the object properties to watch.
     *
     */
    unsigned long objectProperties() const;

    /** \brief Returns the alarms for the object modifications.
     *
     */
    AlarmFlags objectAlarms() const;

    /** \brief Returns the alarm volume value.
     *
     */
    int alarmVolume() const;

    /** \brief Returns the selected alarm color for the keyboard lights.
     *
     */
    QColor alarmColor() const;

    /** \brief Returns true if the object is a subdirectory and the whole tree must be monitored,
     * and false otherwise.
     *
     */
    bool isRecursive() const;

  private slots:
    /** \brief Shows the dialog to select a filesystem file to watch.
     *
     */
    void onAddFileClicked();

    /** \brief Shows the dialog to select a filesystem folder to watch.
     *
     */
    void onAddFolderClicked();

    /** \brief Opens the color selection dialog and updates the color if the user accepts.
     *
     */
    void onColorButtonClicked();

    /** \brief Sets the keyboard color to the given one.
     * \param[in] color QColor object.
     *
     */
    void setKeyboardColor(const QColor &color);

    /** \brief Enables/Disables the keyboard lights according to state and updates de UI.
     * \param[in] state Check state.
     *
     */
    void onKeyboardCheckStateChange(int state);

    /** \brief Updates the UI according to the state.
     * \param[in] state Check state.
     *
     */
    void onSoundAlarmCheckStateChanged(int state);

    /** \brief Updates the volume of the alarm sound.
     * \param[in] value Sound volume in [0,99].
     *
     */
    void onSoundVolumeChanged(int value);

  private:
    /** \brief Helper method to connect signals and slots.
     *
     */
    void connectSignals();

    /** \brief Paints the color button with the m_color color.
     *
     */
    void updateColorButton();

    /** \brief Stops the keyboard coloring.
     *
     */
    void stopKeyboardColors();

    /** \brief Updates the events widgets depending if the selected object is a directory or not.
     * \param[in] isDirectory True if the object selected is a directory and false otherwise.
     *
     */
    void updateEventsWidgets(bool isDirectory);

    /** \brief Helper method to create the temporary file and classes for the sound.
     *
     */
    void createSoundFile();

    /** \brief Helper method to generate a color for the keyboard lights taking into
     * account the colors used by other objects.
     *
     */
    void generateColor();

    QColor                     m_color;     /** keyboard lights color.         */
    QSoundEffect              *m_sound;     /** sound class.                   */
    QTemporaryFile            *m_soundFile; /** wave file temporary file.      */
    QDir                      &m_dir;       /** last opened dir.               */
    const std::vector<Object> &m_objects;   /** list of objects being watched. */
};

#endif // ADDOBJECTDIALOG_H_
