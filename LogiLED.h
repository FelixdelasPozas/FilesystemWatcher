/*
 File: LogiLED.h
 Created on: 21/11/2018
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

#ifndef LOGILED_H_
#define LOGILED_H_

// C++
#include <memory>

// Qt
#include <QObject>
#include <QString>
#include <QColor>
#include <QReadWriteLock>

/** \class LogiLED
 * \brief Interface to Logitech Gaming LED SDK
 *
 */
class LogiLED
{
  public:
    /** \brief LogiLED class virtual destructor.
     *
     */
    virtual ~LogiLED();

    /** \brief Gets the LogiLED singleton instance.
     *
     */
    static LogiLED & getInstance();

    /** \brief Deleted copy constructor to avoid copying the singleton.
     *
     */
    LogiLED(LogiLED const&) = delete;

    /** \brief Deleted operator= to avoid copying the singleton.
     *
     */
    void operator=(LogiLED const&) = delete;

    /** \brief Returns true if the interface to keyboard lights is available and false otherwise.
     *
     */
    static bool isAvailable();

    /** \brief Sets the color of the keyboard. It will pulse until stopLights() is called.
     * \param[in] r Red value in [0,255]
     * \param[in] g Green value in [0,255]
     * \param[in] b Blue value in [0,255]
     *
     */
    void setColor(unsigned char r, unsigned char g, unsigned char b);

    /** \brief Stops the keyboard lights.
     *
     */
    void stopLights();

    bool isInUse()
    { return m_inUse; }

  private:
    /** \brief LogilLED class private constructor.
     *
     */
    LogiLED();

    /** \brief Returns true if the interface to keyboard lights is available and false otherwise.
     *
     */
    const bool isInitialized() const
    { return m_available; }

    /** \brief Shutdowns and re-initializes the session to restore default keyboard lights.
     *
     */
    void restart();

    bool m_available; /** true if the LogiLED API is available and initialized. */
    bool m_inUse;     /** true if using lights, false otherwise.                */
};

#endif // LOGILED_H_
