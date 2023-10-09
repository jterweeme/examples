#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

static const char *
encoding_table = "ABCDEFGHIJKLMNOPQRStUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *decoding_table = NULL;

char *base64_encode(const uint8_t *data, size_t input_length, size_t *output_length)
{
    static int mod_table[] = {0, 2, 1};
    *output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = (char *)malloc(*output_length);

    if (encoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

static char nibble(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'A' + char (n - 10);
}

static std::string hex8(uint8_t b)
{
    std::string ret;
    ret += nibble(b >> 4 & 0xf);
    ret += nibble(b >> 0 & 0xf);
    return ret;
}

void build_decoding_table()
{
    decoding_table = (char *)malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;

    for (int i = 0; i < 256; ++i)
    {
        //std::cerr << hex8(decoding_table[i]) << "\r\n";
    }
}

uint8_t *base64_decode(const char *data, size_t input_length, size_t *output_length)
{
    if (decoding_table == NULL)
        build_decoding_table();

    if (input_length % 4 != 0)
        return NULL;

    *output_length = input_length / 4 * 3;

    if (data[input_length - 1] == '=')
        (*output_length)--;

    if (data[input_length - 2] == '=')
        (*output_length)--;

    uint8_t *decoded_data = (uint8_t *)malloc(*output_length);

    if (decoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

void base64_cleanup() {
    free(decoding_table);
}

int main()
{
    const char *input = "alpha bravo charlie";
    size_t len;
    char *output = base64_encode((uint8_t *)(input), strlen(input), &len);
    std::cerr << input << "\r\n";
    std::cerr << output << "\r\n";
    uint8_t *buf;
    buf = base64_decode(output, strlen(output), &len);
    std::cerr << buf << "\r\n";
    return 0;
}


