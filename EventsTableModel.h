/*
 File: EventsTableModel.h
 Created on: 05/02/2021
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

#ifndef EVENTSTABLEMODEL_H_
#define EVENTSTABLEMODEL_H_

// Project
#include "WatchThread.h"

// Qt
#include <QAbstractTableModel>

/** \class EventsTableModel
 * \brief Implements the model for the events table view.
 *
 */
class EventsTableModel
: public QAbstractTableModel
{
    Q_OBJECT
  public:
    /** \brief EventsTableModel
     * \param[in] p Raw pointer of the object parent of this one.
     *
     */
    explicit EventsTableModel(QObject *p = nullptr);

    /** \brief EventsTableModel class virtual destructor.
     *
     */
    virtual ~EventsTableModel()
    {};

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /** \brief Copies the list of events to the clipboard as text.
     *
     */
    void copyEventsToClipboard();

  public slots:
    /** \brief Updates the model data.
     * \param[in] obj Path of modified object.
     * \param[in] e Modification event.
     *
     */
    void modification(const std::wstring obj, const WatchThread::Event e);

    /** \brief Updates the model data.
     * \param[in] oldName Path of old object name.
     * \param[in] newName Path of new object name.
     *
     */
    void rename(const std::wstring oldName, const std::wstring newName);

  private:
    /** \brief Returns the text associated with the event.
     *
     */
    QString eventText(const WatchThread::Event &e) const;

    std::vector<std::pair<const std::wstring, const WatchThread::Event>> m_data; /** model data. */
};

#endif // EVENTSTABLEMODEL_H_
