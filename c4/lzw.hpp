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
        inline int nextPowerOfTwo(int num) {
            --num;
            for (std::size_t i = 1; i < sizeof(num) * 8; i <<= 1)
                num = num | num >> i;

            return ++num;
        }

        class BitStreamWriter final
        {
        public:

            BitStreamWriter(const BitStreamWriter &) = delete;
            BitStreamWriter& operator = (const BitStreamWriter &) = delete;

            BitStreamWriter() {
                // 8192 bits for a start (1024 bytes). It will resize if needed.
                // Default granularity is 2.
                internalInit();
                allocate(8192);
            }

            void allocate(int bitsWanted) {
                // Require at least a byte.
                if (bitsWanted <= 0)
                    bitsWanted = 8;

                // Round upwards if needed:
                if ((bitsWanted % 8) != 0)
                    bitsWanted = nextPowerOfTwo(bitsWanted);

                // We might already have the required count.
                const int sizeInBytes = bitsWanted / 8;
                if (sizeInBytes <= bytesAllocated)
                    return;

                stream = allocBytes(sizeInBytes, stream, bytesAllocated);
                bytesAllocated = sizeInBytes;
            }

            void setGranularity(int growthGranularity) {
                granularity = (growthGranularity >= 2) ? growthGranularity : 2;
            }

            std::uint8_t * release() {
                std::uint8_t * oldPtr = stream;
                internalInit();
                return oldPtr;
            }


            void appendBit(int bit) {
                const std::uint32_t mask = std::uint32_t(1) << nextBitPos;
                stream[currBytePos] = (stream[currBytePos] & ~mask) | (-bit & mask);
                ++numBitsWritten;

                if (++nextBitPos == 8) {
                    nextBitPos = 0;
                    if (++currBytePos == bytesAllocated) {
                        allocate(bytesAllocated * granularity * 8);
                    }
                }
            }


            void appendBitsU64(std::uint64_t num, int bitCount) {
                assert(bitCount <= 64);
                for (int b = 0; b < bitCount; ++b) {
                    const std::uint64_t mask = std::uint64_t(1) << b;
                    const int bit = !!(num & mask);
                    appendBit(bit);
                }
            }


            int getByteCount() const {
                int usedBytes = numBitsWritten / 8;
                int leftovers = numBitsWritten % 8;
                if (leftovers != 0)
                    ++usedBytes;
                assert(usedBytes <= bytesAllocated);
                return usedBytes;
            }

            int getBitCount()  const {
                return numBitsWritten;
            }

            const std::uint8_t * getBitStream() const {
                return stream;
            }


            ~BitStreamWriter() {
                if (stream != nullptr)
                    std::free(stream);
            }


        private:

            void internalInit() {
                stream = nullptr;
                bytesAllocated = 0;
                granularity = 2;
                currBytePos = 0;
                nextBitPos = 0;
                numBitsWritten = 0;
            }

            static std::uint8_t * allocBytes(int bytesWanted, std::uint8_t * oldPtr, int oldSize) {
                std::uint8_t * newMemory = static_cast<std::uint8_t *>(std::malloc(bytesWanted));
                std::memset(newMemory, 0, bytesWanted);

                if (oldPtr != nullptr){
                    std::memcpy(newMemory, oldPtr, oldSize);
                    std::free(oldPtr);
                }

                return newMemory;
            }


            std::uint8_t * stream; // Growable buffer to store our bits. Heap allocated & owned by the class instance.
            int bytesAllocated;    // Current size of heap-allocated stream buffer *in bytes*.
            int granularity;       // Amount bytesAllocated multiplies by when auto-resizing in appendBit().
            int currBytePos;       // Current byte being written to, from 0 to bytesAllocated-1.
            int nextBitPos;        // Bit position within the current byte to access next. 0 to 7.
            int numBitsWritten;    // Number of bits in use from the stream buffer, not including byte-rounding padding.
        };

        class BitStreamReader final
        {
        public:

            // No copy/assignment.
            BitStreamReader(const BitStreamReader &) = delete;
            BitStreamReader & operator = (const BitStreamReader &) = delete;

            BitStreamReader(const std::uint8_t * bitStream, int byteCount, int bitCount)
                : stream(bitStream)
                , sizeInBytes(byteCount)
                , sizeInBits(bitCount)
            {
                currBytePos = 0;
                nextBitPos = 0;
                numBitsRead = 0;
            }


            bool isEndOfStream() const {
                return numBitsRead >= sizeInBits;
            }

            bool readNextBit(int & bitOut) {
                if (numBitsRead >= sizeInBits)
                {
                    return false; // We are done.
                }

                const std::uint32_t mask = std::uint32_t(1) << nextBitPos;
                bitOut = !!(stream[currBytePos] & mask);
                ++numBitsRead;

                if (++nextBitPos == 8)
                {
                    nextBitPos = 0;
                    ++currBytePos;
                }
                return true;
            }

            std::uint64_t readBitsU64(int bitCount) {
                assert(bitCount <= 64);

                std::uint64_t num = 0;
                for (int b = 0; b < bitCount; ++b)
                {
                    int bit;
                    if (!readNextBit(bit))
                        THROW_EXCEPTION("Failed to read bits from stream! Unexpected end.");

                    // Based on a "Stanford bit-hack":
                    // http://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
                    const std::uint64_t mask = std::uint64_t(1) << b;
                    num = (num & ~mask) | (-bit & mask);
                }

                return num;
            }

        private:

            const std::uint8_t * stream; // Pointer to the external bit stream. Not owned by the reader.
            const int sizeInBytes;       // Size of the stream *in bytes*. Might include padding.
            const int sizeInBits;        // Size of the stream *in bits*, padding *not* include.
            int currBytePos;             // Current byte being read in the stream.
            int nextBitPos;              // Bit position within the current byte to access next. 0 to 7.
            int numBitsRead;             // Total bits read from the stream so far. Never includes byte-rounding padding.
        };

        constexpr int Nil            = 0xffffff;
        constexpr int MaxDictBits    = 16;
        constexpr int StartBits      = 9;
        constexpr int FirstCode      = (1 << (StartBits - 1));
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

                friend inline bool operator==(const Entry& a, const Entry& b) {
                    return a.packed == b.packed;
                }
            };

            // Dictionary entries 0-255 are always reserved to the byte/ASCII range.
            int size;
            std::vector<Entry> entries;
        
            std::unordered_map<uint32_t, uint32_t> ht;

            Dictionary()
                : entries(MaxDictEntries)
            {
                // First 256 dictionary entries are reserved to the byte/ASCII
                // range. Additional entries follow for the character sequences
                // found in the input. Up to 4096 - 256 (MaxDictEntries - FirstCode).
                size = FirstCode;
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

            bool flush(int & codeBitsWidth) {
                if (size == (1 << codeBitsWidth))
                {
                    ++codeBitsWidth;
                    if (codeBitsWidth > MaxDictBits)
                    {
                        // Clear the dictionary (except the first 256 byte entries).
                        codeBitsWidth = StartBits;
                        size = FirstCode;
                        ht.clear();
                        return true;
                    }
                }
                return false;
            }

        };
    };

    void lzw_encode(const std::uint8_t * uncompressed, int uncompressedSizeBytes, std::uint8_t ** compressed, int * compressedSizeBytes, int * compressedSizeBits) {
        using namespace detail;

        if (uncompressed == nullptr || compressed == nullptr)
            THROW_EXCEPTION("lzw::easyEncode(): Null data pointer(s)!");

        if (uncompressedSizeBytes <= 0 || compressedSizeBytes == nullptr || compressedSizeBits == nullptr)
            THROW_EXCEPTION("lzw::easyEncode(): Bad in/out sizes!");

        int code = Nil;
        int codeBitsWidth = StartBits;
        Dictionary<true> dictionary;

        BitStreamWriter bitStream;

        for (; uncompressedSizeBytes > 0; --uncompressedSizeBytes, ++uncompressed)
        {
            const uint8_t value = *uncompressed;
            const int index = dictionary.findIndex(code, value);

            if (index != Nil) {
                code = index;
                continue;
            }

            bitStream.appendBitsU64(code, codeBitsWidth);

            if (!dictionary.flush(codeBitsWidth))
                dictionary.add(code, value);

            code = value;
        }

        if (code != Nil)
            bitStream.appendBitsU64(code, codeBitsWidth);

        *compressedSizeBytes = bitStream.getByteCount();
        *compressedSizeBits  = bitStream.getBitCount();
        *compressed          = bitStream.release();
    }

    namespace detail {

        static bool outputByte(int code, std::uint8_t *& output, int outputSizeBytes, int & bytesDecodedSoFar) {
            if (bytesDecodedSoFar >= outputSizeBytes)
                THROW_EXCEPTION("Decoder output buffer too small!");

            assert(code >= 0 && code < 256);
            *output++ = static_cast<std::uint8_t>(code);
            ++bytesDecodedSoFar;
            return true;
        }

        static bool outputSequence(const Dictionary<false>& dict, int code, std::uint8_t *& output, int outputSizeBytes, int& bytesDecodedSoFar, int& firstByte) {
            // A sequence is stored backwards, so we have to write
            // it to a temp then output the buffer in reverse.
            int i = 0;
            static std::vector<uint8_t> sequence(MaxDictEntries);// FIXME: not thread safe
            do {
                assert(i < MaxDictEntries - 1 && code >= 0);
                sequence[i++] = dict.entries[code].value();
                code = dict.entries[code].code();
            } while (code != Nil);

            firstByte = sequence[--i];
            for (; i >= 0; --i) {
                if (!outputByte(sequence[i], output, outputSizeBytes, bytesDecodedSoFar)) {
                    return false;
                }
            }
            return true;
        }
    };

    int lzw_decode(const std::uint8_t * compressed, const int compressedSizeBytes, const int compressedSizeBits, std::uint8_t * uncompressed, const int uncompressedSizeBytes) { 
        using namespace detail;

        if (compressed == nullptr || uncompressed == nullptr)
            THROW_EXCEPTION("lzw::easyDecode(): Null data pointer(s)!");

        if (compressedSizeBytes <= 0 || compressedSizeBits <= 0 || uncompressedSizeBytes <= 0)
            THROW_EXCEPTION("lzw::easyDecode(): Bad in/out sizes!");

        int code          = Nil;
        int prevCode      = Nil;
        int firstByte     = 0;
        int bytesDecoded  = 0;
        int codeBitsWidth = StartBits;

        Dictionary<false> dictionary;
        BitStreamReader bitStream(compressed, compressedSizeBytes, compressedSizeBits);

        while (!bitStream.isEndOfStream()) {
            assert(codeBitsWidth <= MaxDictBits);
            code = static_cast<int>(bitStream.readBitsU64(codeBitsWidth));

            if (prevCode == Nil) {
                if (!outputByte(code, uncompressed, uncompressedSizeBytes, bytesDecoded))
                    break;

                firstByte = code;
                prevCode  = code;
                continue;
            }

            if (code >= dictionary.size) {
                if (!outputSequence(dictionary, prevCode, uncompressed, uncompressedSizeBytes, bytesDecoded, firstByte))
                    break;

                if (!outputByte(firstByte, uncompressed, uncompressedSizeBytes, bytesDecoded))
                    break;
            } else {
                if (!outputSequence(dictionary, code, uncompressed, uncompressedSizeBytes, bytesDecoded, firstByte))
                    break;
            }

            dictionary.add(prevCode, firstByte);
            if (dictionary.flush(codeBitsWidth)) {
                prevCode = Nil;
            } else {
                prevCode = code;
            }
        }

        return bytesDecoded;
    }
} // namespace c4
