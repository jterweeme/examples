//file: ls/main.cpp

#include "toolbox.h"
#include <filesystem>
#include <iostream>
#include <chrono>

class Options
{
private:
public:
    void parse(int argc, char **argv);
};

void Options::parse(int argc, char **argv)
{

}

int main(int argc, char **argv)
{
    Toolbox t;
    Options o;
    o.parse(argc, argv);

    //std::filesystem::directory_iterator

    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator("."))
    {
        std::filesystem::file_time_type timep = entry.last_write_time();
        std::filesystem::_File_time_clock;
        //std::time_t time2 = std::filesystem::file_time_type::
        //std::put_time()
        uintmax_t size = entry.file_size();
        std::string sizeStr = t.padding(t.dec32(size), ' ', 9);
        std::string fn = entry.path().filename().string();
        std::cout << sizeStr << " " << "time" << " " << fn << "\r\n";
    }

    std::cout.flush();

    return 0;
}

