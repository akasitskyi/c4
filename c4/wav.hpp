//MIT License
//
//Copyright(c) 2020 Alex Kasitskyi
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

// Based on dr_wav by David Reid
// https://github.com/mackron/dr_libs

#pragma once

#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>

enum {
    DR_WAVE_FORMAT_PCM        = 0x1,
    DR_WAVE_FORMAT_ADPCM      = 0x2,
    DR_WAVE_FORMAT_IEEE_FLOAT = 0x3,
    DR_WAVE_FORMAT_ALAW       = 0x6,
    DR_WAVE_FORMAT_MULAW      = 0x7,
    DR_WAVE_FORMAT_DVI_ADPCM  = 0x11,
    DR_WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

enum drwav_container {
    drwav_container_riff,
    drwav_container_w64
};

struct drwav_chunk_header {
    union {
        uint8_t fourcc[4];
        uint8_t guid[16];
    } id;

    /* The size in bytes of the chunk. */
    uint64_t sizeInBytes;

    /*
    RIFF = 2 byte alignment.
    W64  = 8 byte alignment.
    */
    unsigned int paddingSize;
};

struct drwav_fmt {
    /*
    The format tag exactly as specified in the wave file's "fmt" chunk. This can be used by applications
    that require support for data formats not natively supported by dr_wav.
    */
    uint16_t formatTag;

    /* The number of channels making up the audio data. When this is set to 1 it is mono, 2 is stereo, etc. */
    uint16_t channels;

    /* The sample rate. Usually set to something like 44100. */
    uint32_t sampleRate;

    /* Average bytes per second. You probably don't need this, but it's left here for informational purposes. */
    uint32_t avgBytesPerSec;

    /* Block align. This is equal to the number of channels * bytes per sample. */
    uint16_t blockAlign;

    /* Bits per sample. */
    uint16_t bitsPerSample;

    /* The size of the extended data. Only used internally for validation, but left here for informational purposes. */
    uint16_t extendedSize;

    /*
    The number of valid bits per sample. When <formatTag> is equal to WAVE_FORMAT_EXTENSIBLE, <bitsPerSample>
    is always rounded up to the nearest multiple of 8. This variable contains information about exactly how
    many bits are valid per sample. Mainly used for informational purposes.
    */
    uint16_t validBitsPerSample;

    /* The channel mask. Not used at the moment. */
    uint32_t channelMask;

    /* The sub-format, exactly as specified by the wave file. */
    uint8_t subFormat[16];
};

struct drwav_data_format {
    drwav_container container = drwav_container_riff;  /* RIFF, W64. */
    uint32_t format = 0;        /* DR_WAVE_FORMAT_* */
    uint32_t channels = 0;
    uint32_t sampleRate = 0;
    uint32_t bitsPerSample = 0;
};

/* See the following for details on the 'smpl' chunk: https://sites.google.com/site/musicgapi/technical-documents/wav-file-format#smpl */
struct drwav_smpl_loop {
    uint32_t cuePointId;
    uint32_t type;
    uint32_t start;
    uint32_t end;
    uint32_t fraction;
    uint32_t playCount;
};

struct drwav_smpl {
    uint32_t manufacturer;
    uint32_t product;
    uint32_t samplePeriod;
    uint32_t midiUnityNotes;
    uint32_t midiPitchFraction;
    uint32_t smpteFormat;
    uint32_t smpteOffset;
    uint32_t numSampleLoops;
    uint32_t samplerData;
    drwav_smpl_loop loops[1];
};

struct wav_base {
     /* Whether or not the WAV file is formatted as a standard RIFF file or W64. */
     drwav_container container = drwav_container_riff;

     /* Structure containing format information exactly as specified by the wav file. */
     drwav_fmt fmt = {};

     /* The sample rate. Will be set to something like 44100. */
     uint32_t sampleRate = 0;

     /* The number of channels. This will be set to 1 for monaural streams, 2 for stereo, etc. */
     uint16_t channels = 0;

     /* The bits per sample. Will be set to something like 16, 24, etc. */
     uint16_t bitsPerSample = 0;

     /* Equal to fmt.formatTag, or the value specified by fmt.subFormat if fmt.formatTag is equal to 65534 (WAVE_FORMAT_EXTENSIBLE). */
     uint16_t translatedFormatTag = 0;

     /* The total number of PCM frames making up the audio data. */
     uint64_t totalPCMFrameCount = 0;

     /* The size in bytes of the data chunk. */
     uint64_t dataChunkDataSize = 0;

     /* The position in the stream of the first byte of the data chunk. This is used for seeking. */
     uint64_t dataChunkDataPos = 0;

     /* The number of bytes remaining in the data chunk. */
     uint64_t bytesRemaining = 0;

     /* smpl chunk. */
     drwav_smpl smpl = {};
 
     uint32_t get_bytes_per_pcm_frame() const {
         /*
         The bytes per frame is a bit ambiguous. It can be either be based on the bits per sample, or the block align. The way I'm doing it here
         is that if the bits per sample is a multiple of 8, use floor(bitsPerSample*channels/8), otherwise fall back to the block align.
         */
         if ((bitsPerSample & 0x7) == 0) {
             /* Bits per sample is a multiple of 8. */
             return (bitsPerSample * fmt.channels) >> 3;
         } else {
             return fmt.blockAlign;
         }
     }
};
 
template<class T, std::size_t N>
constexpr std::size_t wav_countof(const T(&c)[N]) {
    return N;
}

template<typename T>
inline T wav_clamp(T x, T lo, T hi) {
    return std::max(lo, std::min(hi, x));
}

static const uint8_t drwavGUID_W64_RIFF[16] = {0x72,0x69,0x66,0x66, 0x2E,0x91, 0xCF,0x11, 0xA5,0xD6, 0x28,0xDB,0x04,0xC1,0x00,0x00};    /* 66666972-912E-11CF-A5D6-28DB04C10000 */
static const uint8_t drwavGUID_W64_WAVE[16] = {0x77,0x61,0x76,0x65, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 65766177-ACF3-11D3-8CD1-00C04F8EDB8A */
static const uint8_t drwavGUID_W64_JUNK[16] = {0x6A,0x75,0x6E,0x6B, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 6B6E756A-ACF3-11D3-8CD1-00C04F8EDB8A */
static const uint8_t drwavGUID_W64_FMT [16] = {0x66,0x6D,0x74,0x20, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 20746D66-ACF3-11D3-8CD1-00C04F8EDB8A */
static const uint8_t drwavGUID_W64_FACT[16] = {0x66,0x61,0x63,0x74, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 74636166-ACF3-11D3-8CD1-00C04F8EDB8A */
static const uint8_t drwavGUID_W64_DATA[16] = {0x64,0x61,0x74,0x61, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 61746164-ACF3-11D3-8CD1-00C04F8EDB8A */
static const uint8_t drwavGUID_W64_SMPL[16] = {0x73,0x6D,0x70,0x6C, 0xF3,0xAC, 0xD3,0x11, 0x8C,0xD1, 0x00,0xC0,0x4F,0x8E,0xDB,0x8A};    /* 6C706D73-ACF3-11D3-8CD1-00C04F8EDB8A */

inline bool drwav__guid_equal(const uint8_t a[16], const uint8_t b[16]) {
    return std::memcmp(a, b, 16) == 0;
}

inline bool drwav__fourcc_equal(const uint8_t* a, const char* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

inline bool drwav__is_little_endian() {
    int n = 1;
    return (*(char*)&n) == 1;
}

inline uint16_t drwav__bytes_to_u16(const uint8_t* data) {
    return (data[0] << 0) | (data[1] << 8);
}

inline int16_t drwav__bytes_to_s16(const uint8_t* data) {
    return (int16_t)drwav__bytes_to_u16(data);
}

inline uint32_t drwav__bytes_to_u32(const uint8_t* data) {
    return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

inline int32_t drwav__bytes_to_s32(const uint8_t* data) {
    return (int32_t)drwav__bytes_to_u32(data);
}

inline uint64_t drwav__bytes_to_u64(const uint8_t* data) {
    return
        ((uint64_t)data[0] <<  0) | ((uint64_t)data[1] <<  8) | ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
        ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) | ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
}

inline int64_t drwav__bytes_to_s64(const uint8_t* data) {
    return (int64_t)drwav__bytes_to_u64(data);
}

inline void drwav__bytes_to_guid(const uint8_t* data, uint8_t* guid) {
    memcpy(guid, data, 16);
}


inline uint16_t drwav__bswap16(uint16_t n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

inline uint32_t drwav__bswap32(uint32_t n) {
    return ((n & 0xFF000000) >> 24) | ((n & 0x00FF0000) >>  8) | ((n & 0x0000FF00) <<  8) | ((n & 0x000000FF) << 24);
}

inline uint64_t drwav__bswap64(uint64_t n) {
    return ((n & (uint64_t)0xFF00000000000000) >> 56) |
           ((n & (uint64_t)0x00FF000000000000) >> 40) |
           ((n & (uint64_t)0x0000FF0000000000) >> 24) |
           ((n & (uint64_t)0x000000FF00000000) >>  8) |
           ((n & (uint64_t)0x00000000FF000000) <<  8) |
           ((n & (uint64_t)0x0000000000FF0000) << 24) |
           ((n & (uint64_t)0x000000000000FF00) << 40) |
           ((n & (uint64_t)0x00000000000000FF) << 56);
}

inline int16_t drwav__bswap_s16(int16_t n) {
    return (int16_t)drwav__bswap16((uint16_t)n);
}

inline void drwav__bswap_samples_s16(int16_t* pSamples, uint64_t sampleCount) {
    for (uint64_t iSample = 0; iSample < sampleCount; iSample += 1) {
        pSamples[iSample] = drwav__bswap_s16(pSamples[iSample]);
    }
}

inline void drwav__bswap_s24(uint8_t* p) {
    std::swap(p[0], p[2]);
}

inline void drwav__bswap_samples_s24(uint8_t* pSamples, uint64_t sampleCount) {
    for (uint64_t iSample = 0; iSample < sampleCount; iSample += 1) {
        uint8_t* pSample = pSamples + (iSample*3);
        drwav__bswap_s24(pSample);
    }
}

inline int32_t drwav__bswap_s32(int32_t n) {
    return (int32_t)drwav__bswap32((uint32_t)n);
}

inline void drwav__bswap_samples_s32(int32_t* pSamples, uint64_t sampleCount) {
    for (uint64_t iSample = 0; iSample < sampleCount; iSample += 1) {
        pSamples[iSample] = drwav__bswap_s32(pSamples[iSample]);
    }
}

inline float drwav__bswap_f32(float n) {
    union {
        uint32_t i;
        float f;
    } x;
    x.f = n;
    x.i = drwav__bswap32(x.i);

    return x.f;
}

inline void drwav__bswap_samples_f32(float* pSamples, uint64_t sampleCount) {
    for (uint64_t iSample = 0; iSample < sampleCount; iSample += 1) {
        pSamples[iSample] = drwav__bswap_f32(pSamples[iSample]);
    }
}

inline double drwav__bswap_f64(double n) {
    union {
        uint64_t i;
        double f;
    } x;
    x.f = n;
    x.i = drwav__bswap64(x.i);

    return x.f;
}

inline void drwav__bswap_samples_f64(double* pSamples, uint64_t sampleCount) {
    for (uint64_t iSample = 0; iSample < sampleCount; iSample += 1) {
        pSamples[iSample] = drwav__bswap_f64(pSamples[iSample]);
    }
}

inline void drwav__bswap_samples_pcm(void* pSamples, uint64_t sampleCount, uint32_t bytesPerSample) {
    /* Assumes integer PCM. Floating point PCM is done in drwav__bswap_samples_ieee(). */
    switch (bytesPerSample) {
        case 2: /* s16, s12 (loosely packed) */
            drwav__bswap_samples_s16((int16_t*)pSamples, sampleCount);
            break;
        case 3: /* s24 */
            drwav__bswap_samples_s24((uint8_t*)pSamples, sampleCount);
            break;
        case 4: /* s32 */
            drwav__bswap_samples_s32((int32_t*)pSamples, sampleCount);
            break;
        default:
            /* Unsupported format. */
            assert(false);
    }
}

inline void drwav__bswap_samples_ieee(void* pSamples, uint64_t sampleCount, uint32_t bytesPerSample) {
    switch (bytesPerSample) {
        case 4: /* f32 */
            drwav__bswap_samples_f32((float*)pSamples, sampleCount);
            break;
        case 8: /* f64 */
            drwav__bswap_samples_f64((double*)pSamples, sampleCount);
            break;
        default:
            /* Unsupported format. */
            assert(false);
    }
}

inline void drwav__bswap_samples(void* pSamples, uint64_t sampleCount, uint32_t bytesPerSample, uint16_t format) {
    switch (format) {
        case DR_WAVE_FORMAT_PCM:
            drwav__bswap_samples_pcm(pSamples, sampleCount, bytesPerSample);
            break;
        case DR_WAVE_FORMAT_IEEE_FLOAT:
            drwav__bswap_samples_ieee(pSamples, sampleCount, bytesPerSample);
            break;
        case DR_WAVE_FORMAT_ALAW:
        case DR_WAVE_FORMAT_MULAW:
            drwav__bswap_samples_s16((int16_t*)pSamples, sampleCount);
            break;
        case DR_WAVE_FORMAT_ADPCM:
        case DR_WAVE_FORMAT_DVI_ADPCM:
        default:
            /* Unsupported format. */
            assert(false);
    }
}

inline bool drwav__is_compressed_format_tag(uint16_t formatTag) {
    return formatTag == DR_WAVE_FORMAT_ADPCM || formatTag == DR_WAVE_FORMAT_DVI_ADPCM;
}

static unsigned int drwav__chunk_padding_size_riff(uint64_t chunkSize) {
    return (unsigned int)(chunkSize % 2);
}

static unsigned int drwav__chunk_padding_size_w64(uint64_t chunkSize) {
    return (unsigned int)(chunkSize % 8);
}

static int drwav__read_chunk_header(std::istream& is, drwav_container container, uint64_t* pRunningBytesReadOut, drwav_chunk_header* pHeaderOut) {
    if (container == drwav_container_riff) {
        uint8_t sizeInBytes[4];

        if (!is.read((char*)pHeaderOut->id.fourcc, 4)) {
            return -1;
        }
        if (!is.read((char*)sizeInBytes, 4)) {
            return -1;
        }

        pHeaderOut->sizeInBytes = drwav__bytes_to_u32(sizeInBytes);
        pHeaderOut->paddingSize = drwav__chunk_padding_size_riff(pHeaderOut->sizeInBytes);
        *pRunningBytesReadOut += 8;
    } else {
        uint8_t sizeInBytes[8];

        is.read((char*)pHeaderOut->id.guid, 16);
        is.read((char*)sizeInBytes, 8);

        if (!is) {
            return -1;
        }

        pHeaderOut->sizeInBytes = drwav__bytes_to_u64(sizeInBytes) - 24;    /* <-- Subtract 24 because w64 includes the size of the header. */
        pHeaderOut->paddingSize = drwav__chunk_padding_size_w64(pHeaderOut->sizeInBytes);
        *pRunningBytesReadOut += 24;
    }

    return 0;
}

static bool drwav__read_fmt(std::istream& is, drwav_container container, uint64_t* pRunningBytesReadOut, drwav_fmt* fmtOut) {
    drwav_chunk_header header;
    uint8_t fmt[16];

    if (drwav__read_chunk_header(is, container, pRunningBytesReadOut, &header) != 0) {
        return false;
    }

    /* Skip non-fmt chunks. */
    while ((container == drwav_container_riff && !drwav__fourcc_equal(header.id.fourcc, "fmt ")) || (container == drwav_container_w64 && !drwav__guid_equal(header.id.guid, drwavGUID_W64_FMT))) {
        if (!is.seekg(header.sizeInBytes + header.paddingSize, std::ios_base::cur)) {
            return false;
        }
        *pRunningBytesReadOut += header.sizeInBytes + header.paddingSize;

        /* Try the next header. */
        if (drwav__read_chunk_header(is, container, pRunningBytesReadOut, &header) != 0) {
            return false;
        }
    }

    /* Validation. */
    if (container == drwav_container_riff) {
        if (!drwav__fourcc_equal(header.id.fourcc, "fmt ")) {
            return false;
        }
    } else {
        if (!drwav__guid_equal(header.id.guid, drwavGUID_W64_FMT)) {
            return false;
        }
    }

    if (!is.read((char*)fmt, sizeof(fmt))) {
        return false;
    }
    *pRunningBytesReadOut += sizeof(fmt);

    fmtOut->formatTag      = drwav__bytes_to_u16(fmt + 0);
    fmtOut->channels       = drwav__bytes_to_u16(fmt + 2);
    fmtOut->sampleRate     = drwav__bytes_to_u32(fmt + 4);
    fmtOut->avgBytesPerSec = drwav__bytes_to_u32(fmt + 8);
    fmtOut->blockAlign     = drwav__bytes_to_u16(fmt + 12);
    fmtOut->bitsPerSample  = drwav__bytes_to_u16(fmt + 14);

    fmtOut->extendedSize       = 0;
    fmtOut->validBitsPerSample = 0;
    fmtOut->channelMask        = 0;
    memset(fmtOut->subFormat, 0, sizeof(fmtOut->subFormat));

    if (header.sizeInBytes > 16) {
        uint8_t fmt_cbSize[2];
        int bytesReadSoFar = 0;

        if (!is.read((char*)fmt_cbSize, sizeof(fmt_cbSize))) {
            return false;    /* Expecting more data. */
        }
        *pRunningBytesReadOut += sizeof(fmt_cbSize);

        bytesReadSoFar = 18;

        fmtOut->extendedSize = drwav__bytes_to_u16(fmt_cbSize);
        if (fmtOut->extendedSize > 0) {
            /* Simple validation. */
            if (fmtOut->formatTag == DR_WAVE_FORMAT_EXTENSIBLE) {
                if (fmtOut->extendedSize != 22) {
                    return false;
                }
            }

            if (fmtOut->formatTag == DR_WAVE_FORMAT_EXTENSIBLE) {
                uint8_t fmtext[22];
                
                if (!is.read((char*)fmtext, sizeof(fmtOut->extendedSize))) {
                    return false;    /* Expecting more data. */
                }

                fmtOut->validBitsPerSample = drwav__bytes_to_u16(fmtext + 0);
                fmtOut->channelMask        = drwav__bytes_to_u32(fmtext + 2);
                drwav__bytes_to_guid(fmtext + 6, fmtOut->subFormat);
            } else {
                if (!is.seekg(fmtOut->extendedSize, std::ios_base::cur)) {
                    return false;
                }
            }
            *pRunningBytesReadOut += fmtOut->extendedSize;

            bytesReadSoFar += fmtOut->extendedSize;
        }

        /* Seek past any leftover bytes. For w64 the leftover will be defined based on the chunk size. */
        if (!is.seekg(header.sizeInBytes - bytesReadSoFar, std::ios_base::cur)) {
            return false;
        }
        *pRunningBytesReadOut += (header.sizeInBytes - bytesReadSoFar);
    }

    if (header.paddingSize > 0) {
        if (!is.seekg(header.paddingSize, std::ios_base::cur)) {
            return false;
        }
        *pRunningBytesReadOut += header.paddingSize;
    }

    return true;
}


static size_t drwav__on_read(std::istream& is, void* pBufferOut, size_t bytesToRead, uint64_t* pCursor) {
    is.read((char*)pBufferOut, bytesToRead);
    *pCursor += bytesToRead;
    return is ? bytesToRead : 0;
}

extern uint16_t drwav_fmt_get_format(const drwav_fmt* pFMT) {
    if (pFMT == NULL) {
        return 0;
    }

    if (pFMT->formatTag != DR_WAVE_FORMAT_EXTENSIBLE) {
        return pFMT->formatTag;
    } else {
        return drwav__bytes_to_u16(pFMT->subFormat);    /* Only the first two bytes are required. */
    }
}

static uint32_t drwav__riff_chunk_size_riff(uint64_t dataChunkSize) {
    uint32_t dataSubchunkPaddingSize = drwav__chunk_padding_size_riff(dataChunkSize);

    if (dataChunkSize <= (0xFFFFFFFFUL - 36 - dataSubchunkPaddingSize)) {
        return 36 + (uint32_t)(dataChunkSize + dataSubchunkPaddingSize);
    } else {
        return 0xFFFFFFFF;
    }
}

static uint32_t drwav__data_chunk_size_riff(uint64_t dataChunkSize) {
    if (dataChunkSize <= 0xFFFFFFFFUL) {
        return (uint32_t)dataChunkSize;
    } else {
        return 0xFFFFFFFFUL;
    }
}

static uint64_t drwav__riff_chunk_size_w64(uint64_t dataChunkSize) {
    uint64_t dataSubchunkPaddingSize = drwav__chunk_padding_size_w64(dataChunkSize);

    return 80 + 24 + dataChunkSize + dataSubchunkPaddingSize;   /* +24 because W64 includes the size of the GUID and size fields. */
}

static uint64_t drwav__data_chunk_size_w64(uint64_t dataChunkSize) {
    return 24 + dataChunkSize;        /* +24 because W64 includes the size of the GUID and size fields. */
}

inline size_t wav_write(std::ostream& out, const void* data, size_t count) {
    out.write((const char*)data, count);
    return out ? count : 0;
}

inline size_t wav_read(std::istream& in, void* data, size_t count) {
    in.read((char*)data, count);
    return in ? count : 0;
}

static unsigned short g_drwavAlawTable[256] = {
    0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80, 0xE280, 0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580,
    0xF540, 0xF5C0, 0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0, 0xF140, 0xF1C0, 0xF040, 0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0,
    0xAA00, 0xAE00, 0xA200, 0xA600, 0xBA00, 0xBE00, 0xB200, 0xB600, 0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00, 0x9E00, 0x9200, 0x9600,
    0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00, 0xD900, 0xDB00, 0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900, 0xCB00,
    0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8, 0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58,
    0xFFA8, 0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8, 0xFF28, 0xFF38, 0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58,
    0xFAA0, 0xFAE0, 0xFA20, 0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60, 0xF8A0, 0xF8E0, 0xF820, 0xF860, 0xF9A0, 0xF9E0, 0xF920, 0xF960,
    0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0, 0xFDF0, 0xFD90, 0xFDB0, 0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0, 0xFC90, 0xFCB0,
    0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280, 0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80,
    0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940, 0x0EC0, 0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40,
    0x5600, 0x5200, 0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00, 0x7600, 0x7200, 0x7E00, 0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00,
    0x2B00, 0x2900, 0x2F00, 0x2D00, 0x2300, 0x2100, 0x2700, 0x2500, 0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300, 0x3100, 0x3700, 0x3500,
    0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128, 0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8, 0x01A8,
    0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028, 0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8,
    0x0560, 0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0, 0x0760, 0x0720, 0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0,
    0x02B0, 0x0290, 0x02F0, 0x02D0, 0x0230, 0x0210, 0x0270, 0x0250, 0x03B0, 0x0390, 0x03F0, 0x03D0, 0x0330, 0x0310, 0x0370, 0x0350
};

static unsigned short g_drwavMulawTable[256] = {
    0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84, 0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
    0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84, 0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
    0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804, 0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
    0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444, 0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
    0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64, 0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
    0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74, 0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
    0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC, 0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
    0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0, 0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000,
    0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C, 0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
    0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C, 0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
    0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC, 0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
    0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC, 0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
    0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C, 0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
    0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C, 0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
    0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104, 0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
    0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040, 0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000
};

inline int16_t drwav__alaw_to_s16(uint8_t sampleIn) {
    return (int16_t)g_drwavAlawTable[sampleIn];
}

inline int16_t drwav__mulaw_to_s16(uint8_t sampleIn) {
    return (int16_t)g_drwavMulawTable[sampleIn];
}

extern void drwav_u8_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        int x = pIn[i];
        int r = x << 8;
        r = r - 32768;
        pOut[i] = (short)r;
    }
}

extern void drwav_s24_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        int x = ((int)(((unsigned int)(((const uint8_t*)pIn)[i * 3 + 0]) << 8) | ((unsigned int)(((const uint8_t*)pIn)[i * 3 + 1]) << 16) | ((unsigned int)(((const uint8_t*)pIn)[i * 3 + 2])) << 24)) >> 8;
        int r = x >> 8;
        pOut[i] = (short)r;
    }
}

extern void drwav_s32_to_s16(int16_t* pOut, const int32_t* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        int x = pIn[i];
        int r = x >> 16;
        pOut[i] = (short)r;
    }
}

extern void drwav_f32_to_s16(int16_t* pOut, const float* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        float x = pIn[i];
        float c;
        c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
        c = c + 1;
        int r = (int)(c * 32767.5f);
        r = r - 32768;
        pOut[i] = (short)r;
    }
}

extern void drwav_f64_to_s16(int16_t* pOut, const double* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        double x = pIn[i];
        double c;
        c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
        c = c + 1;
        int r = (int)(c * 32767.5);
        r = r - 32768;
        pOut[i] = (short)r;
    }
}

extern void drwav_alaw_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        pOut[i] = drwav__alaw_to_s16(pIn[i]);
    }
}

extern void drwav_mulaw_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; ++i) {
        pOut[i] = drwav__mulaw_to_s16(pIn[i]);
    }
}

extern void drwav_u8_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        float x = pIn[i];
        x = x * 0.00784313725490196078f;    /* 0..255 to 0..2 */
        x = x - 1;                          /* 0..2 to -1..1 */

        *pOut++ = x;
    }
}

extern void drwav_s16_to_f32(float* pOut, const int16_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = pIn[i] * 0.000030517578125f;
    }
}

extern void drwav_s24_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount)
{
    size_t i;

    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (i = 0; i < sampleCount; ++i) {
        double x = (double)(((int32_t)(((uint32_t)(pIn[i * 3 + 0]) << 8) | ((uint32_t)(pIn[i * 3 + 1]) << 16) | ((uint32_t)(pIn[i * 3 + 2])) << 24)) >> 8);
        *pOut++ = (float)(x * 0.00000011920928955078125);
    }
}

extern void drwav_s32_to_f32(float* pOut, const int32_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = (float)(pIn[i] / 2147483648.0);
    }
}

extern void drwav_f64_to_f32(float* pOut, const double* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = (float)pIn[i];
    }
}

extern void drwav_alaw_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = drwav__alaw_to_s16(pIn[i]) / 32768.0f;
    }
}

extern void drwav_mulaw_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = drwav__mulaw_to_s16(pIn[i]) / 32768.0f;
    }
}

extern void drwav_u8_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = ((int)pIn[i] - 128) << 24;
    }
}

extern void drwav_s16_to_s32(int32_t* pOut, const int16_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = pIn[i] << 16;
    }
}

extern void drwav_s24_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        unsigned int s0 = pIn[i * 3 + 0];
        unsigned int s1 = pIn[i * 3 + 1];
        unsigned int s2 = pIn[i * 3 + 2];

        int32_t sample32 = (int32_t)((s0 << 8) | (s1 << 16) | (s2 << 24));
        *pOut++ = sample32;
    }
}

extern void drwav_f32_to_s32(int32_t* pOut, const float* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = (int32_t)(2147483648.0 * pIn[i]);
    }
}

extern void drwav_f64_to_s32(int32_t* pOut, const double* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = (int32_t)(2147483648.0 * pIn[i]);
    }
}

extern void drwav_alaw_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = ((int32_t)drwav__alaw_to_s16(pIn[i])) << 16;
    }
}

extern void drwav_mulaw_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount) {
    if (pOut == NULL || pIn == NULL) {
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = ((int32_t)drwav__mulaw_to_s16(pIn[i])) << 16;
    }
}

static void drwav__pcm_to_s32(int32_t* pOut, const uint8_t* pIn, size_t totalSampleCount, unsigned int bytesPerSample) {
    /* Special case for 8-bit sample data because it's treated as unsigned. */
    if (bytesPerSample == 1) {
        drwav_u8_to_s32(pOut, pIn, totalSampleCount);
        return;
    }

    /* Slightly more optimal implementation for common formats. */
    if (bytesPerSample == 2) {
        drwav_s16_to_s32(pOut, (const int16_t*)pIn, totalSampleCount);
        return;
    }
    if (bytesPerSample == 3) {
        drwav_s24_to_s32(pOut, pIn, totalSampleCount);
        return;
    }
    if (bytesPerSample == 4) {
        for (unsigned int i = 0; i < totalSampleCount; ++i) {
            *pOut++ = ((const int32_t*)pIn)[i];
        }
        return;
    }


    /* Anything more than 64 bits per sample is not supported. */
    if (bytesPerSample > 8) {
        memset(pOut, 0, totalSampleCount * sizeof(*pOut));
        return;
    }


    /* Generic, slow converter. */
    for (unsigned int i = 0; i < totalSampleCount; ++i) {
        uint64_t sample = 0;
        unsigned int shift = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift += 8;
        }

        pIn += j;
        *pOut++ = (int32_t)((int64_t)sample >> 32);
    }
}

static void drwav__ieee_to_s32(int32_t* pOut, const uint8_t* pIn, size_t totalSampleCount, unsigned int bytesPerSample) {
    if (bytesPerSample == 4) {
        drwav_f32_to_s32(pOut, (const float*)pIn, totalSampleCount);
        return;
    } else if (bytesPerSample == 8) {
        drwav_f64_to_s32(pOut, (const double*)pIn, totalSampleCount);
        return;
    } else {
        /* Only supporting 32- and 64-bit float. Output silence in all other cases. Contributions welcome for 16-bit float. */
        memset(pOut, 0, totalSampleCount * sizeof(*pOut));
        return;
    }
}

static void drwav__pcm_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount, unsigned int bytesPerSample) {
    /* Special case for 8-bit sample data because it's treated as unsigned. */
    if (bytesPerSample == 1) {
        drwav_u8_to_f32(pOut, pIn, sampleCount);
        return;
    }

    /* Slightly more optimal implementation for common formats. */
    if (bytesPerSample == 2) {
        drwav_s16_to_f32(pOut, (const int16_t*)pIn, sampleCount);
        return;
    }
    if (bytesPerSample == 3) {
        drwav_s24_to_f32(pOut, pIn, sampleCount);
        return;
    }
    if (bytesPerSample == 4) {
        drwav_s32_to_f32(pOut, (const int32_t*)pIn, sampleCount);
        return;
    }

    /* Anything more than 64 bits per sample is not supported. */
    if (bytesPerSample > 8) {
        memset(pOut, 0, sampleCount * sizeof(*pOut));
        return;
    }

    /* Generic, slow converter. */
    for (unsigned int i = 0; i < sampleCount; ++i) {
        uint64_t sample = 0;
        unsigned int shift = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift += 8;
        }

        pIn += j;
        *pOut++ = (float)((int64_t)sample / 9223372036854775807.0);
    }
}

static void drwav__ieee_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount, unsigned int bytesPerSample) {
    if (bytesPerSample == 4) {
        unsigned int i;
        for (i = 0; i < sampleCount; ++i) {
            *pOut++ = ((const float*)pIn)[i];
        }
        return;
    } else if (bytesPerSample == 8) {
        drwav_f64_to_f32(pOut, (const double*)pIn, sampleCount);
        return;
    } else {
        /* Only supporting 32- and 64-bit float. Output silence in all other cases. Contributions welcome for 16-bit float. */
        memset(pOut, 0, sampleCount * sizeof(*pOut));
        return;
    }
}

static void drwav__pcm_to_s16(int16_t* pOut, const uint8_t* pIn, size_t totalSampleCount, unsigned int bytesPerSample) {
    /* Special case for 8-bit sample data because it's treated as unsigned. */
    if (bytesPerSample == 1) {
        drwav_u8_to_s16(pOut, pIn, totalSampleCount);
        return;
    }


    /* Slightly more optimal implementation for common formats. */
    if (bytesPerSample == 2) {
        for (unsigned int i = 0; i < totalSampleCount; ++i) {
            *pOut++ = ((const int16_t*)pIn)[i];
        }
        return;
    }
    if (bytesPerSample == 3) {
        drwav_s24_to_s16(pOut, pIn, totalSampleCount);
        return;
    }
    if (bytesPerSample == 4) {
        drwav_s32_to_s16(pOut, (const int32_t*)pIn, totalSampleCount);
        return;
    }


    /* Anything more than 64 bits per sample is not supported. */
    if (bytesPerSample > 8) {
        memset(pOut, 0, totalSampleCount * sizeof(*pOut));
        return;
    }

    /* Generic, slow converter. */
    for (unsigned int i = 0; i < totalSampleCount; ++i) {
        uint64_t sample = 0;
        unsigned int shift = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift += 8;
        }

        pIn += j;
        *pOut++ = (int16_t)((int64_t)sample >> 48);
    }
}

static void drwav__ieee_to_s16(int16_t* pOut, const uint8_t* pIn, size_t totalSampleCount, unsigned int bytesPerSample) {
    if (bytesPerSample == 4) {
        drwav_f32_to_s16(pOut, (const float*)pIn, totalSampleCount);
        return;
    } else if (bytesPerSample == 8) {
        drwav_f64_to_s16(pOut, (const double*)pIn, totalSampleCount);
        return;
    } else {
        /* Only supporting 32- and 64-bit float. Output silence in all other cases. Contributions welcome for 16-bit float. */
        memset(pOut, 0, totalSampleCount * sizeof(*pOut));
        return;
    }
}

class wav_reader : public wav_base {
private:
    std::unique_ptr<std::ifstream> ifstreamHolder;
public:
    std::istream& in;

    /* Generic data for compressed formats. This data is shared across all block-compressed formats. */
    uint64_t iCurrentCompressedPCMFrame = 0;  /* The index of the next PCM frame that will be read by drwav_read_*(). This is used with "totalPCMFrameCount" to ensure we don't read excess samples at the end of the last block. */

    /* Microsoft ADPCM specific data. */
    struct {
        uint32_t bytesRemainingInBlock;
        uint16_t predictor[2];
        int32_t  delta[2];
        int32_t  cachedFrames[4];  /* Samples are stored in this cache during decoding. */
        uint32_t cachedFrameCount;
        int32_t  prevFrames[2][2]; /* The previous 2 samples for each channel (2 channels at most). */
    } msadpcm = {};

    /* IMA ADPCM specific data. */
    struct {
        uint32_t bytesRemainingInBlock;
        int32_t  predictor[2];
        int32_t  stepIndex[2];
        int32_t  cachedFrames[16]; /* Samples are stored in this cache during decoding. */
        uint32_t cachedFrameCount;
    } ima = {};

    wav_reader(std::istream& in) : wav_base(), in(in) {
        init();
    }

    wav_reader(std::string filepath) : wav_base(), ifstreamHolder(std::make_unique<std::ifstream>(filepath, std::ifstream::binary)), in(*ifstreamHolder) {
        init();
    }

    size_t drwav_read_raw(size_t bytesToRead, void* pBufferOut) {
        size_t bytesRead;

        if (bytesToRead == 0) {
            return 0;
        }

        if (bytesToRead > bytesRemaining) {
            bytesToRead = (size_t)bytesRemaining;
        }

        if (pBufferOut != NULL) {
            bytesRead = wav_read(in, pBufferOut, bytesToRead);
        } else {
            /* We need to seek. If we fail, we need to read-and-discard to make sure we get a good byte count. */
            bytesRead = 0;
            while (bytesRead < bytesToRead) {
                size_t bytesToSeek = (bytesToRead - bytesRead);
                if (bytesToSeek > 0x7FFFFFFF) {
                    bytesToSeek = 0x7FFFFFFF;
                }

                if (!in.seekg(bytesToSeek, std::ios_base::cur)) {
                    break;
                }

                bytesRead += bytesToSeek;
            }

            /* When we get here we may need to read-and-discard some data. */
            while (bytesRead < bytesToRead) {
                uint8_t buffer[4096];
                size_t bytesSeeked;
                size_t bytesToSeek = (bytesToRead - bytesRead);
                if (bytesToSeek > sizeof(buffer)) {
                    bytesToSeek = sizeof(buffer);
                }

                bytesSeeked = wav_read(in, buffer, bytesToSeek);
                bytesRead += bytesSeeked;

                if (bytesSeeked < bytesToSeek) {
                    break;  /* Reached the end. */
                }
            }
        }

        bytesRemaining -= bytesRead;
        return bytesRead;
    }

    uint64_t drwav_read_pcm_frames_le(uint64_t framesToRead, void* pBufferOut) {
        if (framesToRead == 0) {
            return 0;
        }

        /* Cannot use this function for compressed formats. */
        if (drwav__is_compressed_format_tag(translatedFormatTag)) {
            return 0;
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        return drwav_read_raw((size_t)(framesToRead * bytesPerFrame), pBufferOut) / bytesPerFrame;
    }

    uint64_t drwav_read_pcm_frames_be(uint64_t framesToRead, void* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_le(framesToRead, pBufferOut);

        if (pBufferOut != NULL) {
            drwav__bswap_samples(pBufferOut, framesRead * channels, get_bytes_per_pcm_frame() / channels, translatedFormatTag);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames(uint64_t framesToRead, void* pBufferOut) {
        if (drwav__is_little_endian()) {
            return drwav_read_pcm_frames_le(framesToRead, pBufferOut);
        } else {
            return drwav_read_pcm_frames_be(framesToRead, pBufferOut);
        }
    }

    uint64_t drwav_read_pcm_frames_s16__pcm(uint64_t framesToRead, int16_t* pBufferOut) {
        uint8_t sampleData[4096];

        /* Fast path. */
        if ((translatedFormatTag == DR_WAVE_FORMAT_PCM && bitsPerSample == 16) || pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, pBufferOut);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__pcm_to_s16(pBufferOut, sampleData, (size_t)(framesRead * channels), bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16__ieee(uint64_t framesToRead, int16_t* pBufferOut) {
        uint8_t sampleData[4096];

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__ieee_to_s16(pBufferOut, sampleData, (size_t)(framesRead * channels), bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16__alaw(uint64_t framesToRead, int16_t* pBufferOut) {
        uint8_t sampleData[4096];

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_alaw_to_s16(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16__mulaw(uint64_t framesToRead, int16_t* pBufferOut) {
        uint8_t sampleData[4096];

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_mulaw_to_s16(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16le(uint64_t framesToRead, int16_t* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == false) {
            drwav__bswap_samples_s16(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames_s16be(uint64_t framesToRead, int16_t* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == true) {
            drwav__bswap_samples_s16(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }



    uint64_t drwav_read_pcm_frames_f32__pcm(uint64_t framesToRead, float* pBufferOut) {
        uint8_t sampleData[4096];

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__pcm_to_f32(pBufferOut, sampleData, (size_t)framesRead * channels, bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32__msadpcm(uint64_t framesToRead, float* pBufferOut) {
        /*
        We're just going to borrow the implementation from the drwav_read_s16() since ADPCM is a little bit more complicated than other formats and I don't
        want to duplicate that code.
        */
        uint64_t totalFramesRead = 0;
        int16_t samples16[2048];
        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames_s16(std::min(framesToRead, wav_countof(samples16) / channels), samples16);
            if (framesRead == 0) {
                break;
            }

            drwav_s16_to_f32(pBufferOut, samples16, (size_t)(framesRead * channels));   /* <-- Safe cast because we're clamping to 2048. */

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32__ima(uint64_t framesToRead, float* pBufferOut) {
        /*
        We're just going to borrow the implementation from the drwav_read_s16() since IMA-ADPCM is a little bit more complicated than other formats and I don't
        want to duplicate that code.
        */
        uint64_t totalFramesRead = 0;
        int16_t samples16[2048];
        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames_s16(std::min(framesToRead, wav_countof(samples16) / channels), samples16);
            if (framesRead == 0) {
                break;
            }

            drwav_s16_to_f32(pBufferOut, samples16, (size_t)(framesRead * channels));   /* <-- Safe cast because we're clamping to 2048. */

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32__ieee(uint64_t framesToRead, float* pBufferOut) {
        uint8_t sampleData[4096];

        /* Fast path. */
        if (translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT && bitsPerSample == 32) {
            return drwav_read_pcm_frames(framesToRead, pBufferOut);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__ieee_to_f32(pBufferOut, sampleData, (size_t)(framesRead * channels), bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32__alaw(uint64_t framesToRead, float* pBufferOut) {
        uint8_t sampleData[4096];
        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_alaw_to_f32(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32__mulaw(uint64_t framesToRead, float* pBufferOut) {
        uint8_t sampleData[4096];

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_mulaw_to_f32(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_f32(uint64_t framesToRead, float* pBufferOut) {
        if (framesToRead == 0) {
            return 0;
        }

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        switch (translatedFormatTag)
        {
        case DR_WAVE_FORMAT_PCM:
            return drwav_read_pcm_frames_f32__pcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ADPCM:
            return drwav_read_pcm_frames_f32__msadpcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_IEEE_FLOAT:
            return drwav_read_pcm_frames_f32__ieee(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ALAW:
            return drwav_read_pcm_frames_f32__alaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_MULAW:
            return drwav_read_pcm_frames_f32__mulaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_DVI_ADPCM:
            return drwav_read_pcm_frames_f32__ima(framesToRead, pBufferOut);
        }

        return 0;
    }

    uint64_t drwav_read_pcm_frames_f32le(uint64_t framesToRead, float* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_f32(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == false) {
            drwav__bswap_samples_f32(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames_f32be(uint64_t framesToRead, float* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_f32(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == true) {
            drwav__bswap_samples_f32(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__pcm(uint64_t framesToRead, int32_t* pBufferOut) {
        uint8_t sampleData[4096];

        /* Fast path. */
        if (translatedFormatTag == DR_WAVE_FORMAT_PCM && bitsPerSample == 32) {
            return drwav_read_pcm_frames(framesToRead, pBufferOut);
        }

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__pcm_to_s32(pBufferOut, sampleData, (size_t)(framesRead * channels), bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__msadpcm(uint64_t framesToRead, int32_t* pBufferOut) {
        /*
        We're just going to borrow the implementation from the drwav_read_s16() since ADPCM is a little bit more complicated than other formats and I don't
        want to duplicate that code.
        */
        uint64_t totalFramesRead = 0;
        int16_t samples16[2048];
        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames_s16(std::min(framesToRead, wav_countof(samples16) / channels), samples16);
            if (framesRead == 0) {
                break;
            }

            drwav_s16_to_s32(pBufferOut, samples16, (size_t)(framesRead * channels));   /* <-- Safe cast because we're clamping to 2048. */

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__ima(uint64_t framesToRead, int32_t* pBufferOut) {
        /*
        We're just going to borrow the implementation from the drwav_read_s16() since IMA-ADPCM is a little bit more complicated than other formats and I don't
        want to duplicate that code.
        */
        uint64_t totalFramesRead = 0;
        int16_t samples16[2048];
        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames_s16(std::min(framesToRead, wav_countof(samples16) / channels), samples16);
            if (framesRead == 0) {
                break;
            }

            drwav_s16_to_s32(pBufferOut, samples16, (size_t)(framesRead * channels));   /* <-- Safe cast because we're clamping to 2048. */

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__ieee(uint64_t framesToRead, int32_t* pBufferOut) {
        uint8_t sampleData[4096];

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav__ieee_to_s32(pBufferOut, sampleData, (size_t)(framesRead * channels), bytesPerFrame / channels);

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__alaw(uint64_t framesToRead, int32_t* pBufferOut) {
        uint8_t sampleData[4096];

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_alaw_to_s32(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32__mulaw(uint64_t framesToRead, int32_t* pBufferOut) {
        uint8_t sampleData[4096];

        uint32_t bytesPerFrame = get_bytes_per_pcm_frame();
        if (bytesPerFrame == 0) {
            return 0;
        }

        uint64_t totalFramesRead = 0;

        while (framesToRead > 0) {
            uint64_t framesRead = drwav_read_pcm_frames(std::min(framesToRead, sizeof(sampleData) / bytesPerFrame), sampleData);
            if (framesRead == 0) {
                break;
            }

            drwav_mulaw_to_s32(pBufferOut, sampleData, (size_t)(framesRead * channels));

            pBufferOut += framesRead * channels;
            framesToRead -= framesRead;
            totalFramesRead += framesRead;
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s32(uint64_t framesToRead, int32_t* pBufferOut) {
        if (framesToRead == 0) {
            return 0;
        }

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        switch (translatedFormatTag) {
        case DR_WAVE_FORMAT_PCM:
            return drwav_read_pcm_frames_s32__pcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ADPCM:
            return drwav_read_pcm_frames_s32__msadpcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_IEEE_FLOAT:
            return drwav_read_pcm_frames_s32__ieee(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ALAW:
            return drwav_read_pcm_frames_s32__alaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_MULAW:
            return drwav_read_pcm_frames_s32__mulaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_DVI_ADPCM:
            return drwav_read_pcm_frames_s32__ima(framesToRead, pBufferOut);
        }

        return 0;
    }

    uint64_t drwav_read_pcm_frames_s32le(uint64_t framesToRead, int32_t* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_s32(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == false) {
            drwav__bswap_samples_s32(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames_s32be(uint64_t framesToRead, int32_t* pBufferOut) {
        uint64_t framesRead = drwav_read_pcm_frames_s32(framesToRead, pBufferOut);
        if (pBufferOut != NULL && drwav__is_little_endian() == true) {
            drwav__bswap_samples_s32(pBufferOut, framesRead * channels);
        }

        return framesRead;
    }

    uint64_t drwav_read_pcm_frames_s16__msadpcm(uint64_t framesToRead, int16_t* pBufferOut) {
        uint64_t totalFramesRead = 0;

        assert(framesToRead > 0);

        /* TODO: Lots of room for optimization here. */

        while (framesToRead > 0 && iCurrentCompressedPCMFrame < totalPCMFrameCount) {
            /* If there are no cached frames we need to load a new block. */
            if (msadpcm.cachedFrameCount == 0 && msadpcm.bytesRemainingInBlock == 0) {
                if (channels == 1) {
                    /* Mono. */
                    uint8_t header[7];
                    if (!wav_read(in, header, sizeof(header))) {
                        return totalFramesRead;
                    }
                    msadpcm.bytesRemainingInBlock = fmt.blockAlign - sizeof(header);

                    msadpcm.predictor[0] = header[0];
                    msadpcm.delta[0] = drwav__bytes_to_s16(header + 1);
                    msadpcm.prevFrames[0][1] = (int32_t)drwav__bytes_to_s16(header + 3);
                    msadpcm.prevFrames[0][0] = (int32_t)drwav__bytes_to_s16(header + 5);
                    msadpcm.cachedFrames[2] = msadpcm.prevFrames[0][0];
                    msadpcm.cachedFrames[3] = msadpcm.prevFrames[0][1];
                    msadpcm.cachedFrameCount = 2;
                } else {
                    /* Stereo. */
                    uint8_t header[14];
                    if (!wav_read(in, header, sizeof(header))) {
                        return totalFramesRead;
                    }
                    msadpcm.bytesRemainingInBlock = fmt.blockAlign - sizeof(header);

                    msadpcm.predictor[0] = header[0];
                    msadpcm.predictor[1] = header[1];
                    msadpcm.delta[0] = drwav__bytes_to_s16(header + 2);
                    msadpcm.delta[1] = drwav__bytes_to_s16(header + 4);
                    msadpcm.prevFrames[0][1] = (int32_t)drwav__bytes_to_s16(header + 6);
                    msadpcm.prevFrames[1][1] = (int32_t)drwav__bytes_to_s16(header + 8);
                    msadpcm.prevFrames[0][0] = (int32_t)drwav__bytes_to_s16(header + 10);
                    msadpcm.prevFrames[1][0] = (int32_t)drwav__bytes_to_s16(header + 12);

                    msadpcm.cachedFrames[0] = msadpcm.prevFrames[0][0];
                    msadpcm.cachedFrames[1] = msadpcm.prevFrames[1][0];
                    msadpcm.cachedFrames[2] = msadpcm.prevFrames[0][1];
                    msadpcm.cachedFrames[3] = msadpcm.prevFrames[1][1];
                    msadpcm.cachedFrameCount = 2;
                }
            }

            /* Output anything that's cached. */
            while (framesToRead > 0 && msadpcm.cachedFrameCount > 0 && iCurrentCompressedPCMFrame < totalPCMFrameCount) {
                if (pBufferOut != NULL) {
                    for (uint32_t iSample = 0; iSample < channels; iSample += 1) {
                        pBufferOut[iSample] = (int16_t)msadpcm.cachedFrames[(wav_countof(msadpcm.cachedFrames) - (msadpcm.cachedFrameCount * channels)) + iSample];
                    }

                    pBufferOut += channels;
                }

                framesToRead -= 1;
                totalFramesRead += 1;
                iCurrentCompressedPCMFrame += 1;
                msadpcm.cachedFrameCount -= 1;
            }

            if (framesToRead == 0) {
                return totalFramesRead;
            }


            /*
            If there's nothing left in the cache, just go ahead and load more. If there's nothing left to load in the current block we just continue to the next
            loop iteration which will trigger the loading of a new block.
            */
            if (msadpcm.cachedFrameCount == 0) {
                if (msadpcm.bytesRemainingInBlock == 0) {
                    continue;
                } else {
                    static int32_t adaptationTable[] = {
                        230, 230, 230, 230, 307, 409, 512, 614,
                        768, 614, 512, 409, 307, 230, 230, 230
                    };
                    static int32_t coeff1Table[] = { 256, 512, 0, 192, 240, 460,  392 };
                    static int32_t coeff2Table[] = { 0,  -256, 0, 64,  0,  -208, -232 };

                    uint8_t nibbles;
                    int32_t nibble0;
                    int32_t nibble1;

                    if (!wav_read(in, &nibbles, 1)) {
                        return totalFramesRead;
                    }
                    msadpcm.bytesRemainingInBlock -= 1;

                    /* TODO: Optimize away these if statements. */
                    nibble0 = ((nibbles & 0xF0) >> 4); if ((nibbles & 0x80)) { nibble0 |= 0xFFFFFFF0UL; }
                    nibble1 = ((nibbles & 0x0F) >> 0); if ((nibbles & 0x08)) { nibble1 |= 0xFFFFFFF0UL; }

                    if (channels == 1) {
                        /* Mono. */
                        int32_t newSample0;
                        int32_t newSample1;

                        newSample0 = ((msadpcm.prevFrames[0][1] * coeff1Table[msadpcm.predictor[0]]) + (msadpcm.prevFrames[0][0] * coeff2Table[msadpcm.predictor[0]])) >> 8;
                        newSample0 += nibble0 * msadpcm.delta[0];
                        newSample0 = wav_clamp(newSample0, -32768, 32767);

                        msadpcm.delta[0] = (adaptationTable[((nibbles & 0xF0) >> 4)] * msadpcm.delta[0]) >> 8;
                        if (msadpcm.delta[0] < 16) {
                            msadpcm.delta[0] = 16;
                        }

                        msadpcm.prevFrames[0][0] = msadpcm.prevFrames[0][1];
                        msadpcm.prevFrames[0][1] = newSample0;


                        newSample1 = ((msadpcm.prevFrames[0][1] * coeff1Table[msadpcm.predictor[0]]) + (msadpcm.prevFrames[0][0] * coeff2Table[msadpcm.predictor[0]])) >> 8;
                        newSample1 += nibble1 * msadpcm.delta[0];
                        newSample1 = wav_clamp(newSample1, -32768, 32767);

                        msadpcm.delta[0] = (adaptationTable[((nibbles & 0x0F) >> 0)] * msadpcm.delta[0]) >> 8;
                        if (msadpcm.delta[0] < 16) {
                            msadpcm.delta[0] = 16;
                        }

                        msadpcm.prevFrames[0][0] = msadpcm.prevFrames[0][1];
                        msadpcm.prevFrames[0][1] = newSample1;


                        msadpcm.cachedFrames[2] = newSample0;
                        msadpcm.cachedFrames[3] = newSample1;
                        msadpcm.cachedFrameCount = 2;
                    } else {
                        /* Stereo. */
                        int32_t newSample0;
                        int32_t newSample1;

                        /* Left. */
                        newSample0 = ((msadpcm.prevFrames[0][1] * coeff1Table[msadpcm.predictor[0]]) + (msadpcm.prevFrames[0][0] * coeff2Table[msadpcm.predictor[0]])) >> 8;
                        newSample0 += nibble0 * msadpcm.delta[0];
                        newSample0 = wav_clamp(newSample0, -32768, 32767);

                        msadpcm.delta[0] = (adaptationTable[((nibbles & 0xF0) >> 4)] * msadpcm.delta[0]) >> 8;
                        if (msadpcm.delta[0] < 16) {
                            msadpcm.delta[0] = 16;
                        }

                        msadpcm.prevFrames[0][0] = msadpcm.prevFrames[0][1];
                        msadpcm.prevFrames[0][1] = newSample0;


                        /* Right. */
                        newSample1 = ((msadpcm.prevFrames[1][1] * coeff1Table[msadpcm.predictor[1]]) + (msadpcm.prevFrames[1][0] * coeff2Table[msadpcm.predictor[1]])) >> 8;
                        newSample1 += nibble1 * msadpcm.delta[1];
                        newSample1 = wav_clamp(newSample1, -32768, 32767);

                        msadpcm.delta[1] = (adaptationTable[((nibbles & 0x0F) >> 0)] * msadpcm.delta[1]) >> 8;
                        if (msadpcm.delta[1] < 16) {
                            msadpcm.delta[1] = 16;
                        }

                        msadpcm.prevFrames[1][0] = msadpcm.prevFrames[1][1];
                        msadpcm.prevFrames[1][1] = newSample1;

                        msadpcm.cachedFrames[2] = newSample0;
                        msadpcm.cachedFrames[3] = newSample1;
                        msadpcm.cachedFrameCount = 1;
                    }
                }
            }
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16__ima(uint64_t framesToRead, int16_t* pBufferOut) {
        uint64_t totalFramesRead = 0;

        static int32_t indexTable[16] = {
            -1, -1, -1, -1, 2, 4, 6, 8,
            -1, -1, -1, -1, 2, 4, 6, 8
        };

        static int32_t stepTable[89] = {
            7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
            19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
            50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
            130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
            337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
            876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
            2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
            5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
            15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
        };

        assert(framesToRead > 0);

        /* TODO: Lots of room for optimization here. */

        while (framesToRead > 0 && iCurrentCompressedPCMFrame < totalPCMFrameCount) {
            /* If there are no cached samples we need to load a new block. */
            if (ima.cachedFrameCount == 0 && ima.bytesRemainingInBlock == 0) {
                if (channels == 1) {
                    /* Mono. */
                    uint8_t header[4];
                    if (!wav_read(in, header, sizeof(header))) {
                        return totalFramesRead;
                    }
                    ima.bytesRemainingInBlock = fmt.blockAlign - sizeof(header);

                    if (header[2] >= wav_countof(stepTable)) {
                        in.seekg(ima.bytesRemainingInBlock, std::ios_base::cur);
                        ima.bytesRemainingInBlock = 0;
                        return totalFramesRead; /* Invalid data. */
                    }

                    ima.predictor[0] = drwav__bytes_to_s16(header + 0);
                    ima.stepIndex[0] = header[2];
                    ima.cachedFrames[wav_countof(ima.cachedFrames) - 1] = ima.predictor[0];
                    ima.cachedFrameCount = 1;
                } else {
                    /* Stereo. */
                    uint8_t header[8];
                    if (!wav_read(in, header, sizeof(header))) {
                        return totalFramesRead;
                    }
                    ima.bytesRemainingInBlock = fmt.blockAlign - sizeof(header);

                    if (header[2] >= wav_countof(stepTable) || header[6] >= wav_countof(stepTable)) {
                        in.seekg(ima.bytesRemainingInBlock, std::ios_base::cur);
                        ima.bytesRemainingInBlock = 0;
                        return totalFramesRead; /* Invalid data. */
                    }

                    ima.predictor[0] = drwav__bytes_to_s16(header + 0);
                    ima.stepIndex[0] = header[2];
                    ima.predictor[1] = drwav__bytes_to_s16(header + 4);
                    ima.stepIndex[1] = header[6];

                    ima.cachedFrames[wav_countof(ima.cachedFrames) - 2] = ima.predictor[0];
                    ima.cachedFrames[wav_countof(ima.cachedFrames) - 1] = ima.predictor[1];
                    ima.cachedFrameCount = 1;
                }
            }

            /* Output anything that's cached. */
            while (framesToRead > 0 && ima.cachedFrameCount > 0 && iCurrentCompressedPCMFrame < totalPCMFrameCount) {
                if (pBufferOut != NULL) {
                    for (uint32_t iSample = 0; iSample < channels; iSample += 1) {
                        pBufferOut[iSample] = (int16_t)ima.cachedFrames[(wav_countof(ima.cachedFrames) - (ima.cachedFrameCount * channels)) + iSample];
                    }
                    pBufferOut += channels;
                }

                framesToRead -= 1;
                totalFramesRead += 1;
                iCurrentCompressedPCMFrame += 1;
                ima.cachedFrameCount -= 1;
            }

            if (framesToRead == 0) {
                return totalFramesRead;
            }

            /*
            If there's nothing left in the cache, just go ahead and load more. If there's nothing left to load in the current block we just continue to the next
            loop iteration which will trigger the loading of a new block.
            */
            if (ima.cachedFrameCount == 0) {
                if (ima.bytesRemainingInBlock == 0) {
                    continue;
                } else {
                    /*
                    From what I can tell with stereo streams, it looks like every 4 bytes (8 samples) is for one channel. So it goes 4 bytes for the
                    left channel, 4 bytes for the right channel.
                    */
                    ima.cachedFrameCount = 8;
                    for (uint32_t iChannel = 0; iChannel < channels; ++iChannel) {
                        uint32_t iByte;
                        uint8_t nibbles[4];
                        if (!wav_read(in, &nibbles, 4)) {
                            ima.cachedFrameCount = 0;
                            return totalFramesRead;
                        }
                        ima.bytesRemainingInBlock -= 4;

                        for (iByte = 0; iByte < 4; ++iByte) {
                            uint8_t nibble0 = ((nibbles[iByte] & 0x0F) >> 0);
                            uint8_t nibble1 = ((nibbles[iByte] & 0xF0) >> 4);

                            int32_t step = stepTable[ima.stepIndex[iChannel]];
                            int32_t predictor = ima.predictor[iChannel];

                            int32_t      diff = step >> 3;
                            if (nibble0 & 1) diff += step >> 2;
                            if (nibble0 & 2) diff += step >> 1;
                            if (nibble0 & 4) diff += step;
                            if (nibble0 & 8) diff = -diff;

                            predictor = wav_clamp(predictor + diff, -32768, 32767);
                            ima.predictor[iChannel] = predictor;
                            ima.stepIndex[iChannel] = wav_clamp(ima.stepIndex[iChannel] + indexTable[nibble0], 0, (int32_t)wav_countof(stepTable) - 1);
                            ima.cachedFrames[(wav_countof(ima.cachedFrames) - (ima.cachedFrameCount * channels)) + (iByte * 2 + 0) * channels + iChannel] = predictor;


                            step = stepTable[ima.stepIndex[iChannel]];
                            predictor = ima.predictor[iChannel];

                            diff = step >> 3;
                            if (nibble1 & 1) diff += step >> 2;
                            if (nibble1 & 2) diff += step >> 1;
                            if (nibble1 & 4) diff += step;
                            if (nibble1 & 8) diff = -diff;

                            predictor = wav_clamp(predictor + diff, -32768, 32767);
                            ima.predictor[iChannel] = predictor;
                            ima.stepIndex[iChannel] = wav_clamp(ima.stepIndex[iChannel] + indexTable[nibble1], 0, (int32_t)wav_countof(stepTable) - 1);
                            ima.cachedFrames[(wav_countof(ima.cachedFrames) - (ima.cachedFrameCount * channels)) + (iByte * 2 + 1) * channels + iChannel] = predictor;
                        }
                    }
                }
            }
        }

        return totalFramesRead;
    }

    uint64_t drwav_read_pcm_frames_s16(uint64_t framesToRead, int16_t* pBufferOut) {
        if (framesToRead == 0) {
            return 0;
        }

        if (pBufferOut == NULL) {
            return drwav_read_pcm_frames(framesToRead, NULL);
        }

        switch (translatedFormatTag) {
        case DR_WAVE_FORMAT_PCM:
            return drwav_read_pcm_frames_s16__pcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_IEEE_FLOAT:
            return drwav_read_pcm_frames_s16__ieee(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ALAW:
            return drwav_read_pcm_frames_s16__alaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_MULAW:
            return drwav_read_pcm_frames_s16__mulaw(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_ADPCM:
            return drwav_read_pcm_frames_s16__msadpcm(framesToRead, pBufferOut);
        case DR_WAVE_FORMAT_DVI_ADPCM:
            return drwav_read_pcm_frames_s16__ima(framesToRead, pBufferOut);
        }

        return 0;
    }

    int16_t* read_pcm_frames_s16(unsigned int* pChannels, unsigned int* pSampleRate, uint64_t* pTotalFrameCount) {
        uint64_t sampleDataSize = totalPCMFrameCount * channels * sizeof(int16_t);

        int16_t* pSampleData = (int16_t*)malloc((size_t)sampleDataSize);
        if (pSampleData == NULL) {
            return NULL;    /* Failed to allocate memory. */
        }

        uint64_t framesRead = drwav_read_pcm_frames_s16(totalPCMFrameCount, pSampleData);
        if (framesRead != totalPCMFrameCount) {
            free(pSampleData);
            return NULL;    /* There was an error reading the samples. */
        }

        if (pSampleRate) {
            *pSampleRate = sampleRate;
        }
        if (pChannels) {
            *pChannels = channels;
        }
        if (pTotalFrameCount) {
            *pTotalFrameCount = totalPCMFrameCount;
        }

        return pSampleData;
    }

    float* drwav__read_pcm_frames_and_close_f32(unsigned int* pChannels, unsigned int* pSampleRate, uint64_t* pTotalFrameCount) {
        uint64_t sampleDataSize = totalPCMFrameCount * channels * sizeof(float);

        float* pSampleData = (float*)malloc((size_t)sampleDataSize); /* <-- Safe cast due to the check above. */
        if (pSampleData == NULL) {
            return NULL;    /* Failed to allocate memory. */
        }

        uint64_t framesRead = drwav_read_pcm_frames_f32(totalPCMFrameCount, pSampleData);
        if (framesRead != totalPCMFrameCount) {
            free(pSampleData);
            return NULL;    /* There was an error reading the samples. */
        }

        if (pSampleRate) {
            *pSampleRate = sampleRate;
        }
        if (pChannels) {
            *pChannels = channels;
        }
        if (pTotalFrameCount) {
            *pTotalFrameCount = totalPCMFrameCount;
        }

        return pSampleData;
    }

    int32_t* drwav__read_pcm_frames_and_close_s32(unsigned int* pChannels, unsigned int* pSampleRate, uint64_t* pTotalFrameCount) {
        uint64_t sampleDataSize = totalPCMFrameCount * channels * sizeof(int32_t);

        int32_t* pSampleData = (int32_t*)malloc((size_t)sampleDataSize); /* <-- Safe cast due to the check above. */
        if (pSampleData == NULL) {
            return NULL;    /* Failed to allocate memory. */
        }

        uint64_t framesRead = drwav_read_pcm_frames_s32((size_t)totalPCMFrameCount, pSampleData);
        if (framesRead != totalPCMFrameCount) {
            free(pSampleData);
            return NULL;    /* There was an error reading the samples. */
        }

        if (pSampleRate) {
            *pSampleRate = sampleRate;
        }
        if (pChannels) {
            *pChannels = channels;
        }
        if (pTotalFrameCount) {
            *pTotalFrameCount = totalPCMFrameCount;
        }

        return pSampleData;
    }
private:
    void init() {
        uint8_t riff[4];
        uint64_t sampleCountFromFactChunk;
        bool foundDataChunk;
        uint64_t dataChunkSize;
        uint64_t chunkSize;

        uint64_t cursor = 0;

        /* The first 4 bytes should be the RIFF identifier. */
        if (!drwav__on_read(in, riff, sizeof(riff), &cursor)) {
            throw std::runtime_error("Can't read RIFF identifier");
        }

        /*
        The first 4 bytes can be used to identify the container. For RIFF files it will start with "RIFF" and for
        w64 it will start with "riff".
        */
        if (drwav__fourcc_equal(riff, "RIFF")) {
            container = drwav_container_riff;
        } else if (drwav__fourcc_equal(riff, "riff")) {
            uint8_t riff2[12];

            container = drwav_container_w64;

            /* Check the rest of the GUID for validity. */
            if (!drwav__on_read(in, riff2, sizeof(riff2), &cursor)) {
                throw std::runtime_error("Can't read GUID");
            }

            for (int i = 0; i < 12; ++i) {
                if (riff2[i] != drwavGUID_W64_RIFF[i + 4]) {
                    throw std::runtime_error("Invalid GUID");
                }
            }
        } else {
            throw std::runtime_error("Unknown or unsupported container");
        }

        if (container == drwav_container_riff) {
            uint8_t chunkSizeBytes[4];
            uint8_t wave[4];

            /* RIFF/WAVE */
            if (!drwav__on_read(in, chunkSizeBytes, sizeof(chunkSizeBytes), &cursor)) {
                throw std::runtime_error("Can't read RIFF");
            }

            if (drwav__bytes_to_u32(chunkSizeBytes) < 36) {
                throw std::runtime_error("Chunk size should always be at least 36 bytes");
            }

            if (!drwav__on_read(in, wave, sizeof(wave), &cursor)) {
                throw std::runtime_error("Can't read WAVE");
            }

            if (!drwav__fourcc_equal(wave, "WAVE")) {
                throw std::runtime_error("Expecting WAVE");
            }
        } else {
            uint8_t chunkSizeBytes[8];
            uint8_t wave[16];

            /* W64 */
            if (!drwav__on_read(in, chunkSizeBytes, sizeof(chunkSizeBytes), &cursor)) {
                throw std::runtime_error("Can't read W64");
            }

            if (drwav__bytes_to_u64(chunkSizeBytes) < 80) {
                throw std::runtime_error("Bad chunkSizeBytes");
            }

            if (!drwav__on_read(in, wave, sizeof(wave), &cursor)) {
                throw std::runtime_error("Can't read WAVE");
            }

            if (!drwav__guid_equal(wave, drwavGUID_W64_WAVE)) {
                throw std::runtime_error("Expecting W64_WAVE");
            }
        }


        if (!drwav__read_fmt(in, container, &cursor, &fmt)) {
            throw std::runtime_error("Failed to read the fmt chunk");
        }

        translatedFormatTag = fmt.formatTag;
        if (translatedFormatTag == DR_WAVE_FORMAT_EXTENSIBLE) {
            translatedFormatTag = drwav__bytes_to_u16(fmt.subFormat + 0);
        }

        sampleCountFromFactChunk = 0;

        /*
        We need to enumerate over each chunk for two reasons:
        1) The "data" chunk may not be the next one
        2) We may want to report each chunk back to the client

        In order to correctly report each chunk back to the client we will need to keep looping until the end of the file.
        */
        foundDataChunk = false;
        dataChunkSize = 0;

        /* The next chunk we care about is the "data" chunk. This is not necessarily the next chunk so we'll need to loop. */
        for (;;)
        {
            drwav_chunk_header header;
            int result = drwav__read_chunk_header(in, container, &cursor, &header);
            if (result != 0) {
                if (!foundDataChunk) {
                    throw std::runtime_error("Failed to find data chunk");
                } else {
                    in.clear(); // need to clear eofbit
                    break;  /* Probably at the end of the file. Get out of the loop. */
                }
            }

            if (!foundDataChunk) {
                dataChunkDataPos = cursor;
            }

            chunkSize = header.sizeInBytes;
            if (container == drwav_container_riff) {
                if (drwav__fourcc_equal(header.id.fourcc, "data")) {
                    foundDataChunk = true;
                    dataChunkSize = chunkSize;
                }
            } else {
                if (drwav__guid_equal(header.id.guid, drwavGUID_W64_DATA)) {
                    foundDataChunk = true;
                    dataChunkSize = chunkSize;
                }
            }

            /* Optional. Get the total sample count from the FACT chunk. This is useful for compressed formats. */
            if (container == drwav_container_riff) {
                if (drwav__fourcc_equal(header.id.fourcc, "fact")) {
                    uint32_t sampleCount;
                    if (!drwav__on_read(in, &sampleCount, 4, &cursor)) {
                        throw std::runtime_error("Failed to read the fact chunk");
                    }
                    chunkSize -= 4;

                    if (!foundDataChunk) {
                        dataChunkDataPos = cursor;
                    }

                    /*
                    The sample count in the "fact" chunk is either unreliable, or I'm not understanding it properly. For now I am only enabling this
                    for Microsoft ADPCM formats.
                    */
                    if (translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
                        sampleCountFromFactChunk = sampleCount;
                    } else {
                        sampleCountFromFactChunk = 0;
                    }
                }
            } else {
                if (drwav__guid_equal(header.id.guid, drwavGUID_W64_FACT)) {
                    if (!drwav__on_read(in, &sampleCountFromFactChunk, 8, &cursor)) {
                        throw std::runtime_error("Failed to read the W64_fact chunk");
                    }
                    chunkSize -= 8;

                    if (!foundDataChunk) {
                        dataChunkDataPos = cursor;
                    }
                }
            }

            /* "smpl" chunk. */
            if (container == drwav_container_riff) {
                if (drwav__fourcc_equal(header.id.fourcc, "smpl")) {
                    uint8_t smplHeaderData[36];    /* 36 = size of the smpl header section, not including the loop data. */
                    if (chunkSize >= sizeof(smplHeaderData)) {
                        uint64_t bytesJustRead = drwav__on_read(in, smplHeaderData, sizeof(smplHeaderData), &cursor);
                        chunkSize -= bytesJustRead;

                        if (bytesJustRead == sizeof(smplHeaderData)) {
                            uint32_t iLoop;

                            smpl.manufacturer = drwav__bytes_to_u32(smplHeaderData + 0);
                            smpl.product = drwav__bytes_to_u32(smplHeaderData + 4);
                            smpl.samplePeriod = drwav__bytes_to_u32(smplHeaderData + 8);
                            smpl.midiUnityNotes = drwav__bytes_to_u32(smplHeaderData + 12);
                            smpl.midiPitchFraction = drwav__bytes_to_u32(smplHeaderData + 16);
                            smpl.smpteFormat = drwav__bytes_to_u32(smplHeaderData + 20);
                            smpl.smpteOffset = drwav__bytes_to_u32(smplHeaderData + 24);
                            smpl.numSampleLoops = drwav__bytes_to_u32(smplHeaderData + 28);
                            smpl.samplerData = drwav__bytes_to_u32(smplHeaderData + 32);

                            for (iLoop = 0; iLoop < smpl.numSampleLoops && iLoop < wav_countof(smpl.loops); ++iLoop) {
                                uint8_t smplLoopData[24];  /* 24 = size of a loop section in the smpl chunk. */
                                bytesJustRead = drwav__on_read(in, smplLoopData, sizeof(smplLoopData), &cursor);
                                chunkSize -= bytesJustRead;

                                if (bytesJustRead == sizeof(smplLoopData)) {
                                    smpl.loops[iLoop].cuePointId = drwav__bytes_to_u32(smplLoopData + 0);
                                    smpl.loops[iLoop].type = drwav__bytes_to_u32(smplLoopData + 4);
                                    smpl.loops[iLoop].start = drwav__bytes_to_u32(smplLoopData + 8);
                                    smpl.loops[iLoop].end = drwav__bytes_to_u32(smplLoopData + 12);
                                    smpl.loops[iLoop].fraction = drwav__bytes_to_u32(smplLoopData + 16);
                                    smpl.loops[iLoop].playCount = drwav__bytes_to_u32(smplLoopData + 20);
                                } else {
                                    break;  /* Break from the smpl loop for loop. */
                                }
                            }
                        }
                    } else {
                        /* Looks like invalid data. Ignore the chunk. */
                    }
                }
            } else {
                if (drwav__guid_equal(header.id.guid, drwavGUID_W64_SMPL)) {
                    /*
                    This path will be hit when a W64 WAV file contains a smpl chunk. I don't have a sample file to test this path, so a contribution
                    is welcome to add support for this.
                    */
                }
            }

            /* Make sure we seek past the padding. */
            chunkSize += header.paddingSize;

            if (!in.seekg(chunkSize, std::ios_base::cur)) {
                break;
            }
            cursor += chunkSize;

            if (!foundDataChunk) {
                dataChunkDataPos = cursor;
            }
        }

        /* If we haven't found a data chunk, return an error. */
        if (!foundDataChunk) {
            throw std::runtime_error("Failed to find data chunk");
        }

        /* We may have moved passed the data chunk. If so we need to move back. */
        if (!in.seekg(dataChunkDataPos, std::ios_base::beg)) {
            throw std::runtime_error("Failed to seek to the data chunk");
        }
        cursor = dataChunkDataPos;


        /* At this point we should be sitting on the first byte of the raw audio data. */

        sampleRate = fmt.sampleRate;
        channels = fmt.channels;
        bitsPerSample = fmt.bitsPerSample;
        bytesRemaining = dataChunkSize;
        dataChunkDataSize = dataChunkSize;

        if (sampleCountFromFactChunk != 0) {
            totalPCMFrameCount = sampleCountFromFactChunk;
        } else {
            int tmp = get_bytes_per_pcm_frame();
            totalPCMFrameCount = dataChunkSize / tmp;

            if (translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
                uint64_t totalBlockHeaderSizeInBytes;
                uint64_t blockCount = dataChunkSize / fmt.blockAlign;

                /* Make sure any trailing partial block is accounted for. */
                if ((blockCount * fmt.blockAlign) < dataChunkSize) {
                    blockCount += 1;
                }

                /* We decode two samples per byte. There will be blockCount headers in the data chunk. This is enough to know how to calculate the total PCM frame count. */
                totalBlockHeaderSizeInBytes = blockCount * (6 * fmt.channels);
                totalPCMFrameCount = ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) / fmt.channels;
            }
            if (translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
                uint64_t totalBlockHeaderSizeInBytes;
                uint64_t blockCount = dataChunkSize / fmt.blockAlign;

                /* Make sure any trailing partial block is accounted for. */
                if ((blockCount * fmt.blockAlign) < dataChunkSize) {
                    blockCount += 1;
                }

                /* We decode two samples per byte. There will be blockCount headers in the data chunk. This is enough to know how to calculate the total PCM frame count. */
                totalBlockHeaderSizeInBytes = blockCount * (4 * fmt.channels);
                totalPCMFrameCount = ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) / fmt.channels;

                /* The header includes a decoded sample for each channel which acts as the initial predictor sample. */
                totalPCMFrameCount += blockCount;
            }
        }

        /* Some formats only support a certain number of channels. */
        if (translatedFormatTag == DR_WAVE_FORMAT_ADPCM || translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
            if (channels > 2) {
                throw std::runtime_error("Channels number doesn't match the format");
            }
        }
    }
};

struct wav_writer : public wav_base {
    std::ostream& out;

    wav_writer(std::ostream& out, const drwav_data_format& format) : wav_base(), out(out) {
        init(format);
    }

    void init(const drwav_data_format& format) {
        if (format.format == DR_WAVE_FORMAT_EXTENSIBLE || format.format == DR_WAVE_FORMAT_ADPCM || format.format == DR_WAVE_FORMAT_DVI_ADPCM) {
            throw std::logic_error("Format not supported for writing: " + std::to_string(format.format));
        }

        fmt.formatTag = (uint16_t)format.format;
        fmt.channels = (uint16_t)format.channels;
        fmt.sampleRate = format.sampleRate;
        fmt.avgBytesPerSec = (uint32_t)((format.bitsPerSample * format.sampleRate * format.channels) / 8);
        fmt.blockAlign = (uint16_t)((format.channels * format.bitsPerSample) / 8);
        fmt.bitsPerSample = (uint16_t)format.bitsPerSample;
        fmt.extendedSize = 0;

        size_t runningPos = 0;
        uint64_t initialDataChunkSize = 0;
        uint64_t chunkSizeFMT;

        /* "RIFF" chunk. */
        if (format.container == drwav_container_riff) {
            uint32_t chunkSizeRIFF = 36 + (uint32_t)initialDataChunkSize;   /* +36 = "RIFF"+[RIFF Chunk Size]+"WAVE" + [sizeof "fmt " chunk] */
            runningPos += wav_write(out, "RIFF", 4);
            runningPos += wav_write(out, &chunkSizeRIFF, 4);
            runningPos += wav_write(out, "WAVE", 4);
        } else {
            uint64_t chunkSizeRIFF = 80 + 24 + initialDataChunkSize;   /* +24 because W64 includes the size of the GUID and size fields. */
            runningPos += wav_write(out, drwavGUID_W64_RIFF, 16);
            runningPos += wav_write(out, &chunkSizeRIFF, 8);
            runningPos += wav_write(out, drwavGUID_W64_WAVE, 16);
        }

        /* "fmt " chunk. */
        if (format.container == drwav_container_riff) {
            chunkSizeFMT = 16;
            runningPos += wav_write(out, "fmt ", 4);
            runningPos += wav_write(out, &chunkSizeFMT, 4);
        } else {
            chunkSizeFMT = 40;
            runningPos += wav_write(out, drwavGUID_W64_FMT, 16);
            runningPos += wav_write(out, &chunkSizeFMT, 8);
        }

        runningPos += wav_write(out, &fmt.formatTag, 2);
        runningPos += wav_write(out, &fmt.channels, 2);
        runningPos += wav_write(out, &fmt.sampleRate, 4);
        runningPos += wav_write(out, &fmt.avgBytesPerSec, 4);
        runningPos += wav_write(out, &fmt.blockAlign, 2);
        runningPos += wav_write(out, &fmt.bitsPerSample, 2);

        dataChunkDataPos = runningPos;

        /* "data" chunk. */
        if (format.container == drwav_container_riff) {
            uint32_t chunkSizeDATA = (uint32_t)initialDataChunkSize;
            runningPos += wav_write(out, "data", 4);
            runningPos += wav_write(out, &chunkSizeDATA, 4);
        } else {
            uint64_t chunkSizeDATA = 24 + initialDataChunkSize; /* +24 because W64 includes the size of the GUID and size fields. */
            runningPos += wav_write(out, drwavGUID_W64_DATA, 16);
            runningPos += wav_write(out, &chunkSizeDATA, 8);
        }

        /* Set some properties for the client's convenience. */
        container = format.container;
        channels = (uint16_t)format.channels;
        sampleRate = format.sampleRate;
        bitsPerSample = (uint16_t)format.bitsPerSample;
        translatedFormatTag = (uint16_t)format.format;
    }

    bool finalized = false;

    void finalize() {
        if (finalized) {
            return;
        }

        finalized = true;

        /*
        If the drwav object was opened in write mode we'll need to finalize a few things:
        - Make sure the "data" chunk is aligned to 16-bits for RIFF containers, or 64 bits for W64 containers.
        - Set the size of the "data" chunk.
        */
        uint32_t paddingSize = 0;

        /* Padding. Do not adjust wav.dataChunkDataSize - this should not include the padding. */
        if (container == drwav_container_riff) {
            paddingSize = drwav__chunk_padding_size_riff(dataChunkDataSize);
        } else {
            paddingSize = drwav__chunk_padding_size_w64(dataChunkDataSize);
        }

        if (paddingSize > 0) {
            uint64_t paddingData = 0;
            out.write((char*)&paddingData, paddingSize);
        }

        if (container == drwav_container_riff) {
            /* The "RIFF" chunk size. */
            if (out.seekp(4, std::ios_base::beg)) {
                uint32_t riffChunkSize = drwav__riff_chunk_size_riff(dataChunkDataSize);
                wav_write(out, &riffChunkSize, 4);
            }

            /* the "data" chunk size. */
            if (out.seekp(dataChunkDataPos + 4, std::ios_base::beg)) {
                uint32_t dataChunkSize = drwav__data_chunk_size_riff(dataChunkDataSize);
                wav_write(out, &dataChunkSize, 4);
            }
        } else {
            /* The "RIFF" chunk size. */
            if (out.seekp(16, std::ios_base::beg)) {
                uint64_t riffChunkSize = drwav__riff_chunk_size_w64(dataChunkDataSize);
                wav_write(out, &riffChunkSize, 8);
            }

            /* The "data" chunk size. */
            if (out.seekp(dataChunkDataPos + 16, std::ios_base::beg)) {
                uint64_t dataChunkSize = drwav__data_chunk_size_w64(dataChunkDataSize);
                wav_write(out, &dataChunkSize, 8);
            }
        }

        out.flush();
    }

    ~wav_writer() {
        finalize();
    }
};

extern uint64_t drwav_target_write_size_bytes(const drwav_data_format* pFormat, uint64_t totalSampleCount) {
    /* Casting totalSampleCount to int64_t for VC6 compatibility. No issues in practice because nobody is going to exhaust the whole 63 bits. */
    uint64_t targetDataSizeBytes = totalSampleCount * pFormat->channels * (pFormat->bitsPerSample / 8);
    uint64_t riffChunkSizeBytes;
    uint64_t fileSizeBytes;

    if (pFormat->container == drwav_container_riff) {
        riffChunkSizeBytes = drwav__riff_chunk_size_riff(targetDataSizeBytes);
        fileSizeBytes = (8 + riffChunkSizeBytes); /* +8 because WAV doesn't include the size of the ChunkID and ChunkSize fields. */
    } else {
        riffChunkSizeBytes = drwav__riff_chunk_size_w64(targetDataSizeBytes);
        fileSizeBytes = riffChunkSizeBytes;
    }

    return fileSizeBytes;
}

static int drwav_fopen(FILE** ppFile, const char* pFilePath, const char* pOpenMode) {
    if (fopen_s(ppFile, pFilePath, pOpenMode) != 0) {
        return -1;
    }

    return 0;
}

/*
Writes raw audio data.

Returns the number of bytes actually written. If this differs from bytesToWrite, it indicates an error.
*/
extern size_t drwav_write_raw(wav_writer& wav, size_t bytesToWrite, const void* pData) {
    if (bytesToWrite == 0 || pData == NULL) {
        return 0;
    }

    size_t bytesWritten = wav_write(wav.out, pData, bytesToWrite);
    wav.dataChunkDataSize += bytesWritten;

    return bytesWritten;
}


extern uint64_t drwav_write_pcm_frames_le(wav_writer& wav, uint64_t framesToWrite, const void* pData) {
    if (framesToWrite == 0 || pData == NULL) {
        return 0;
    }

    uint64_t bytesToWrite = ((framesToWrite * wav.channels * wav.bitsPerSample) / 8);

    uint64_t bytesWritten = 0;
    const uint8_t* pRunningData = (const uint8_t*)pData;

    while (bytesToWrite > 0) {
        size_t bytesJustWritten = drwav_write_raw(wav, (size_t)bytesToWrite, pRunningData);
        if (bytesJustWritten == 0) {
            break;
        }

        bytesToWrite -= bytesJustWritten;
        bytesWritten += bytesJustWritten;
        pRunningData += bytesJustWritten;
    }

    return (bytesWritten * 8) / wav.bitsPerSample / wav.channels;
}

extern uint64_t drwav_write_pcm_frames_be(wav_writer& wav, uint64_t framesToWrite, const void* pData) {
    if (framesToWrite == 0 || pData == NULL) {
        return 0;
    }

    uint64_t bytesToWrite = ((framesToWrite * wav.channels * wav.bitsPerSample) / 8);

    uint64_t bytesWritten = 0;
    const uint8_t* pRunningData = (const uint8_t*)pData;

    uint32_t bytesPerSample = wav.get_bytes_per_pcm_frame() / wav.channels;
    
    while (bytesToWrite > 0) {
        uint8_t temp[4096];
        uint32_t sampleCount;
        size_t bytesJustWritten;
        uint64_t bytesToWriteThisIteration;

        bytesToWriteThisIteration = bytesToWrite;

        /*
        WAV files are always little-endian. We need to byte swap on big-endian architectures. Since our input buffer is read-only we need
        to use an intermediary buffer for the conversion.
        */
        sampleCount = sizeof(temp)/bytesPerSample;

        if (bytesToWriteThisIteration > ((uint64_t)sampleCount)*bytesPerSample) {
            bytesToWriteThisIteration = ((uint64_t)sampleCount)*bytesPerSample;
        }

        memcpy(temp, pRunningData, (size_t)bytesToWriteThisIteration);
        drwav__bswap_samples(temp, sampleCount, bytesPerSample, wav.translatedFormatTag);

        bytesJustWritten = drwav_write_raw(wav, (size_t)bytesToWriteThisIteration, temp);
        if (bytesJustWritten == 0) {
            break;
        }

        bytesToWrite -= bytesJustWritten;
        bytesWritten += bytesJustWritten;
        pRunningData += bytesJustWritten;
    }

    return (bytesWritten * 8) / wav.bitsPerSample / wav.channels;
}

extern uint64_t drwav_write_pcm_frames(wav_writer& wav, uint64_t framesToWrite, const void* pData) {
    if (drwav__is_little_endian()) {
        return drwav_write_pcm_frames_le(wav, framesToWrite, pData);
    } else {
        return drwav_write_pcm_frames_be(wav, framesToWrite, pData);
    }
}

