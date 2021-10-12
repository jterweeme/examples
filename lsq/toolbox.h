//file: lsq/toolbox.h

#include <QString>

class Toolbox
{
public:
    static bool isxdigit(char c);
    static quint8 nibble2(char c);
    static char hex4(quint8 n);
    static QString hex8q(quint8 b);
    static QString hex16q(quint16 w);
    static QString hex32q(quint32 dw);
    static quint32 iPow32(quint32 base, quint32 exp);
    static quint64 iPow64(quint64 base, quint64 exp);
    static QString dec8q(quint8 b);
    static QString dec16q(quint16 w);
    static QString dec32q(quint32 dw);
    static QString dec64q(quint64 qw);
    static QString dec64sq(qint64 val);
    static QString padding(const QString &s, char c, size_t n);
};

