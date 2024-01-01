#include <stdint.h>

uint32_t h0, h1, h2, h3;

int main()
{
    FILE *fp = stdin;
    uint8_t data[64];
    

    for (size_t n; (n = fread(data, 1, 64, fp)) > 0;)
    {
        if (n < 56)
        {
            data[n] = 0x80;
            
                
        }
    }
}


