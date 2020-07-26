#include "toolbox.h"

char Toolbox::nibble(BYTE n)
{
    return n <= 9 ? '0' + char(n) : 'A' + char(n - 10);
}

std::string Toolbox::hex8(BYTE b)
{
    std::string ret;
    ret += nibble(b >> 4 & 0xf);
    ret += nibble(b >> 0 & 0xf);
    return ret;
}

void Toolbox::hex8(std::ostream &os, BYTE b)
{
    os.put(nibble(b >> 4 & 0xf));
    os.put(nibble(b >> 0 & 0xf));
}

std::string Toolbox::hex16(WORD w)
{
    std::string ret;
    ret += hex8(w >> 8 & 0xff);
    ret += hex8(w >> 0 & 0xff);
    return ret;
}

std::string Toolbox::hex32(DWORD dw)
{
    std::string ret;
    ret += hex16(dw >> 16 & 0xffff);
    ret += hex16(dw >>  0 & 0xffff);
    return ret;
}

std::string Toolbox::hex64(DWORD64 dw64)
{
    std::string ret;
    ret += hex32(dw64 >> 32 & 0xffffffff);
    ret += hex32(dw64 >>  0 & 0xffffffff);
    return ret;
}

std::wstring Toolbox::strtowstr(const std::string &s)
{
    std::wstring ret;
    size_t len = s.length();

    for (size_t i = 0; i < len; i++)
        ret.push_back(wchar_t(s.at(i)));

    return ret;
}

void Toolbox::hexdump(std::ostream &os, BYTE *data, DWORD len)
{
    for (DWORD i = 0; i < len; i += 16)
    {
        DWORD j = i;

        while (j < i + 16 && j < len)
        {
            hex8(os, data[j++]);
            os.put(' ');
        }

        while (j < i + 16)
        {
            os << "   ";
            j++;
        }

        for (j = i; j < i + 16 && j < len; j++)
        {
            if (isprint(data[j]))
                os.put(data[j]);
            else
                os.put('.');
        }

        os << "\r\n";
    }
}

void Toolbox::messageBox(HINSTANCE hInstance, HWND hwnd, UINT errId, UINT captionId)
{
    TCHAR err[80], caption[80];
    ::LoadString(hInstance, errId, err, sizeof(err));
    ::LoadString(hInstance, captionId, caption, sizeof(caption));
    ::MessageBox(hwnd, err, caption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
}

void Toolbox::errorBox(HINSTANCE hInstance, HWND hwnd, UINT errId)
{
    TCHAR err[80];
    ::LoadString(hInstance, errId, err, sizeof(err));
    ::MessageBox(hwnd, err, TEXT("Error"), MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
}


