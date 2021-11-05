//file: mymodel.cpp

#include "mymodel.h"
#include <QDebug>

const char *NyModel::names[] = {
    "John",
    "James",
    "Joyce",
    "Ronald",
    "Richard",
    "Robert",
    "Rudolf",
    "Tristan",
    "Karst",
    "Karel",
    "Carl"
};

QModelIndex NyModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

QModelIndex NyModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

int NyModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

int NyModel::rowCount(const QModelIndex &) const
{
    return 3;
}

QVariant NyModel::data(const QModelIndex &, int role) const
{
    //qDebug() << "role: " << role;

    if (role == Qt::DisplayRole)
        return QVariant("Hello world");

    return QVariant();
}

