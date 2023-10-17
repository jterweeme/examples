/*
Naieve manier om binaire STL om te zetten naar Wavefront *.obj
*/

#include <iostream>
#include <cstdint>

template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

using std::cin;

float rf()
{
    return readX<float>(cin);
}

int main()
{
    using std::cout;
    cin.ignore(80);
    uint32_t n_triangles = readX<uint32_t>(cin);
    int k = 1, l = 1;
    
    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        cout << "vn " << rf() << " " << rf() << " " << rf() << "\r\n";

        for (uint32_t j = 0; j < 3; ++j)
            cout << "v " << rf() << " " << rf() << " " << rf() << "\r\n";

        cin.ignore(2);
        cout << "f " << k++ << "//" << l << " ";
        cout << k++ << "//" << l << " ";
        cout << k++ << "//" << l++ << "\r\n";
    }

    return 0;
}


