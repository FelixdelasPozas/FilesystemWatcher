/*
 File: FilesystemWatcher.h
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

#ifndef FILESYSTEMWATCHER_H_
#define FILESYSTEMWATCHER_H_

#include "ui_filesystemWatcher.h"

// Qt
#include <QDialog>
#include <QSystemTrayIcon>

// Project
#include "AddObjectDialog.h"
#include "WatchThread.h"

class QCloseEvent;
class QSoundEffect;
class QTemporaryFile;
class Object;

/** \class FilesystemWatcher
 * \brief Implements the main dialog of the application.
 *
 */
class FilesystemWatcher
: public QDialog
, private Ui::FilesystemWatcher
{
    Q_OBJECT
  public:
    /** \brief FilesystemWatcher class constructor.
     * \param[in] p Raw pointer of the widget parent of this one.
     * \param[in] f Dialog flags.
     *
     */
    explicit FilesystemWatcher(QWidget *p = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief FilesystemWatcher class virtual destructor.
     *
     */
    virtual ~FilesystemWatcher();

  protected:
    virtual void closeEvent(QCloseEvent *e);

  public slots:
    virtual void done(int) override
    { close(); }

    virtual void accept() override
    { close(); }

    virtual void reject() override
    { close(); }

  private slots:
    /** \brief Closes the application.
     *
     */
    void quitApplication();

    /** \brief Restores the main dialog if the user double-clicks the tray icon.
     * \param[in] reason Tray icon activation reason.
     *
     */
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::DoubleClick);

    /** \brief Shows the about dialog.
     *
     */
    void onAboutButtonClicked();

    /** \brief Shows the file selection dialog.
     *
     */
    void onAddObjectButtonClicked();

    /** \brief Copies the events contents to the clipboard.
     *
     */
    void onCopyButtonClicked();

    /** \brief Warns the user about an error.
     * \param[in] message Error message.
     *
     */
    void onWatcherError(const QString message);

    /** \brief Alarms the user about an event.
     * \param[in] object Object name.
     * \param[in] e Event.
     *
     */
    void onModification(const std::wstring object, const Events e);

    /** \brief Updates the internal data about the object and warns the user of an event.
     * \param[in] oldName Object old name.
     * \param[in] newName Object new name.
     *
     */
    void onRename(const std::wstring oldName, const std::wstring newName);

    /** \brief Animates the tray icon.
     *
     */
    void updateTrayIcon();

    /** \brief Stops the lights and sound alarms.
     *
     */
    void stopAlarms();

    /** \brief Enables/disables the reset and delete object button when user selects items on the table.
     *
     */
    void onSelectionChanged();

    /** \brief Resets the events number of the selected object.
     *
     */
    void onResetButtonClicked();

    /** \brief Removes the selected object in the objects table.
     *
     */
    void onRemoveButtonClicked();

    /** \brief Displays the context menu for the objects table.
     * \param[in] p Point where the context menu was requested.
     *
     */
    void onCustomMenuRequested(const QPoint &p);

    /** \brief Modifies the UI when mute action is checked.
     *
     */
    void onMuteActionClicked();

  private:
    /** \brief Helper method to connect signals to slots in the dialog.
     *
     */
    void connectSignals();

    /** \brief Helper method to setup and connect the tray icon.
     *
     */
    void setupTrayIcon();

    /** \brief Helper method that loads application settings from the registry.
     *
     */
    void loadSettings();

    /** \brief Helper method that saves application settings to the registry.
     *
     */
    void saveSettings();

    /** \brief Writes the given message to the log tab.
     * \param[in] message Text message.
     *
     */
    void log(const QString &message);

    /** \brief Shows an alarm message to the user. Returns true if is able to show the message and false otherwise.
     * \param[in] title Window title.
     * \param[in] message Text message.
     *
     */
    bool showMessage(const QString title, const QString message);

    /** \brief Sounds the appropiate alarms. 
     * \param[in] hasSound True if the event has sound alarm.
     * \param[in] hasLights True if the event has lights alarm.
     * \param[in] hasMessage True if the event has message alarm. 
     * \param[in] obj Object of the event.
     * \param[in] type Event type. 
     * 
     */
    void soundAlarms(bool hasSound, bool hasLights, bool hasMessage, Object &obj, const Events type);

    QSystemTrayIcon    *m_trayIcon;    /** tray icon.                                      */
    bool                m_needsExit;   /** true to close the application, false otherwise. */
    std::vector<Object> m_objects;     /** list of watched objects.                        */
    QAction            *m_stopAction;  /** stop alarms tray menu action.                   */
    QSoundEffect       *m_alarmSound;  /** alarm sound.                                    */
    QTemporaryFile     *m_soundFile;   /** temporary file for alarm wav file.              */
    QDir                m_lastDir;     /** last opened dir to select objects.              */
    unsigned char       m_alarmVolume; /** volume of the sound alarm [0-100].              */
    AlarmFlags          m_alarmFlags;  /** default alarms for add object dialog.           */
    Events              m_events;      /** default events for add object dialog.           */
};

/** \class Object
 * \brief Contains the data of an object to watch.
 *
 */
class Object
{
  public:
    /** \brief Returns the alarms that will be triggered.
     *
     */
    AlarmFlags getAlarms() const
    { return alarms; }

    /** \brief Returns the color of the keyboard alarm.
     *
     */
    const QColor& getColor() const
    { return color; }

    /** \brief Returns the events being watched.
     *
     */
    Events getEvents() const
    { return events; }

    /** \brief Returns the number of events watched till now.
     *
     */
    unsigned long getEventsNumber() const
    { return eventsNumber; }

    /** \brief Returs the path of the object.
     *
     */
    std::filesystem::path getPath() const
    { return path; }

    /** \brief Returns the volume of the sound alarm.
     *
     */
    unsigned char getVolume() const
    { return volume; }

    /** \brief Returns true if the object is in alarm mode.
     * 
     */
    bool isInAlarm() const
    { return inAlarm; }

    /** \brief Set the object in alarm or not.
     * \param[in] value True to set in alarm and false otherwise. 
     * 
     */
    void setIsInAlarm(const bool value)
    { inAlarm = value; }

  private:
    /** \brief Object class constructor.
     * \param[in] objectPath  Filesystem path of the object.
     * \param[in] alarmFlags  Alarms to trigger when the object changes.
     * \param[in] lightsColor Color to use for the keyboard alarm.
     * \param[in] alarmVolume Volume of the sound alarm.
     * \param[in] watchEvents Events to watch for modification.
     * \param[in] t           Pointer to the watcher thread.
     *
     */
    Object(const std::wstring &objectPath, const AlarmFlags alarmFlags,
           const QColor &lightsColor, const unsigned char alarmVolume,
           const Events watchEvents, WatchThread *t)
    : path{objectPath}, alarms{alarmFlags}, color{lightsColor},
      volume{alarmVolume}, events{watchEvents}, thread{t},
      eventsNumber{0}, inAlarm{false}
      {};

    std::filesystem::path path;         /** object path.                      */
    AlarmFlags            alarms;       /** alarms for the user.              */
    QColor                color;        /** color for keyboard alarm.         */
    unsigned char         volume;       /** volume of sound alarm in [1-100]. */
    Events                events;       /** events to watch.                  */
    WatchThread          *thread;       /** watcher thread.                   */
    unsigned long         eventsNumber; /** number of registed events.        */
    bool                  inAlarm;      /** true if currently in alarm mode.  */

    friend class FilesystemWatcher;
};


#endif // FILESYSTEMWATCHER_H_
