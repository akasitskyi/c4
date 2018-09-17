#pragma once
// https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <unordered_map>

#include "exception.hpp"

namespace c4 {
    namespace detail {
        constexpr int Nil            = 0xffffff;
        constexpr int MaxDictBits    = 16;
        constexpr int MaxDictEntries = (1 << MaxDictBits);

        template<bool enable_find>
        class Dictionary {
        public:

            struct Entry {
                uint32_t packed;
                Entry() {}
                Entry(int code, uint8_t value) : packed((uint32_t(code) << 8) | value) {}

                int code() const {
                    return int(packed >> 8);
                }

                uint8_t value() const {
                    return uint8_t(packed & 0xff);
                }
            };

            // Dictionary entries 0-255 are always reserved to the byte/ASCII range.
            int size;
            std::vector<Entry> entries;
        
            std::unordered_map<uint32_t, uint32_t> ht;

            Dictionary()
                : entries(MaxDictEntries)
            {
                size = 256;
                for (int i = 0; i < size; ++i)
                    entries[i] = Entry(Nil, (uint8_t)i);
            }

            template<class = std::enable_if<enable_find>::type>
            int findIndex(int code, uint8_t value) const {
                if (code == Nil)
                    return value;

                Entry e(code, value);

                auto it = ht.find(e.packed);
            
                return it != ht.end() ? it->second : Nil;
            }

            bool add(int code, uint8_t value) {
                assert(size < MaxDictEntries);

                Entry e(code, value);

                entries[size] = e;
                if(enable_find)
                    ht[e.packed] = size;
                ++size;

                return true;
            }

            bool flush() {
                if (size == MaxDictEntries){
                    size = 256;
                    ht.clear();
                    return true;
                }
                return false;
            }
        };
    };

    void lzw_encode(const std::uint8_t * uncompressed, int uncompressedSizeBytes, std::vector<uint16_t>& compressed) {
        using namespace detail;

        int code = Nil;
        Dictionary<true> dictionary;

        for (; uncompressedSizeBytes > 0; --uncompressedSizeBytes, ++uncompressed)
        {
            const uint8_t value = *uncompressed;
            const int index = dictionary.findIndex(code, value);

            if (index != Nil) {
                code = index;
                continue;
            }

            compressed.push_back((uint16_t)code);

            if (!dictionary.flush())
                dictionary.add(code, value);

            code = value;
        }

        if (code != Nil)
            compressed.push_back((uint16_t)code);
    }

    namespace detail {

        static void outputByte(int code, std::uint8_t *& output, int & bytesDecodedSoFar) {
            *output++ = static_cast<std::uint8_t>(code);
            ++bytesDecodedSoFar;
        }

        static void outputSequence(const Dictionary<false>& dict, int code, std::vector<uint8_t>& output, int& firstByte) {
            // A sequence is stored backwards, so we have to write
            // it to a temp then output the buffer in reverse.
            int i = 0;
            static std::vector<uint8_t> sequence(MaxDictEntries);// FIXME: not thread safe
            do {
                sequence[i++] = dict.entries[code].value();
                code = dict.entries[code].code();
            } while (code != Nil);

            firstByte = sequence[--i];

            for (; i >= 0; --i) {
                output.push_back((uint8_t)sequence[i]);
            }
        }
    };

    void lzw_decode(const std::vector<uint16_t>& compressed, std::vector<uint8_t>& uncompressed) { 
        using namespace detail;

        int prevCode      = Nil;
        int firstByte     = 0;

        Dictionary<false> dictionary;

        for(int i : c4::range(compressed)) {
            auto code = compressed[i];

            if (prevCode == Nil) {
                uncompressed.push_back((uint8_t)code);

                firstByte = code;
                prevCode  = code;
                continue;
            }

            if (code >= dictionary.size) {
                outputSequence(dictionary, prevCode, uncompressed, firstByte);

                uncompressed.push_back((uint8_t)firstByte);
            } else {
                outputSequence(dictionary, code, uncompressed, firstByte);
            }

            dictionary.add(prevCode, firstByte);
            if (dictionary.flush()) {
                prevCode = Nil;
            } else {
                prevCode = code;
            }
        }
    }
} // namespace c4
