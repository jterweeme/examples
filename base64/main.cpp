#include <iostream>
#include <fstream>
#include <stdint.h>

using namespace std;

static void encode(istream &is, ostream &os)
{
    char set2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (int i = 1; is; i++)
    {
        uint8_t buf[3] = {0};
        is.read((char *)buf, sizeof(buf));

        if (is.gcount() <= 0)
            break;

        uint8_t s2 = (buf[1] >> 4 | buf[0] << 4) & 0x3f;
        uint8_t s3 = ((buf[1] << 2)  | (buf[2] >> 6)) & 0x3f;
        uint8_t remainder = is.gcount() % 3;
        os.put(set2[(buf[0] >> 2) & 0x3f]);
        os.put(set2[s2]);
        os.put(remainder == 1 ? '=' : set2[s3]);
        os.put(remainder != 0 ? '=' : set2[buf[2] & 0x3f]);

        if (i % 19 == 0)
            os << "\n";
    }

    os << "\n";
}

int main(int argc, char **argv)
{
    ifstream ifs(argv[1]);
    encode(ifs, cout);
    ifs.close();
    return 0;
}



