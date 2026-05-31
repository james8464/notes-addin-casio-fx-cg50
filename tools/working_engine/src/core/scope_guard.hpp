#pragma once

#if defined(TARGET_PRIZM)
#include "device/casio_compat.hpp"
#else
#include <cctype>
#include <cstdint>
#include <string>
#endif

namespace casio
{

inline uint32_t removed_hash_step(uint32_t h, unsigned char c)
{
    return (h ^ c) * 16777619u;
}

inline bool is_removed_function_hash(uint32_t h)
{
    switch(h) {
        case 0x0b11b072u:
        case 0xbcefb4d8u:
        case 0xf5cf8c7du:
        case 0xaa7d7949u:
        case 0x11c2662du:
        case 0xdb9215fdu:
        case 0x8117ae3du:
        case 0xc7e16877u:
        case 0x0073dacfu:
        case 0x011ce908u:
        case 0x018ead2eu:
        case 0x01975c10u:
        case 0x02dc2b01u:
        case 0x04e0311fu:
        case 0x04fe91d1u:
        case 0x05f85713u:
        case 0x06bd505bu:
        case 0x071515d2u:
        case 0x07275075u:
        case 0x08230495u:
        case 0x08c21d83u:
        case 0x092855d0u:
        case 0x0ad0ed6cu:
        case 0x0c82e56bu:
        case 0x0dd0b0beu:
        case 0x0fad7a44u:
        case 0x10d2583fu:
        case 0x11420879u:
        case 0x1413edcdu:
        case 0x1427b873u:
        case 0x14cdb897u:
        case 0x15c2f8ecu:
        case 0x1642b069u:
        case 0x1696d742u:
        case 0x178d4c35u:
        case 0x17d1d6fcu:
        case 0x18ba068bu:
        case 0x198be6b8u:
        case 0x1b5e2979u:
        case 0x1b777caeu:
        case 0x1bbede44u:
        case 0x1eab292au:
        case 0x1ebe2c6au:
        case 0x1f82d9a8u:
        case 0x22fc8274u:
        case 0x23b19318u:
        case 0x23c6e902u:
        case 0x24fa4703u:
        case 0x262cfcabu:
        case 0x26851732u:
        case 0x2733f84au:
        case 0x2b8eb358u:
        case 0x2ba548a6u:
        case 0x2d5f622cu:
        case 0x2f271707u:
        case 0x318b991du:
        case 0x31bd46a1u:
        case 0x38185d76u:
        case 0x39888dd9u:
        case 0x3a64d347u:
        case 0x3a7c9fcdu:
        case 0x3e6f3127u:
        case 0x3f2f216au:
        case 0x4038790bu:
        case 0x41229629u:
        case 0x41320c2du:
        case 0x4143b863u:
        case 0x4172a512u:
        case 0x4188b5ddu:
        case 0x440cf8a1u:
        case 0x44c63840u:
        case 0x46b93feau:
        case 0x45855c00u:
        case 0x462f5ba1u:
        case 0x47060460u:
        case 0x47a80046u:
        case 0x4988a709u:
        case 0x49c43198u:
        case 0x49f2dba0u:
        case 0x4a8d76c1u:
        case 0x4abab91fu:
        case 0x4caa143cu:
        case 0x4ceb35d5u:
        case 0x4db564c2u:
        case 0x4edf1404u:
        case 0x4f04d9d2u:
        case 0x503e3086u:
        case 0x50b13859u:
        case 0x51295e73u:
        case 0x5188bedbu:
        case 0x519b7f08u:
        case 0x52807cf8u:
        case 0x5319fe9eu:
        case 0x53fd0c42u:
        case 0x5683a798u:
        case 0x5824dcf7u:
        case 0x58547550u:
        case 0x58bc3fe6u:
        case 0x59bc4179u:
        case 0x5bde019bu:
        case 0x5c07d9ecu:
        case 0x5c31e95au:
        case 0x5c95b7aeu:
        case 0x5d0e6938u:
        case 0x5d36cd35u:
        case 0x5d4c2e6au:
        case 0x5d7168a6u:
        case 0x5ea077c6u:
        case 0x614ba208u:
        case 0x623beeebu:
        case 0x633cdd81u:
        case 0x68365378u:
        case 0x65a9728bu:
        case 0x66b85f2du:
        case 0x6a7d2c89u:
        case 0x6b5871d0u:
        case 0x6bff7e93u:
        case 0x6c04a192u:
        case 0x6cc00459u:
        case 0x6e72bdcfu:
        case 0x6eb56479u:
        case 0x6ef4d9bdu:
        case 0x6f20f7ddu:
        case 0x7302743fu:
        case 0x7390ec3bu:
        case 0x756f22cbu:
        case 0x77b208f6u:
        case 0x7817e298u:
        case 0x7855062eu:
        case 0x78978610u:
        case 0x7917e42bu:
        case 0x7a0e47bau:
        case 0x7bc647b9u:
        case 0x7bcfbd8au:
        case 0x7d033966u:
        case 0x7d3ff927u:
        case 0x7d6fc026u:
        case 0x7e8a3329u:
        case 0x808153e1u:
        case 0x8132f281u:
        case 0x813d75aeu:
        case 0x81df5ea5u:
        case 0x856d7f49u:
        case 0x880dac7fu:
        case 0x8a2c68b7u:
        case 0x8bd5ceabu:
        case 0x8bfaffcfu:
        case 0x8d2519cbu:
        case 0x8e816a60u:
        case 0x8e9dd543u:
        case 0x8f605233u:
        case 0x8f9b7080u:
        case 0x8fae83d8u:
        case 0x90d2e37au:
        case 0x91708e0cu:
        case 0x93e97b38u:
        case 0x940b5e09u:
        case 0x98a2959eu:
        case 0x991301e0u:
        case 0x99618446u:
        case 0x99d9f9f0u:
        case 0x9c662fa4u:
        case 0xa3ad2dc9u:
        case 0x9c90b906u:
        case 0x9ce3039bu:
        case 0x9e888b98u:
        case 0x9ede2954u:
        case 0x9f634068u:
        case 0xa1cf14d3u:
        case 0xa3325c6fu:
        case 0xa71467f9u:
        case 0xa893baa8u:
        case 0xa9c8feadu:
        case 0xaa9b9b01u:
        case 0xab66535cu:
        case 0xac9b9e27u:
        case 0xacd224d9u:
        case 0xacda80fbu:
        case 0xae13f94au:
        case 0xaea3ddb0u:
        case 0xaf645378u:
        case 0xb1c0e744u:
        case 0xb1e15f65u:
        case 0xb23e7192u:
        case 0xb27e96edu:
        case 0xb3dab835u:
        case 0xb4423af0u:
        case 0xb469c380u:
        case 0xb4e7f8e4u:
        case 0xb5fc9604u:
        case 0xb8b5368fu:
        case 0xb9c6a953u:
        case 0xbdeeca89u:
        case 0xba56d76du:
        case 0xbab19e4au:
        case 0xbb4555a1u:
        case 0xbbc6ac79u:
        case 0xbc3c467au:
        case 0xc03183c0u:
        case 0xc1c5c6a2u:
        case 0xc2eb0145u:
        case 0xc352e04au:
        case 0xc3e8e5f7u:
        case 0xc4c0bedeu:
        case 0xc7bd6948u:
        case 0xcada163bu:
        case 0xcae0a60au:
        case 0xcb08e2d8u:
        case 0xcb9ba81du:
        case 0xcbea440au:
        case 0xcc30f8fau:
        case 0xccee2183u:
        case 0xce2ab760u:
        case 0xce59cbb3u:
        case 0xcf9fd8f0u:
        case 0xd04036f3u:
        case 0xd2353873u:
        case 0xd4d5660fu:
        case 0xd4f0716cu:
        case 0xd4f67cfdu:
        case 0xd57925a4u:
        case 0xd5a84be9u:
        case 0xd5b7b015u:
        case 0xd64da1c4u:
        case 0xd6dfb05du:
        case 0xd6f47b66u:
        case 0xd7037e9bu:
        case 0xd7599ae2u:
        case 0xd81f50a6u:
        case 0xd9c30496u:
        case 0xdd4a6ba7u:
        case 0xde375a53u:
        case 0xe0504bdeu:
        case 0xe2438311u:
        case 0xe26424d0u:
        case 0xe272ae1bu:
        case 0xe49f78d1u:
        case 0xe44789e8u:
        case 0xe667368du:
        case 0xe80fa444u:
        case 0xe848572au:
        case 0xea8a4792u:
        case 0xeb40ffd3u:
        case 0xebd12f63u:
        case 0xec7aec99u:
        case 0xedf2c855u:
        case 0xee43b913u:
        case 0xee65cbdau:
        case 0xeea7e9ccu:
        case 0xf28765e9u:
        case 0xf2c80d5bu:
        case 0xf3c7dd11u:
        case 0xf3fa9284u:
        case 0xf45c461cu:
        case 0xf51d8813u:
        case 0xf752b052u:
        case 0xf76dddbbu:
        case 0xf8aaffd3u:
        case 0xf9950dffu:
        case 0xfba40b66u:
        case 0xfcaee333u:
        case 0xfcafa6eau:
        case 0xfd455c3bu:
        case 0xfe42bdf4u:
        case 0xfeeb57a9u:
        case 0x65c08844u:
        case 0x9e99b437u:
        case 0xa1ed81c2u:
        case 0xfceb725bu:
        case 0xeb2af50bu:
        case 0x9c265311u:
        case 0x62e5b688u:
        case 0x3bea1b45u:
        case 0x6ce3e74du:
        case 0x69e3e294u:
        case 0xdef6fa2au:
        case 0x9ebaa335u:
        case 0x6cf8e81cu:
        case 0x8c496ab8u:
        case 0x616bf796u:
        case 0x102c19eau:
        case 0xbf5074c1u:
        case 0xac5195bau:
        case 0xd51ec1fdu:
        case 0xc109695bu:
        case 0xad7f573cu:
        case 0xcedfa3c5u:
        case 0xbe269f5cu:
        case 0x6e12076fu:
        case 0xa8aef257u:
        case 0x78fda3ffu:
        case 0xfbf05517u:
        case 0xd46fb0cfu:
        case 0xea42b557u:
        case 0x826fd909u:
        case 0x1f949aa9u:
        case 0xa21ca480u:
        case 0x4c515decu:
        case 0xbde0f6e0u:
        case 0x78b0ef32u:
        case 0xea8bdf11u:
        case 0x2bd14411u:
        case 0xb5bda71fu:
        case 0x89b28834u:
        case 0x9b4bf7b0u:
        case 0x28c49dd1u:
        case 0xb760552au:
        case 0x83092c31u:
        case 0x11a3508au:
        case 0x94b9211bu:
        case 0x6371dcb0u:
        case 0xd20a0c9du:
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
