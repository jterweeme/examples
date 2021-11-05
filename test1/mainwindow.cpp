//file: mainwindow.cpp

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mymodel.h"
#include "ztringlistmodel.h"
//#include <QStringListModel>

MainWindow::MainWindow() : _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);
    _myModel = new NyModel();
    _ui->bogota->setModel(_myModel);


    QStringList list;
    list << "alpha" << "bravo" << "charlie";

    ZtringListModel *model = new ZtringListModel(list);
    //_ui->bogota->setModel(model);
    list.push_back("delta");
}

MainWindow::~MainWindow()
{

}



