#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace casio
{

inline uint32_t removed_hash_step(uint32_t h, unsigned char c)
{
    return (h ^ c) * 16777619u;
}

inline bool is_removed_function_hash(uint32_t h)
{
    switch(h) {
        case 0x0073dacfu:
        case 0x011ce908u:
        case 0x01975c10u:
        case 0x02dc2b01u:
        case 0x04fe91d1u:
        case 0x07275075u:
        case 0x08230495u:
        case 0x092855d0u:
        case 0x0dd0b0beu:
        case 0x10d2583fu:
        case 0x11420879u:
        case 0x1427b873u:
        case 0x14cdb897u:
        case 0x15c2f8ecu:
        case 0x178d4c35u:
        case 0x22fc8274u:
        case 0x23b19318u:
        case 0x23c6e902u:
        case 0x24fa4703u:
        case 0x2733f84au:
        case 0x29f5189bu:
        case 0x2f271707u:
        case 0x318b991du:
        case 0x3a7c9fcdu:
        case 0x3e6f3127u:
        case 0x3f2f216au:
        case 0x4038790bu:
        case 0x41229629u:
        case 0x41320c2du:
        case 0x4172a512u:
        case 0x47060460u:
        case 0x47a80046u:
        case 0x49f2dba0u:
        case 0x4a8d76c1u:
        case 0x4db564c2u:
        case 0x4edf1404u:
        case 0x4f04d9d2u:
        case 0x50b13859u:
        case 0x5188bedbu:
        case 0x52807cf8u:
        case 0x5319fe9eu:
        case 0x58547550u:
        case 0x58bc3fe6u:
        case 0x59bc4179u:
        case 0x5bde019bu:
        case 0x5c07d9ecu:
        case 0x5c31e95au:
        case 0x5c95b7aeu:
        case 0x5d4c2e6au:
        case 0x65a9728bu:
        case 0x66b85f2du:
        case 0x6a7d2c89u:
        case 0x6b5871d0u:
        case 0x6bff7e93u:
        case 0x6cc00459u:
        case 0x7302743fu:
        case 0x7390ec3bu:
        case 0x756f22cbu:
        case 0x77b208f6u:
        case 0x7817e298u:
        case 0x7855062eu:
        case 0x7917e42bu:
        case 0x7a0e47bau:
        case 0x7bc647b9u:
        case 0x7bcfbd8au:
        case 0x813d75aeu:
        case 0x880dac7fu:
        case 0x8a2c68b7u:
        case 0x8e9dd543u:
        case 0x8f9b7080u:
        case 0x93e97b38u:
        case 0x99d9f9f0u:
        case 0x9c662fa4u:
        case 0x9ce3039bu:
        case 0x9ede2954u:
        case 0x9f634068u:
        case 0xa1cf14d3u:
        case 0xa893baa8u:
        case 0xa9c8feadu:
        case 0xaa9b9b01u:
        case 0xac9b9e27u:
        case 0xacd224d9u:
        case 0xae13f94au:
        case 0xaea3ddb0u:
        case 0xb23e7192u:
        case 0xb27e96edu:
        case 0xb4e7f8e4u:
        case 0xb8b5368fu:
        case 0xb9c6a953u:
        case 0xba56d76du:
        case 0xbab19e4au:
        case 0xbb4555a1u:
        case 0xbbc6ac79u:
        case 0xbc3c467au:
        case 0xc03183c0u:
        case 0xc1c5c6a2u:
        case 0xc2eb0145u:
        case 0xc3e8e5f7u:
        case 0xc4c0bedeu:
        case 0xc7bd6948u:
        case 0xcada163bu:
        case 0xcae0a60au:
        case 0xcbea440au:
        case 0xce59cbb3u:
        case 0xd04036f3u:
        case 0xd2353873u:
        case 0xd3689f20u:
        case 0xd4d5660fu:
        case 0xd4f67cfdu:
        case 0xd5a84be9u:
        case 0xd64da1c4u:
        case 0xd6dfb05du:
        case 0xd6f47b66u:
        case 0xd7599ae2u:
        case 0xe0504bdeu:
        case 0xe2438311u:
        case 0xe272ae1bu:
        case 0xe44789e8u:
        case 0xe667368du:
        case 0xea8a4792u:
        case 0xedf2c855u:
        case 0xf3fa9284u:
        case 0xf45c461cu:
        case 0xf51d8813u:
        case 0xf752b052u:
        case 0xf76dddbbu:
        case 0xfcafa6eau:
        case 0xfe42bdf4u:
            return true;
    }
    return false;
}

inline bool contains_removed_function(std::string const &text)
{
    std::string folded;
    folded.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '*') continue;
        folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    uint32_t h = 2166136261u;
    bool inword = false;
    for(char raw : folded) {
        unsigned char c = static_cast<unsigned char>(raw);
        if(std::isalpha(c) || c == '_') {
            if(!inword) {
                h = 2166136261u;
                inword = true;
            }
            h = removed_hash_step(h, c);
            continue;
        }
        if(inword && std::isdigit(c)) {
            h = removed_hash_step(h, c);
            continue;
        }
        if(inword && c == '(' && is_removed_function_hash(h)) return true;
        inword = false;
    }
    return false;
}

} // namespace casio
