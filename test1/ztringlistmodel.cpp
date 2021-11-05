//file: ztringlistmodel.cpp

#include "ztringlistmodel.h"

ZtringListModel::ZtringListModel(const QStringList &strings, QObject *parent)
    : QAbstractListModel(parent), lst(strings)
{
}

int ZtringListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : lst.count();
}

QVariant ZtringListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= lst.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return lst.at(index.row());

    return QVariant();
}


