//file: main.cpp

#include "toolbox.h"
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QDateTime>

static QString fileInfoPrinter(const QFileInfo &fi)
{
    Toolbox t;
    QString ret;

    if (fi.isDir())
        ret.push_back('d');
    else
        ret.push_back('-');

    ret.append("---------");
    ret.push_back(' ');
    QString filesize = t.dec64sq(fi.size());
    ret.append(t.padding(filesize, ' ', 9));
    ret.push_back(' ');
    QDateTime filetime = fi.lastModified();
    ret.append(filetime.toString());
    ret.push_back(' ');
    ret.append(fi.fileName());
    return ret;
}

int main(int argc, char **argv)
{
    QTextStream qis(stdin);
    QTextStream qos(stdout);
    QString pwd = QDir::currentPath();
    //qos << pwd << "\r\n";
    //qos << Toolbox::dec64sq(-123456789) << "\r\n";

    QDirIterator dit(pwd);

    while (dit.hasNext())
    {
        dit.next();
        QFileInfo fi = dit.fileInfo();
        qos << fileInfoPrinter(fi) << "\r\n";
    }

    qos.flush();
    return 0;
}

