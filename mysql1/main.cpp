//file: mysql1/main.cpp

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <stdexcept>


int main(int argc, char **argv)
{
    bool ret;
    QCoreApplication a(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("nederland");
    db.setUserName("jasper");
    db.setPassword("onzin35");

    try
    {
        ret = db.open();

        if (ret == false)
            throw std::runtime_error("Cannot open database");

        QSqlQuery query(db);
        ret = query.exec("SELECT * FROM plaats");

        if (ret == false)
            throw std::runtime_error("Cannot query");
    }
    catch (...)
    {
        qDebug() << "Unknown exception";
    }


    return a.exec();
}

