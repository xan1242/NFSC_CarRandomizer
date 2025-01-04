#pragma once
#include <cstdint>
#include <string>

#ifndef COMPILETIMEHASH_HPP
#define COMPILETIMEHASH_HPP

constexpr std::size_t constexpr_strlen(const char* s)
{
    return std::char_traits<char>::length(s);
}

constexpr uint32_t load_u32(const char* str) {
    return static_cast<uint32_t>(static_cast<unsigned char>(str[0])) |
        (static_cast<uint32_t>(static_cast<unsigned char>(str[1])) << 8) |
        (static_cast<uint32_t>(static_cast<unsigned char>(str[2])) << 16) |
        (static_cast<uint32_t>(static_cast<unsigned char>(str[3])) << 24);
}

template <uint32_t V>
static constexpr uint32_t compiler_stringhash32 = V;
constexpr uint32_t stringhash32(const char* str, size_t string_length)
{
    // tell intellisense to shut up
#ifdef __INTELLISENSE__
    return 0;
#endif

    uint32_t v4 = string_length;
    uint32_t v5 = 0x9E3779B9;
    uint32_t v6 = string_length;
    uint32_t  v7 = 0x9E3779B9;
    uint32_t  a2 = 0xABCDEF00;

    // Manual extraction of bytes instead of pointer casts
    if (string_length >= 12)
    {
        uint32_t num_chunks = string_length / 12;
        for (size_t i = 0; i < num_chunks; ++i)
        {
            uint32_t chunk0 = (static_cast<uint8_t>(str[0])) |
                (static_cast<uint8_t>(str[1]) << 8) |
                (static_cast<uint8_t>(str[2]) << 16) |
                (static_cast<uint8_t>(str[3]) << 24);
            uint32_t chunk1 = (static_cast<uint8_t>(str[4])) |
                (static_cast<uint8_t>(str[5]) << 8) |
                (static_cast<uint8_t>(str[6]) << 16) |
                (static_cast<uint8_t>(str[7]) << 24);
            uint32_t chunk2 = (static_cast<uint8_t>(str[8])) |
                (static_cast<uint8_t>(str[9]) << 8) |
                (static_cast<uint8_t>(str[10]) << 16) |
                (static_cast<uint8_t>(str[11]) << 24);

            uint32_t v10 = chunk1 + v5;
            uint32_t v11 = chunk2 + a2;
            uint32_t  v12 = (v11 >> 13) ^ (v7 + chunk0 - v11 - v10);
            uint32_t v13 = (v12 << 8) ^ (v10 - v11 - v12);
            uint32_t v14 = (v13 >> 13) ^ (v11 - v13 - v12);
            uint32_t  v15 = (v14 >> 12) ^ (v12 - v14 - v13);
            uint32_t v16 = (v15 << 16) ^ (v13 - v14 - v15);
            uint32_t v17 = (v16 >> 5) ^ (v14 - v16 - v15);
            v7 = (v17 >> 3) ^ (v15 - v17 - v16);
            v5 = (v7 << 10) ^ (v16 - v17 - v7);
            a2 = (v5 >> 15) ^ (v17 - v5 - v7);
            str += 12;
            v6 -= 12;
        }
    }

    uint32_t v18 = v4 + a2;

    // Switch-case logic to handle the remaining bytes
    switch (v6)
    {
        case 11:
        {
            v18 += static_cast<uint8_t>(str[10]) << 24; [[fallthrough]];
        }
        case 10:
        {
            v18 += static_cast<uint8_t>(str[9]) << 16; [[fallthrough]];
        }
        case 9:
        {
            v18 += static_cast<uint8_t>(str[8]) << 8; [[fallthrough]];
        }
        case 8:
        {
            v5 += static_cast<uint8_t>(str[7]) << 24; [[fallthrough]];
        }
        case 7:
        {
            v5 += static_cast<uint8_t>(str[6]) << 16; [[fallthrough]];
        }
        case 6:
        {
            v5 += static_cast<uint8_t>(str[5]) << 8; [[fallthrough]];
        }
        case 5:
        {
            v5 += static_cast<uint8_t>(str[4]); [[fallthrough]];
        }
        case 4:
        {
            v7 += static_cast<uint8_t>(str[3]) << 24; [[fallthrough]];
        }
        case 3:
        {
            v7 += static_cast<uint8_t>(str[2]) << 16; [[fallthrough]];
        }
        case 2:
        {
            v7 += static_cast<uint8_t>(str[1]) << 8; [[fallthrough]];
        }
        case 1:
        {
            v7 += static_cast<uint8_t>(str[0]);         break;
        }
        default: break;
    }

    uint32_t  v19 = (v18 >> 13) ^ (v7 - v18 - v5);
    uint32_t v20 = (v19 << 8) ^ (v5 - v18 - v19);
    uint32_t v21 = (v20 >> 13) ^ (v18 - v20 - v19);
    uint32_t  v22 = (v21 >> 12) ^ (v19 - v21 - v20);
    uint32_t v23 = (v22 << 16) ^ (v20 - v21 - v22);
    uint32_t v24 = (v23 >> 5) ^ (v21 - v23 - v22);
    uint32_t  v25 = (v24 >> 3) ^ (v22 - v24 - v23);

    return (((v25 << 10) ^ (v23 - v24 - v25)) >> 15) ^ (v24 - ((v25 << 10) ^ (v23 - v24 - v25)) - v25);
}

#define STRINGHASH32(str) (compiler_stringhash32<stringhash32(str, constexpr_strlen(str))>)

#endif