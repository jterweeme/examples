//file: ztringlistmodel.h

#ifndef ZTRINGLISTMODEL_H
#define ZTRINGLISTMODEL_H

#include <QAbstractListModel>

class ZtringListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ZtringListModel(const QStringList &strings, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
private:
    QStringList lst;
};
#endif


