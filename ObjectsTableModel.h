/*
 File: ObjectsTableModel.h
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

#ifndef OBJECTSTABLEMODEL_H_
#define OBJECTSTABLEMODEL_H_

// Project
#include "WatchThread.h"

// Qt
#include <QAbstractTableModel>
#include <QColor>

/** \class ObjectsTableModel
 * \brief Implements the model for the objects table view.
 *
 */
class ObjectsTableModel
: public QAbstractTableModel
{
    Q_OBJECT
  public:
    /** \brief ObjectsTableModel class constructor.
     * \param[in] p Raw pointer of the object parent of this one.
     *
     */
    explicit ObjectsTableModel(QObject *p = nullptr);

    /** \brief ObjectsTableModel class virtual destructor.
     *
     */
    virtual ~ObjectsTableModel()
    {};

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /** \brief Adds an object to the table.
     * \param[in] obj Objects path.
     *
     */
    void addObject(const QString &obj, const QColor &color);

    /** \brief Resets the number of events of the given object.
     * \param[in] obj Path of object.
     *
     */
    void resetObject(const std::wstring &obj);

    /** \brief Removes the given object from the model.
     * \param[in] obj Path of object.
     *
     */
    void removeObject(const std::wstring &obj);

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
    QString eventText(const WatchThread::Event &e);

    std::vector<std::tuple<std::wstring, std::wstring, unsigned long, QColor>> m_data; /** model data. */
};

#endif // OBJECTSTABLEMODEL_H_
