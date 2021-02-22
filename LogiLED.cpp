/*
 File: LogiLED.cpp
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

// Project
#include <LogiLED.h>

// C++
#include <chrono>
#include <thread>

// Logitech gaming SDK
extern "C"
{
  #include <LogitechLEDLib.h>
}

using namespace LogiLed;

//--------------------------------------------------------------------
LogiLED::LogiLED()
: m_available{LogiLedInitWithName("FilesystemWatcher")}
, m_inUse{false}
{
  if(m_available)
  {
    LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
  }
}

//--------------------------------------------------------------------
LogiLED::~LogiLED()
{
  LogiLedShutdown();
}

//--------------------------------------------------------------------
LogiLED& LogiLED::getInstance()
{
  static LogiLED instance;

  return instance;
}

//--------------------------------------------------------------------
bool LogiLED::isAvailable()
{
  return getInstance().isInitialized();
}

//--------------------------------------------------------------------
void LogiLED::setColor(unsigned char r, unsigned char g, unsigned char b)
{
  if(!m_inUse)
  {
    LogiLedInitWithName("FilesystemWatcher");
    m_inUse = true;
  }

  const int red = 100 * (static_cast<float>(r)/255.);
  const int green = 100 * (static_cast<float>(g)/255.);
  const int blue = 100 * (static_cast<float>(b)/255.);

  LogiLedPulseLighting(red, green, blue, 0, 1000);
}

//--------------------------------------------------------------------
void LogiLED::stopLights()
{
  restart();
}

//--------------------------------------------------------------------
void LogiLED::restart()
{
  if(m_available)
  {
    // Apparently this is the only way to restore the default profile of the user, as there is no way
    // in the Logitech SDK to get the keys color (for us to store state) and the methods in the SDK
    // to store that info just don't work.
    LogiLedShutdown();
    m_inUse = false;
  }
}

//--------------------------------------------------------------------
std::string LogiLED::version() const
{
  int major, minor, build;
  LogiLedGetSdkVersion(&major, &minor, &build);

  return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build);
}
