//file: mainwindow.cpp

#include "mainwindow.h"
#include <QLayout>
#include <QToolButton>
#include <QButtonGroup>

MainWindow::MainWindow()
{
    QToolButton *button1 = new QToolButton(this);
    button1->setText("Onzin");
    button1->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button1->setIcon(QIcon(":/images/globe"));
    button1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    button1->setFixedHeight(100);
    layout()->addWidget(button1);
    QPixmap p;
}

