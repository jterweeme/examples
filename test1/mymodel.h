//file: mymodel.h

#include <QAbstractTableModel>

class NyModel : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
private:
    static const char *names[];
};
