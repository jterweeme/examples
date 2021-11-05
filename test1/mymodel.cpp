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

