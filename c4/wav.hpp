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

#include <cstdint>
#include <algorithm>
#include <stdexcept>

enum {
    DR_WAVE_FORMAT_PCM        = 0x1,
    DR_WAVE_FORMAT_ADPCM      = 0x2,
    DR_WAVE_FORMAT_IEEE_FLOAT = 0x3,
    DR_WAVE_FORMAT_ALAW       = 0x6,
    DR_WAVE_FORMAT_MULAW      = 0x7,
    DR_WAVE_FORMAT_DVI_ADPCM  = 0x11,
    DR_WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

/* Flags to pass into drwav_init_ex(), etc. */
#define DRWAV_SEQUENTIAL            0x00000001

typedef enum
{
    drwav_seek_origin_start,
    drwav_seek_origin_current
} drwav_seek_origin;

typedef enum
{
    drwav_container_riff,
    drwav_container_w64
} drwav_container;

typedef struct
{
    union
    {
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
} drwav_chunk_header;

typedef struct
{
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
} drwav_fmt;

extern uint16_t drwav_fmt_get_format(const drwav_fmt* pFMT);


/*
Callback for when data is read. Return value is the number of bytes actually read.

pUserData   [in]  The user data that was passed to drwav_init() and family.
pBufferOut  [out] The output buffer.
bytesToRead [in]  The number of bytes to read.

Returns the number of bytes actually read.

A return value of less than bytesToRead indicates the end of the stream. Do _not_ return from this callback until
either the entire bytesToRead is filled or you have reached the end of the stream.
*/
typedef size_t (* drwav_read_proc)(void* pUserData, void* pBufferOut, size_t bytesToRead);

/*
Callback for when data is written. Returns value is the number of bytes actually written.

pUserData    [in]  The user data that was passed to drwav_init_write() and family.
pData        [out] A pointer to the data to write.
bytesToWrite [in]  The number of bytes to write.

Returns the number of bytes actually written.

If the return value differs from bytesToWrite, it indicates an error.
*/
typedef size_t (* drwav_write_proc)(void* pUserData, const void* pData, size_t bytesToWrite);

/*
Callback for when data needs to be seeked.

pUserData [in] The user data that was passed to drwav_init() and family.
offset    [in] The number of bytes to move, relative to the origin. Will never be negative.
origin    [in] The origin of the seek - the current position or the start of the stream.

Returns whether or not the seek was successful.

Whether or not it is relative to the beginning or current position is determined by the "origin" parameter which will be either drwav_seek_origin_start or
drwav_seek_origin_current.
*/
typedef bool (* drwav_seek_proc)(void* pUserData, int offset, drwav_seek_origin origin);

typedef struct
{
    drwav_container container;  /* RIFF, W64. */
    uint32_t format;        /* DR_WAVE_FORMAT_* */
    uint32_t channels;
    uint32_t sampleRate;
    uint32_t bitsPerSample;
} drwav_data_format;


/* See the following for details on the 'smpl' chunk: https://sites.google.com/site/musicgapi/technical-documents/wav-file-format#smpl */
typedef struct
{
    uint32_t cuePointId;
    uint32_t type;
    uint32_t start;
    uint32_t end;
    uint32_t fraction;
    uint32_t playCount;
} drwav_smpl_loop;

 typedef struct
{
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
} drwav_smpl;

typedef struct
{
    /* A pointer to the function to call when more data is needed. */
    drwav_read_proc onRead;

    /* A pointer to the function to call when data needs to be written. Only used when the drwav object is opened in write mode. */
    drwav_write_proc onWrite;

    /* A pointer to the function to call when the wav file needs to be seeked. */
    drwav_seek_proc onSeek;

    /* The user data to pass to callbacks. */
    void* pUserData;


    /* Whether or not the WAV file is formatted as a standard RIFF file or W64. */
    drwav_container container;


    /* Structure containing format information exactly as specified by the wav file. */
    drwav_fmt fmt;

    /* The sample rate. Will be set to something like 44100. */
    uint32_t sampleRate;

    /* The number of channels. This will be set to 1 for monaural streams, 2 for stereo, etc. */
    uint16_t channels;

    /* The bits per sample. Will be set to something like 16, 24, etc. */
    uint16_t bitsPerSample;

    /* Equal to fmt.formatTag, or the value specified by fmt.subFormat if fmt.formatTag is equal to 65534 (WAVE_FORMAT_EXTENSIBLE). */
    uint16_t translatedFormatTag;

    /* The total number of PCM frames making up the audio data. */
    uint64_t totalPCMFrameCount;


    /* The size in bytes of the data chunk. */
    uint64_t dataChunkDataSize;
    
    /* The position in the stream of the first byte of the data chunk. This is used for seeking. */
    uint64_t dataChunkDataPos;

    /* The number of bytes remaining in the data chunk. */
    uint64_t bytesRemaining;


    /*
    Only used in sequential write mode. Keeps track of the desired size of the "data" chunk at the point of initialization time. Always
    set to 0 for non-sequential writes and when the drwav object is opened in read mode. Used for validation.
    */
    uint64_t dataChunkDataSizeTargetWrite;

    /* Keeps track of whether or not the wav writer was initialized in sequential mode. */
    uint32_t isSequentialWrite;


    /* smpl chunk. */
    drwav_smpl smpl;


    /* Generic data for compressed formats. This data is shared across all block-compressed formats. */
    struct
    {
        uint64_t iCurrentPCMFrame;  /* The index of the next PCM frame that will be read by drwav_read_*(). This is used with "totalPCMFrameCount" to ensure we don't read excess samples at the end of the last block. */
    } compressed;
    
    /* Microsoft ADPCM specific data. */
    struct
    {
        uint32_t bytesRemainingInBlock;
        uint16_t predictor[2];
        int32_t  delta[2];
        int32_t  cachedFrames[4];  /* Samples are stored in this cache during decoding. */
        uint32_t cachedFrameCount;
        int32_t  prevFrames[2][2]; /* The previous 2 samples for each channel (2 channels at most). */
    } msadpcm;

    /* IMA ADPCM specific data. */
    struct
    {
        uint32_t bytesRemainingInBlock;
        int32_t  predictor[2];
        int32_t  stepIndex[2];
        int32_t  cachedFrames[16]; /* Samples are stored in this cache during decoding. */
        uint32_t cachedFrameCount;
    } ima;
} drwav;


/*
Initializes a pre-allocated drwav object for reading.

pWav                         [out]          A pointer to the drwav object being initialized.
onRead                       [in]           The function to call when data needs to be read from the client.
onSeek                       [in]           The function to call when the read position of the client data needs to move.
pUserData, pReadSeekUserData [in, optional] A pointer to application defined data that will be passed to onRead and onSeek.
pChunkUserData               [in, optional] A pointer to application defined data that will be passed to onChunk.
flags                        [in, optional] A set of flags for controlling how things are loaded.

Returns true if successful; false otherwise.

Close the loader with drwav_uninit().

This is the lowest level function for initializing a WAV file. You can also use drwav_init_file() and drwav_init_memory()
to open the stream from a file or from a block of memory respectively.

Possible values for flags:
  DRWAV_SEQUENTIAL: Never perform a backwards seek while loading. This disables the chunk callback and will cause this function
                    to return as soon as the data chunk is found. Any chunks after the data chunk will be ignored.

drwav_init() is equivalent to "drwav_init_ex(pWav, onRead, onSeek, NULL, pUserData, NULL, 0);".

See also: drwav_init_file(), drwav_init_memory(), drwav_uninit()
*/
extern bool drwav_init(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData);
extern bool drwav_init_ex(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pReadSeekUserData, void* pChunkUserData, uint32_t flags);

/*
Initializes a pre-allocated drwav object for writing.

onWrite   [in]           The function to call when data needs to be written.
onSeek    [in]           The function to call when the write position needs to move.
pUserData [in, optional] A pointer to application defined data that will be passed to onWrite and onSeek.

Returns true if successful; false otherwise.

Close the writer with drwav_uninit().

This is the lowest level function for initializing a WAV file. You can also use drwav_init_file_write() and drwav_init_memory_write()
to open the stream from a file or from a block of memory respectively.

If the total sample count is known, you can use drwav_init_write_sequential(). This avoids the need for dr_wav to perform
a post-processing step for storing the total sample count and the size of the data chunk which requires a backwards seek.

See also: drwav_init_file_write(), drwav_init_memory_write(), drwav_uninit()
*/
extern bool drwav_init_write(drwav* pWav, const drwav_data_format* pFormat, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData);
extern bool drwav_init_write_sequential(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalSampleCount, drwav_write_proc onWrite, void* pUserData);
extern bool drwav_init_write_sequential_pcm_frames(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalPCMFrameCount, drwav_write_proc onWrite, void* pUserData);

/*
Utility function to determine the target size of the entire data to be written (including all headers and chunks).

Returns the target size in bytes.

Useful if the application needs to know the size to allocate.

Only writing to the RIFF chunk and one data chunk is currently supported.

See also: drwav_init_write(), drwav_init_file_write(), drwav_init_memory_write()
*/
extern uint64_t drwav_target_write_size_bytes(const drwav_data_format* pFormat, uint64_t totalSampleCount);

/*
Uninitializes the given drwav object.

Use this only for objects initialized with drwav_init*() functions (drwav_init(), drwav_init_ex(), drwav_init_write(), drwav_init_write_sequential()).
*/
extern int drwav_uninit(drwav* pWav);


/*
Reads raw audio data.

This is the lowest level function for reading audio data. It simply reads the given number of
bytes of the raw internal sample data.

Consider using drwav_read_pcm_frames_s16(), drwav_read_pcm_frames_s32() or drwav_read_pcm_frames_f32() for
reading sample data in a consistent format.

pBufferOut can be NULL in which case a seek will be performed.

Returns the number of bytes actually read.
*/
extern size_t drwav_read_raw(drwav* pWav, size_t bytesToRead, void* pBufferOut);

/*
Reads up to the specified number of PCM frames from the WAV file.

The output data will be in the file's internal format, converted to native-endian byte order. Use
drwav_read_pcm_frames_s16/f32/s32() to read data in a specific format.

If the return value is less than <framesToRead> it means the end of the file has been reached or
you have requested more PCM frames than can possibly fit in the output buffer.

This function will only work when sample data is of a fixed size and uncompressed. If you are
using a compressed format consider using drwav_read_raw() or drwav_read_pcm_frames_s16/s32/f32().

pBufferOut can be NULL in which case a seek will be performed.
*/
extern uint64_t drwav_read_pcm_frames(drwav* pWav, uint64_t framesToRead, void* pBufferOut);
extern uint64_t drwav_read_pcm_frames_le(drwav* pWav, uint64_t framesToRead, void* pBufferOut);
extern uint64_t drwav_read_pcm_frames_be(drwav* pWav, uint64_t framesToRead, void* pBufferOut);

/*
Seeks to the given PCM frame.

Returns true if successful; false otherwise.
*/
extern bool drwav_seek_to_pcm_frame(drwav* pWav, uint64_t targetFrameIndex);


/*
Writes raw audio data.

Returns the number of bytes actually written. If this differs from bytesToWrite, it indicates an error.
*/
extern size_t drwav_write_raw(drwav* pWav, size_t bytesToWrite, const void* pData);

/*
Writes PCM frames.

Returns the number of PCM frames written.

Input samples need to be in native-endian byte order. On big-endian architectures the input data will be converted to
little-endian. Use drwav_write_raw() to write raw audio data without performing any conversion.
*/
extern uint64_t drwav_write_pcm_frames(drwav* pWav, uint64_t framesToWrite, const void* pData);
extern uint64_t drwav_write_pcm_frames_le(drwav* pWav, uint64_t framesToWrite, const void* pData);
extern uint64_t drwav_write_pcm_frames_be(drwav* pWav, uint64_t framesToWrite, const void* pData);


/* Conversion Utilities */
/*
Reads a chunk of audio data and converts it to signed 16-bit PCM samples.

pBufferOut can be NULL in which case a seek will be performed.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has been reached.
*/
extern uint64_t drwav_read_pcm_frames_s16(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut);
extern uint64_t drwav_read_pcm_frames_s16le(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut);
extern uint64_t drwav_read_pcm_frames_s16be(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to signed 16-bit PCM samples. */
extern void drwav_u8_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to signed 16-bit PCM samples. */
extern void drwav_s24_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 32-bit PCM samples to signed 16-bit PCM samples. */
extern void drwav_s32_to_s16(int16_t* pOut, const int32_t* pIn, size_t sampleCount);

/* Low-level function for converting IEEE 32-bit floating point samples to signed 16-bit PCM samples. */
extern void drwav_f32_to_s16(int16_t* pOut, const float* pIn, size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to signed 16-bit PCM samples. */
extern void drwav_f64_to_s16(int16_t* pOut, const double* pIn, size_t sampleCount);

/* Low-level function for converting A-law samples to signed 16-bit PCM samples. */
extern void drwav_alaw_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting u-law samples to signed 16-bit PCM samples. */
extern void drwav_mulaw_to_s16(int16_t* pOut, const uint8_t* pIn, size_t sampleCount);


/*
Reads a chunk of audio data and converts it to IEEE 32-bit floating point samples.

pBufferOut can be NULL in which case a seek will be performed.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has been reached.
*/
extern uint64_t drwav_read_pcm_frames_f32(drwav* pWav, uint64_t framesToRead, float* pBufferOut);
extern uint64_t drwav_read_pcm_frames_f32le(drwav* pWav, uint64_t framesToRead, float* pBufferOut);
extern uint64_t drwav_read_pcm_frames_f32be(drwav* pWav, uint64_t framesToRead, float* pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to IEEE 32-bit floating point samples. */
extern void drwav_u8_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 16-bit PCM samples to IEEE 32-bit floating point samples. */
extern void drwav_s16_to_f32(float* pOut, const int16_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to IEEE 32-bit floating point samples. */
extern void drwav_s24_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 32-bit PCM samples to IEEE 32-bit floating point samples. */
extern void drwav_s32_to_f32(float* pOut, const int32_t* pIn, size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to IEEE 32-bit floating point samples. */
extern void drwav_f64_to_f32(float* pOut, const double* pIn, size_t sampleCount);

/* Low-level function for converting A-law samples to IEEE 32-bit floating point samples. */
extern void drwav_alaw_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting u-law samples to IEEE 32-bit floating point samples. */
extern void drwav_mulaw_to_f32(float* pOut, const uint8_t* pIn, size_t sampleCount);


/*
Reads a chunk of audio data and converts it to signed 32-bit PCM samples.

pBufferOut can be NULL in which case a seek will be performed.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has been reached.
*/
extern uint64_t drwav_read_pcm_frames_s32(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut);
extern uint64_t drwav_read_pcm_frames_s32le(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut);
extern uint64_t drwav_read_pcm_frames_s32be(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to signed 32-bit PCM samples. */
extern void drwav_u8_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 16-bit PCM samples to signed 32-bit PCM samples. */
extern void drwav_s16_to_s32(int32_t* pOut, const int16_t* pIn, size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to signed 32-bit PCM samples. */
extern void drwav_s24_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting IEEE 32-bit floating point samples to signed 32-bit PCM samples. */
extern void drwav_f32_to_s32(int32_t* pOut, const float* pIn, size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to signed 32-bit PCM samples. */
extern void drwav_f64_to_s32(int32_t* pOut, const double* pIn, size_t sampleCount);

/* Low-level function for converting A-law samples to signed 32-bit PCM samples. */
extern void drwav_alaw_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount);

/* Low-level function for converting u-law samples to signed 32-bit PCM samples. */
extern void drwav_mulaw_to_s32(int32_t* pOut, const uint8_t* pIn, size_t sampleCount);


/* High-Level Convenience Helpers */

/*
Helper for initializing a wave file for reading using stdio.

This holds the internal FILE object until drwav_uninit() is called. Keep this in mind if you're caching drwav
objects because the operating system may restrict the number of file handles an application can have open at
any given time.
*/
extern bool drwav_init_file(drwav* pWav, const char* filename);
extern bool drwav_init_file_ex(drwav* pWav, const char* filename, void* pChunkUserData, uint32_t flags);

/*
Helper for initializing a wave file for writing using stdio.

This holds the internal FILE object until drwav_uninit() is called. Keep this in mind if you're caching drwav
objects because the operating system may restrict the number of file handles an application can have open at
any given time.
*/
extern bool drwav_init_file_write(drwav* pWav, const char* filename, const drwav_data_format* pFormat);
extern bool drwav_init_file_write_sequential(drwav* pWav, const char* filename, const drwav_data_format* pFormat, uint64_t totalSampleCount);
extern bool drwav_init_file_write_sequential_pcm_frames(drwav* pWav, const char* filename, const drwav_data_format* pFormat, uint64_t totalPCMFrameCount);

extern int16_t* drwav_open_and_read_pcm_frames_s16(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);
extern float* drwav_open_and_read_pcm_frames_f32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);
extern int32_t* drwav_open_and_read_pcm_frames_s32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);
extern int16_t* drwav_open_file_and_read_pcm_frames_s16(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);
extern float* drwav_open_file_and_read_pcm_frames_f32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);
extern int32_t* drwav_open_file_and_read_pcm_frames_s32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut);

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>

#define drwav_countof(x)                   (sizeof(x) / sizeof(x[0]))

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

inline bool drwav__guid_equal(const uint8_t a[16], const uint8_t b[16])
{
    int i;
    for (i = 0; i < 16; i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
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


static void* drwav__malloc_from_callbacks(size_t sz) {
    return malloc(sz);
}

static void* drwav__realloc_from_callbacks(void* p, size_t szNew, size_t szOld) {
    return realloc(p, szNew);
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

static uint64_t drwav_read_pcm_frames_s16__msadpcm(drwav* pWav, uint64_t samplesToRead, int16_t* pBufferOut);
static uint64_t drwav_read_pcm_frames_s16__ima(drwav* pWav, uint64_t samplesToRead, int16_t* pBufferOut);
static bool drwav_init_write__internal(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalSampleCount);

static int drwav__read_chunk_header(drwav_read_proc onRead, void* pUserData, drwav_container container, uint64_t* pRunningBytesReadOut, drwav_chunk_header* pHeaderOut) {
    if (container == drwav_container_riff) {
        uint8_t sizeInBytes[4];

        if (onRead(pUserData, pHeaderOut->id.fourcc, 4) != 4) {
            return -1;
        }

        if (onRead(pUserData, sizeInBytes, 4) != 4) {
            return -1;
        }

        pHeaderOut->sizeInBytes = drwav__bytes_to_u32(sizeInBytes);
        pHeaderOut->paddingSize = drwav__chunk_padding_size_riff(pHeaderOut->sizeInBytes);
        *pRunningBytesReadOut += 8;
    } else {
        uint8_t sizeInBytes[8];

        if (onRead(pUserData, pHeaderOut->id.guid, 16) != 16) {
            return -1;
        }

        if (onRead(pUserData, sizeInBytes, 8) != 8) {
            return -1;
        }

        pHeaderOut->sizeInBytes = drwav__bytes_to_u64(sizeInBytes) - 24;    /* <-- Subtract 24 because w64 includes the size of the header. */
        pHeaderOut->paddingSize = drwav__chunk_padding_size_w64(pHeaderOut->sizeInBytes);
        *pRunningBytesReadOut += 24;
    }

    return 0;
}

static bool drwav__seek_forward(drwav_seek_proc onSeek, uint64_t offset, void* pUserData) {
    uint64_t bytesRemainingToSeek = offset;
    while (bytesRemainingToSeek > 0) {
        if (bytesRemainingToSeek > 0x7FFFFFFF) {
            if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_current)) {
                return false;
            }
            bytesRemainingToSeek -= 0x7FFFFFFF;
        } else {
            if (!onSeek(pUserData, (int)bytesRemainingToSeek, drwav_seek_origin_current)) {
                return false;
            }
            bytesRemainingToSeek = 0;
        }
    }

    return true;
}

static bool drwav__seek_from_start(drwav_seek_proc onSeek, uint64_t offset, void* pUserData) {
    if (offset <= 0x7FFFFFFF) {
        return onSeek(pUserData, (int)offset, drwav_seek_origin_start);
    }

    /* Larger than 32-bit seek. */
    if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_start)) {
        return false;
    }
    offset -= 0x7FFFFFFF;

    for (;;) {
        if (offset <= 0x7FFFFFFF) {
            return onSeek(pUserData, (int)offset, drwav_seek_origin_current);
        }

        if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_current)) {
            return false;
        }
        offset -= 0x7FFFFFFF;
    }

    throw std::logic_error("Should never get here");
}


static bool drwav__read_fmt(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, drwav_container container, uint64_t* pRunningBytesReadOut, drwav_fmt* fmtOut) {
    drwav_chunk_header header;
    uint8_t fmt[16];

    if (drwav__read_chunk_header(onRead, pUserData, container, pRunningBytesReadOut, &header) != 0) {
        return false;
    }

    /* Skip non-fmt chunks. */
    while ((container == drwav_container_riff && !drwav__fourcc_equal(header.id.fourcc, "fmt ")) || (container == drwav_container_w64 && !drwav__guid_equal(header.id.guid, drwavGUID_W64_FMT))) {
        if (!drwav__seek_forward(onSeek, header.sizeInBytes + header.paddingSize, pUserData)) {
            return false;
        }
        *pRunningBytesReadOut += header.sizeInBytes + header.paddingSize;

        /* Try the next header. */
        if (drwav__read_chunk_header(onRead, pUserData, container, pRunningBytesReadOut, &header) != 0) {
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


    if (onRead(pUserData, fmt, sizeof(fmt)) != sizeof(fmt)) {
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

        if (onRead(pUserData, fmt_cbSize, sizeof(fmt_cbSize)) != sizeof(fmt_cbSize)) {
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
                if (onRead(pUserData, fmtext, fmtOut->extendedSize) != fmtOut->extendedSize) {
                    return false;    /* Expecting more data. */
                }

                fmtOut->validBitsPerSample = drwav__bytes_to_u16(fmtext + 0);
                fmtOut->channelMask        = drwav__bytes_to_u32(fmtext + 2);
                drwav__bytes_to_guid(fmtext + 6, fmtOut->subFormat);
            } else {
                if (!onSeek(pUserData, fmtOut->extendedSize, drwav_seek_origin_current)) {
                    return false;
                }
            }
            *pRunningBytesReadOut += fmtOut->extendedSize;

            bytesReadSoFar += fmtOut->extendedSize;
        }

        /* Seek past any leftover bytes. For w64 the leftover will be defined based on the chunk size. */
        if (!onSeek(pUserData, (int)(header.sizeInBytes - bytesReadSoFar), drwav_seek_origin_current)) {
            return false;
        }
        *pRunningBytesReadOut += (header.sizeInBytes - bytesReadSoFar);
    }

    if (header.paddingSize > 0) {
        if (!onSeek(pUserData, header.paddingSize, drwav_seek_origin_current)) {
            return false;
        }
        *pRunningBytesReadOut += header.paddingSize;
    }

    return true;
}


static size_t drwav__on_read(drwav_read_proc onRead, void* pUserData, void* pBufferOut, size_t bytesToRead, uint64_t* pCursor) {
    assert(onRead != NULL);
    assert(pCursor != NULL);

    size_t bytesRead = onRead(pUserData, pBufferOut, bytesToRead);
    *pCursor += bytesRead;
    return bytesRead;
}

static uint32_t drwav_get_bytes_per_pcm_frame(drwav* pWav) {
    /*
    The bytes per frame is a bit ambiguous. It can be either be based on the bits per sample, or the block align. The way I'm doing it here
    is that if the bits per sample is a multiple of 8, use floor(bitsPerSample*channels/8), otherwise fall back to the block align.
    */
    if ((pWav->bitsPerSample & 0x7) == 0) {
        /* Bits per sample is a multiple of 8. */
        return (pWav->bitsPerSample * pWav->fmt.channels) >> 3;
    } else {
        return pWav->fmt.blockAlign;
    }
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

static bool drwav_preinit(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pReadSeekUserData) {
    if (pWav == NULL || onRead == NULL || onSeek == NULL) {
        return false;
    }

    memset(pWav, 0, sizeof(*pWav));
    pWav->onRead    = onRead;
    pWav->onSeek    = onSeek;
    pWav->pUserData = pReadSeekUserData;

    return true;
}

static bool drwav_init__internal(drwav* pWav, void* pChunkUserData, uint32_t flags) {
    /* This function assumes drwav_preinit() has been called beforehand. */

    uint8_t riff[4];
    drwav_fmt fmt;
    unsigned short translatedFormatTag;
    uint64_t sampleCountFromFactChunk;
    bool foundDataChunk;
    uint64_t dataChunkSize;
    uint64_t chunkSize;

    uint64_t cursor = 0;
    bool sequential = (flags & DRWAV_SEQUENTIAL) != 0;

    /* The first 4 bytes should be the RIFF identifier. */
    if (drwav__on_read(pWav->onRead, pWav->pUserData, riff, sizeof(riff), &cursor) != sizeof(riff)) {
        return false;
    }

    /*
    The first 4 bytes can be used to identify the container. For RIFF files it will start with "RIFF" and for
    w64 it will start with "riff".
    */
    if (drwav__fourcc_equal(riff, "RIFF")) {
        pWav->container = drwav_container_riff;
    } else if (drwav__fourcc_equal(riff, "riff")) {
        int i;
        uint8_t riff2[12];

        pWav->container = drwav_container_w64;

        /* Check the rest of the GUID for validity. */
        if (drwav__on_read(pWav->onRead, pWav->pUserData, riff2, sizeof(riff2), &cursor) != sizeof(riff2)) {
            return false;
        }

        for (i = 0; i < 12; ++i) {
            if (riff2[i] != drwavGUID_W64_RIFF[i+4]) {
                return false;
            }
        }
    } else {
        return false;   /* Unknown or unsupported container. */
    }


    if (pWav->container == drwav_container_riff) {
        uint8_t chunkSizeBytes[4];
        uint8_t wave[4];

        /* RIFF/WAVE */
        if (drwav__on_read(pWav->onRead, pWav->pUserData, chunkSizeBytes, sizeof(chunkSizeBytes), &cursor) != sizeof(chunkSizeBytes)) {
            return false;
        }

        if (drwav__bytes_to_u32(chunkSizeBytes) < 36) {
            return false;    /* Chunk size should always be at least 36 bytes. */
        }

        if (drwav__on_read(pWav->onRead, pWav->pUserData, wave, sizeof(wave), &cursor) != sizeof(wave)) {
            return false;
        }

        if (!drwav__fourcc_equal(wave, "WAVE")) {
            return false;    /* Expecting "WAVE". */
        }
    } else {
        uint8_t chunkSizeBytes[8];
        uint8_t wave[16];

        /* W64 */
        if (drwav__on_read(pWav->onRead, pWav->pUserData, chunkSizeBytes, sizeof(chunkSizeBytes), &cursor) != sizeof(chunkSizeBytes)) {
            return false;
        }

        if (drwav__bytes_to_u64(chunkSizeBytes) < 80) {
            return false;
        }

        if (drwav__on_read(pWav->onRead, pWav->pUserData, wave, sizeof(wave), &cursor) != sizeof(wave)) {
            return false;
        }

        if (!drwav__guid_equal(wave, drwavGUID_W64_WAVE)) {
            return false;
        }
    }


    /* The next bytes should be the "fmt " chunk. */
    if (!drwav__read_fmt(pWav->onRead, pWav->onSeek, pWav->pUserData, pWav->container, &cursor, &fmt)) {
        return false;    /* Failed to read the "fmt " chunk. */
    }

    /* Basic validation. */
    if (fmt.sampleRate == 0 || fmt.channels == 0 || fmt.bitsPerSample == 0 || fmt.blockAlign == 0) {
        return false; /* Probably an invalid WAV file. */
    }

    /* Translate the internal format. */
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
        int result = drwav__read_chunk_header(pWav->onRead, pWav->pUserData, pWav->container, &cursor, &header);
        if (result != 0) {
            if (!foundDataChunk) {
                return false;
            } else {
                break;  /* Probably at the end of the file. Get out of the loop. */
            }
        }

        if (!foundDataChunk) {
            pWav->dataChunkDataPos = cursor;
        }

        chunkSize = header.sizeInBytes;
        if (pWav->container == drwav_container_riff) {
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

        /*
        If at this point we have found the data chunk and we're running in sequential mode, we need to break out of this loop. The reason for
        this is that we would otherwise require a backwards seek which sequential mode forbids.
        */
        if (foundDataChunk && sequential) {
            break;
        }

        /* Optional. Get the total sample count from the FACT chunk. This is useful for compressed formats. */
        if (pWav->container == drwav_container_riff) {
            if (drwav__fourcc_equal(header.id.fourcc, "fact")) {
                uint32_t sampleCount;
                if (drwav__on_read(pWav->onRead, pWav->pUserData, &sampleCount, 4, &cursor) != 4) {
                    return false;
                }
                chunkSize -= 4;

                if (!foundDataChunk) {
                    pWav->dataChunkDataPos = cursor;
                }

                /*
                The sample count in the "fact" chunk is either unreliable, or I'm not understanding it properly. For now I am only enabling this
                for Microsoft ADPCM formats.
                */
                if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
                    sampleCountFromFactChunk = sampleCount;
                } else {
                    sampleCountFromFactChunk = 0;
                }
            }
        } else {
            if (drwav__guid_equal(header.id.guid, drwavGUID_W64_FACT)) {
                if (drwav__on_read(pWav->onRead, pWav->pUserData, &sampleCountFromFactChunk, 8, &cursor) != 8) {
                    return false;
                }
                chunkSize -= 8;

                if (!foundDataChunk) {
                    pWav->dataChunkDataPos = cursor;
                }
            }
        }

        /* "smpl" chunk. */
        if (pWav->container == drwav_container_riff) {
            if (drwav__fourcc_equal(header.id.fourcc, "smpl")) {
                uint8_t smplHeaderData[36];    /* 36 = size of the smpl header section, not including the loop data. */
                if (chunkSize >= sizeof(smplHeaderData)) {
                    uint64_t bytesJustRead = drwav__on_read(pWav->onRead, pWav->pUserData, smplHeaderData, sizeof(smplHeaderData), &cursor);
                    chunkSize -= bytesJustRead;

                    if (bytesJustRead == sizeof(smplHeaderData)) {
                        uint32_t iLoop;

                        pWav->smpl.manufacturer      = drwav__bytes_to_u32(smplHeaderData+0);
                        pWav->smpl.product           = drwav__bytes_to_u32(smplHeaderData+4);
                        pWav->smpl.samplePeriod      = drwav__bytes_to_u32(smplHeaderData+8);
                        pWav->smpl.midiUnityNotes    = drwav__bytes_to_u32(smplHeaderData+12);
                        pWav->smpl.midiPitchFraction = drwav__bytes_to_u32(smplHeaderData+16);
                        pWav->smpl.smpteFormat       = drwav__bytes_to_u32(smplHeaderData+20);
                        pWav->smpl.smpteOffset       = drwav__bytes_to_u32(smplHeaderData+24);
                        pWav->smpl.numSampleLoops    = drwav__bytes_to_u32(smplHeaderData+28);
                        pWav->smpl.samplerData       = drwav__bytes_to_u32(smplHeaderData+32);

                        for (iLoop = 0; iLoop < pWav->smpl.numSampleLoops && iLoop < drwav_countof(pWav->smpl.loops); ++iLoop) {
                            uint8_t smplLoopData[24];  /* 24 = size of a loop section in the smpl chunk. */
                            bytesJustRead = drwav__on_read(pWav->onRead, pWav->pUserData, smplLoopData, sizeof(smplLoopData), &cursor);
                            chunkSize -= bytesJustRead;

                            if (bytesJustRead == sizeof(smplLoopData)) {
                                pWav->smpl.loops[iLoop].cuePointId = drwav__bytes_to_u32(smplLoopData+0);
                                pWav->smpl.loops[iLoop].type       = drwav__bytes_to_u32(smplLoopData+4);
                                pWav->smpl.loops[iLoop].start      = drwav__bytes_to_u32(smplLoopData+8);
                                pWav->smpl.loops[iLoop].end        = drwav__bytes_to_u32(smplLoopData+12);
                                pWav->smpl.loops[iLoop].fraction   = drwav__bytes_to_u32(smplLoopData+16);
                                pWav->smpl.loops[iLoop].playCount  = drwav__bytes_to_u32(smplLoopData+20);
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
        if (!drwav__seek_forward(pWav->onSeek, chunkSize, pWav->pUserData)) {
            break;
        }
        cursor += chunkSize;

        if (!foundDataChunk) {
            pWav->dataChunkDataPos = cursor;
        }
    }

    /* If we haven't found a data chunk, return an error. */
    if (!foundDataChunk) {
        return false;
    }

    /* We may have moved passed the data chunk. If so we need to move back. If running in sequential mode we can assume we are already sitting on the data chunk. */
    if (!sequential) {
        if (!drwav__seek_from_start(pWav->onSeek, pWav->dataChunkDataPos, pWav->pUserData)) {
            return false;
        }
        cursor = pWav->dataChunkDataPos;
    }
    

    /* At this point we should be sitting on the first byte of the raw audio data. */

    pWav->fmt                 = fmt;
    pWav->sampleRate          = fmt.sampleRate;
    pWav->channels            = fmt.channels;
    pWav->bitsPerSample       = fmt.bitsPerSample;
    pWav->bytesRemaining      = dataChunkSize;
    pWav->translatedFormatTag = translatedFormatTag;
    pWav->dataChunkDataSize   = dataChunkSize;

    if (sampleCountFromFactChunk != 0) {
        pWav->totalPCMFrameCount = sampleCountFromFactChunk;
    } else {
        pWav->totalPCMFrameCount = dataChunkSize / drwav_get_bytes_per_pcm_frame(pWav);

        if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
            uint64_t totalBlockHeaderSizeInBytes;
            uint64_t blockCount = dataChunkSize / fmt.blockAlign;

            /* Make sure any trailing partial block is accounted for. */
            if ((blockCount * fmt.blockAlign) < dataChunkSize) {
                blockCount += 1;
            }

            /* We decode two samples per byte. There will be blockCount headers in the data chunk. This is enough to know how to calculate the total PCM frame count. */
            totalBlockHeaderSizeInBytes = blockCount * (6*fmt.channels);
            pWav->totalPCMFrameCount = ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) / fmt.channels;
        }
        if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
            uint64_t totalBlockHeaderSizeInBytes;
            uint64_t blockCount = dataChunkSize / fmt.blockAlign;

            /* Make sure any trailing partial block is accounted for. */
            if ((blockCount * fmt.blockAlign) < dataChunkSize) {
                blockCount += 1;
            }

            /* We decode two samples per byte. There will be blockCount headers in the data chunk. This is enough to know how to calculate the total PCM frame count. */
            totalBlockHeaderSizeInBytes = blockCount * (4*fmt.channels);
            pWav->totalPCMFrameCount = ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) / fmt.channels;

            /* The header includes a decoded sample for each channel which acts as the initial predictor sample. */
            pWav->totalPCMFrameCount += blockCount;
        }
    }

    /* Some formats only support a certain number of channels. */
    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM || pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
        if (pWav->channels > 2) {
            return false;
        }
    }

    return true;
}

extern bool drwav_init(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData) {
    return drwav_init_ex(pWav, onRead, onSeek, pUserData, NULL, 0);
}

extern bool drwav_init_ex(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pReadSeekUserData, void* pChunkUserData, uint32_t flags) {
    if (!drwav_preinit(pWav, onRead, onSeek, pReadSeekUserData)) {
        return false;
    }

    return drwav_init__internal(pWav, pChunkUserData, flags);
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


static bool drwav_preinit_write(drwav* pWav, const drwav_data_format* pFormat, bool isSequential, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData) {
    if (pWav == NULL || onWrite == NULL) {
        return false;
    }

    if (!isSequential && onSeek == NULL) {
        return false; /* <-- onSeek is required when in non-sequential mode. */
    }

    /* Not currently supporting compressed formats. Will need to add support for the "fact" chunk before we enable this. */
    if (pFormat->format == DR_WAVE_FORMAT_EXTENSIBLE) {
        return false;
    }
    if (pFormat->format == DR_WAVE_FORMAT_ADPCM || pFormat->format == DR_WAVE_FORMAT_DVI_ADPCM) {
        return false;
    }

    memset(pWav, 0, sizeof(*pWav));
    pWav->onWrite   = onWrite;
    pWav->onSeek    = onSeek;
    pWav->pUserData = pUserData;

    pWav->fmt.formatTag = (uint16_t)pFormat->format;
    pWav->fmt.channels = (uint16_t)pFormat->channels;
    pWav->fmt.sampleRate = pFormat->sampleRate;
    pWav->fmt.avgBytesPerSec = (uint32_t)((pFormat->bitsPerSample * pFormat->sampleRate * pFormat->channels) / 8);
    pWav->fmt.blockAlign = (uint16_t)((pFormat->channels * pFormat->bitsPerSample) / 8);
    pWav->fmt.bitsPerSample = (uint16_t)pFormat->bitsPerSample;
    pWav->fmt.extendedSize = 0;
    pWav->isSequentialWrite = isSequential;

    return true;
}

static bool drwav_init_write__internal(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalSampleCount) {
    /* The function assumes drwav_preinit_write() was called beforehand. */

    size_t runningPos = 0;
    uint64_t initialDataChunkSize = 0;
    uint64_t chunkSizeFMT;

    /*
    The initial values for the "RIFF" and "data" chunks depends on whether or not we are initializing in sequential mode or not. In
    sequential mode we set this to its final values straight away since they can be calculated from the total sample count. In non-
    sequential mode we initialize it all to zero and fill it out in drwav_uninit() using a backwards seek.
    */
    if (pWav->isSequentialWrite) {
        initialDataChunkSize = (totalSampleCount * pWav->fmt.bitsPerSample) / 8;

        /*
        The RIFF container has a limit on the number of samples. drwav is not allowing this. There's no practical limits for Wave64
        so for the sake of simplicity I'm not doing any validation for that.
        */
        if (pFormat->container == drwav_container_riff) {
            if (initialDataChunkSize > (0xFFFFFFFFUL - 36)) {
                return false; /* Not enough room to store every sample. */
            }
        }
    }

    pWav->dataChunkDataSizeTargetWrite = initialDataChunkSize;


    /* "RIFF" chunk. */
    if (pFormat->container == drwav_container_riff) {
        uint32_t chunkSizeRIFF = 36 + (uint32_t)initialDataChunkSize;   /* +36 = "RIFF"+[RIFF Chunk Size]+"WAVE" + [sizeof "fmt " chunk] */
        runningPos += pWav->onWrite(pWav->pUserData, "RIFF", 4);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeRIFF, 4);
        runningPos += pWav->onWrite(pWav->pUserData, "WAVE", 4);
    } else {
        uint64_t chunkSizeRIFF = 80 + 24 + initialDataChunkSize;   /* +24 because W64 includes the size of the GUID and size fields. */
        runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_RIFF, 16);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeRIFF, 8);
        runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_WAVE, 16);
    }

    /* "fmt " chunk. */
    if (pFormat->container == drwav_container_riff) {
        chunkSizeFMT = 16;
        runningPos += pWav->onWrite(pWav->pUserData, "fmt ", 4);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeFMT, 4);
    } else {
        chunkSizeFMT = 40;
        runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_FMT, 16);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeFMT, 8);
    }

    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.formatTag,      2);
    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.channels,       2);
    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.sampleRate,     4);
    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.avgBytesPerSec, 4);
    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.blockAlign,     2);
    runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.bitsPerSample,  2);

    pWav->dataChunkDataPos = runningPos;

    /* "data" chunk. */
    if (pFormat->container == drwav_container_riff) {
        uint32_t chunkSizeDATA = (uint32_t)initialDataChunkSize;
        runningPos += pWav->onWrite(pWav->pUserData, "data", 4);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeDATA, 4);
    } else {
        uint64_t chunkSizeDATA = 24 + initialDataChunkSize; /* +24 because W64 includes the size of the GUID and size fields. */
        runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_DATA, 16);
        runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeDATA, 8);
    }


    /* Simple validation. */
    if (pFormat->container == drwav_container_riff) {
        if (runningPos != 20 + chunkSizeFMT + 8) {
            return false;
        }
    } else {
        if (runningPos != 40 + chunkSizeFMT + 24) {
            return false;
        }
    }
    

    /* Set some properties for the client's convenience. */
    pWav->container = pFormat->container;
    pWav->channels = (uint16_t)pFormat->channels;
    pWav->sampleRate = pFormat->sampleRate;
    pWav->bitsPerSample = (uint16_t)pFormat->bitsPerSample;
    pWav->translatedFormatTag = (uint16_t)pFormat->format;

    return true;
}


extern bool drwav_init_write(drwav* pWav, const drwav_data_format* pFormat, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData) {
    if (!drwav_preinit_write(pWav, pFormat, false, onWrite, onSeek, pUserData)) {
        return false;
    }

    return drwav_init_write__internal(pWav, pFormat, 0);               /* false = Not Sequential */
}

extern bool drwav_init_write_sequential(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalSampleCount, drwav_write_proc onWrite, void* pUserData) {
    if (!drwav_preinit_write(pWav, pFormat, true, onWrite, NULL, pUserData)) {
        return false;
    }

    return drwav_init_write__internal(pWav, pFormat, totalSampleCount); /* true = Sequential */
}

extern bool drwav_init_write_sequential_pcm_frames(drwav* pWav, const drwav_data_format* pFormat, uint64_t totalPCMFrameCount, drwav_write_proc onWrite, void* pUserData) {
    if (pFormat == NULL) {
        return false;
    }

    return drwav_init_write_sequential(pWav, pFormat, totalPCMFrameCount*pFormat->channels, onWrite, pUserData);
}

extern uint64_t drwav_target_write_size_bytes(const drwav_data_format* pFormat, uint64_t totalSampleCount) {
    /* Casting totalSampleCount to int64_t for VC6 compatibility. No issues in practice because nobody is going to exhaust the whole 63 bits. */
    uint64_t targetDataSizeBytes = (uint64_t)((int64_t)totalSampleCount * pFormat->channels * pFormat->bitsPerSample/8.0);
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


static size_t drwav__on_read_stdio(void* pUserData, void* pBufferOut, size_t bytesToRead) {
    return fread(pBufferOut, 1, bytesToRead, (FILE*)pUserData);
}

static size_t drwav__on_write_stdio(void* pUserData, const void* pData, size_t bytesToWrite) {
    return fwrite(pData, 1, bytesToWrite, (FILE*)pUserData);
}

static bool drwav__on_seek_stdio(void* pUserData, int offset, drwav_seek_origin origin) {
    return fseek((FILE*)pUserData, offset, (origin == drwav_seek_origin_current) ? SEEK_CUR : SEEK_SET) == 0;
}

extern bool drwav_init_file(drwav* pWav, const char* filename) {
    return drwav_init_file_ex(pWav, filename, NULL, 0);
}


static bool drwav_init_file__internal_FILE(drwav* pWav, FILE* pFile, void* pChunkUserData, uint32_t flags) {
    bool result = drwav_preinit(pWav, drwav__on_read_stdio, drwav__on_seek_stdio, (void*)pFile);
    if (result != true) {
        fclose(pFile);
        return result;
    }

    result = drwav_init__internal(pWav, pChunkUserData, flags);
    if (result != true) {
        fclose(pFile);
        return result;
    }

    return true;
}

static int drwav_fopen(FILE** ppFile, const char* pFilePath, const char* pOpenMode) {
    if (fopen_s(ppFile, pFilePath, pOpenMode) != 0) {
        return -1;
    }

    return 0;
}

extern bool drwav_init_file_ex(drwav* pWav, const char* filename, void* pChunkUserData, uint32_t flags) {
    FILE* pFile;
    if (drwav_fopen(&pFile, filename, "rb") != 0) {
        return false;
    }

    /* This takes ownership of the FILE* object. */
    return drwav_init_file__internal_FILE(pWav, pFile, pChunkUserData, flags);
}

static bool drwav_init_file_write__internal_FILE(drwav* pWav, FILE* pFile, const drwav_data_format* pFormat, uint64_t totalSampleCount, bool isSequential) {
    bool result = drwav_preinit_write(pWav, pFormat, isSequential, drwav__on_write_stdio, drwav__on_seek_stdio, (void*)pFile);
    if (result != true) {
        fclose(pFile);
        return result;
    }

    result = drwav_init_write__internal(pWav, pFormat, totalSampleCount);
    if (result != true) {
        fclose(pFile);
        return result;
    }

    return true;
}

static bool drwav_init_file_write__internal(drwav* pWav, const char* filename, const drwav_data_format* pFormat, uint64_t totalSampleCount, bool isSequential) {
    FILE* pFile = fopen(filename, "wb");

    /* This takes ownership of the FILE* object. */
    return drwav_init_file_write__internal_FILE(pWav, pFile, pFormat, totalSampleCount, isSequential);
}

extern bool drwav_init_file_write(drwav* pWav, const char* filename, const drwav_data_format* pFormat) {
    return drwav_init_file_write__internal(pWav, filename, pFormat, 0, false);
}

extern bool drwav_init_file_write_sequential(drwav* pWav, const char* filename, const drwav_data_format* pFormat, uint64_t totalSampleCount) {
    return drwav_init_file_write__internal(pWav, filename, pFormat, totalSampleCount, true);
}

extern bool drwav_init_file_write_sequential_pcm_frames(drwav* pWav, const char* filename, const drwav_data_format* pFormat, uint64_t totalPCMFrameCount) {
    if (pFormat == NULL) {
        return false;
    }

    return drwav_init_file_write_sequential(pWav, filename, pFormat, totalPCMFrameCount*pFormat->channels);
}

extern int drwav_uninit(drwav* pWav) {
    if (pWav == NULL) {
        return -1;
    }

    /*
    If the drwav object was opened in write mode we'll need to finalize a few things:
      - Make sure the "data" chunk is aligned to 16-bits for RIFF containers, or 64 bits for W64 containers.
      - Set the size of the "data" chunk.
    */
    if (pWav->onWrite != NULL) {
        uint32_t paddingSize = 0;

        /* Padding. Do not adjust pWav->dataChunkDataSize - this should not include the padding. */
        if (pWav->container == drwav_container_riff) {
            paddingSize = drwav__chunk_padding_size_riff(pWav->dataChunkDataSize);
        } else {
            paddingSize = drwav__chunk_padding_size_w64(pWav->dataChunkDataSize);
        }
        
        if (paddingSize > 0) {
            uint64_t paddingData = 0;
            pWav->onWrite(pWav->pUserData, &paddingData, paddingSize);
        }

        /*
        Chunk sizes. When using sequential mode, these will have been filled in at initialization time. We only need
        to do this when using non-sequential mode.
        */
        if (pWav->onSeek && !pWav->isSequentialWrite) {
            if (pWav->container == drwav_container_riff) {
                /* The "RIFF" chunk size. */
                if (pWav->onSeek(pWav->pUserData, 4, drwav_seek_origin_start)) {
                    uint32_t riffChunkSize = drwav__riff_chunk_size_riff(pWav->dataChunkDataSize);
                    pWav->onWrite(pWav->pUserData, &riffChunkSize, 4);
                }

                /* the "data" chunk size. */
                if (pWav->onSeek(pWav->pUserData, (int)pWav->dataChunkDataPos + 4, drwav_seek_origin_start)) {
                    uint32_t dataChunkSize = drwav__data_chunk_size_riff(pWav->dataChunkDataSize);
                    pWav->onWrite(pWav->pUserData, &dataChunkSize, 4);
                }
            } else {
                /* The "RIFF" chunk size. */
                if (pWav->onSeek(pWav->pUserData, 16, drwav_seek_origin_start)) {
                    uint64_t riffChunkSize = drwav__riff_chunk_size_w64(pWav->dataChunkDataSize);
                    pWav->onWrite(pWav->pUserData, &riffChunkSize, 8);
                }

                /* The "data" chunk size. */
                if (pWav->onSeek(pWav->pUserData, (int)pWav->dataChunkDataPos + 16, drwav_seek_origin_start)) {
                    uint64_t dataChunkSize = drwav__data_chunk_size_w64(pWav->dataChunkDataSize);
                    pWav->onWrite(pWav->pUserData, &dataChunkSize, 8);
                }
            }
        }

        /* Validation for sequential mode. */
        if (pWav->isSequentialWrite) {
            if (pWav->dataChunkDataSize != pWav->dataChunkDataSizeTargetWrite) {
                throw std::runtime_error("INVALID_FILE");
            }
        }
    }

    /*
    If we opened the file with drwav_open_file() we will want to close the file handle. We can know whether or not drwav_open_file()
    was used by looking at the onRead and onSeek callbacks.
    */
    if (pWav->onRead == drwav__on_read_stdio || pWav->onWrite == drwav__on_write_stdio) {
        fclose((FILE*)pWav->pUserData);
    }

    return 0;
}

extern size_t drwav_read_raw(drwav* pWav, size_t bytesToRead, void* pBufferOut) {
    size_t bytesRead;

    if (pWav == NULL || bytesToRead == 0) {
        return 0;
    }

    if (bytesToRead > pWav->bytesRemaining) {
        bytesToRead = (size_t)pWav->bytesRemaining;
    }

    if (pBufferOut != NULL) {
        bytesRead = pWav->onRead(pWav->pUserData, pBufferOut, bytesToRead);
    } else {
        /* We need to seek. If we fail, we need to read-and-discard to make sure we get a good byte count. */
        bytesRead = 0;
        while (bytesRead < bytesToRead) {
            size_t bytesToSeek = (bytesToRead - bytesRead);
            if (bytesToSeek > 0x7FFFFFFF) {
                bytesToSeek = 0x7FFFFFFF;
            }

            if (pWav->onSeek(pWav->pUserData, (int)bytesToSeek, drwav_seek_origin_current) == false) {
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

            bytesSeeked = pWav->onRead(pWav->pUserData, buffer, bytesToSeek);
            bytesRead += bytesSeeked;

            if (bytesSeeked < bytesToSeek) {
                break;  /* Reached the end. */
            }
        }
    }

    pWav->bytesRemaining -= bytesRead;
    return bytesRead;
}



extern uint64_t drwav_read_pcm_frames_le(drwav* pWav, uint64_t framesToRead, void* pBufferOut) {
    if (pWav == NULL || framesToRead == 0) {
        return 0;
    }

    /* Cannot use this function for compressed formats. */
    if (drwav__is_compressed_format_tag(pWav->translatedFormatTag)) {
        return 0;
    }

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    return drwav_read_raw(pWav, (size_t)(framesToRead * bytesPerFrame), pBufferOut) / bytesPerFrame;
}

extern uint64_t drwav_read_pcm_frames_be(drwav* pWav, uint64_t framesToRead, void* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_le(pWav, framesToRead, pBufferOut);

    if (pBufferOut != NULL) {
        drwav__bswap_samples(pBufferOut, framesRead*pWav->channels, drwav_get_bytes_per_pcm_frame(pWav)/pWav->channels, pWav->translatedFormatTag);
    }

    return framesRead;
}

extern uint64_t drwav_read_pcm_frames(drwav* pWav, uint64_t framesToRead, void* pBufferOut) {
    if (drwav__is_little_endian()) {
        return drwav_read_pcm_frames_le(pWav, framesToRead, pBufferOut);
    } else {
        return drwav_read_pcm_frames_be(pWav, framesToRead, pBufferOut);
    }
}



extern bool drwav_seek_to_first_pcm_frame(drwav* pWav) {
    if (pWav->onWrite != NULL) {
        return false; /* No seeking in write mode. */
    }

    if (!pWav->onSeek(pWav->pUserData, (int)pWav->dataChunkDataPos, drwav_seek_origin_start)) {
        return false;
    }

    if (drwav__is_compressed_format_tag(pWav->translatedFormatTag)) {
        pWav->compressed.iCurrentPCMFrame = 0;
    }
    
    pWav->bytesRemaining = pWav->dataChunkDataSize;
    return true;
}

extern bool drwav_seek_to_pcm_frame(drwav* pWav, uint64_t targetFrameIndex) {
    /* Seeking should be compatible with wave files > 2GB. */

    if (pWav == NULL || pWav->onSeek == NULL) {
        return false;
    }

    /* No seeking in write mode. */
    if (pWav->onWrite != NULL) {
        return false;
    }

    /* If there are no samples, just return true without doing anything. */
    if (pWav->totalPCMFrameCount == 0) {
        return true;
    }

    /* Make sure the sample is clamped. */
    if (targetFrameIndex >= pWav->totalPCMFrameCount) {
        targetFrameIndex  = pWav->totalPCMFrameCount - 1;
    }

    /*
    For compressed formats we just use a slow generic seek. If we are seeking forward we just seek forward. If we are going backwards we need
    to seek back to the start.
    */
    if (drwav__is_compressed_format_tag(pWav->translatedFormatTag)) {
        /* TODO: This can be optimized. */
        
        /*
        If we're seeking forward it's simple - just keep reading samples until we hit the sample we're requesting. If we're seeking backwards,
        we first need to seek back to the start and then just do the same thing as a forward seek.
        */
        if (targetFrameIndex < pWav->compressed.iCurrentPCMFrame) {
            if (!drwav_seek_to_first_pcm_frame(pWav)) {
                return false;
            }
        }

        if (targetFrameIndex > pWav->compressed.iCurrentPCMFrame) {
            uint64_t offsetInFrames = targetFrameIndex - pWav->compressed.iCurrentPCMFrame;

            int16_t devnull[2048];
            while (offsetInFrames > 0) {
                uint64_t framesRead = 0;
                uint64_t framesToRead = offsetInFrames;
                if (framesToRead > drwav_countof(devnull)/pWav->channels) {
                    framesToRead = drwav_countof(devnull)/pWav->channels;
                }

                if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
                    framesRead = drwav_read_pcm_frames_s16__msadpcm(pWav, framesToRead, devnull);
                } else if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
                    framesRead = drwav_read_pcm_frames_s16__ima(pWav, framesToRead, devnull);
                } else {
                    assert(false);  /* If this assertion is triggered it means I've implemented a new compressed format but forgot to add a branch for it here. */
                }

                if (framesRead != framesToRead) {
                    return false;
                }

                offsetInFrames -= framesRead;
            }
        }
    } else {
        uint64_t totalSizeInBytes;
        uint64_t currentBytePos;
        uint64_t targetBytePos;
        uint64_t offset;

        totalSizeInBytes = pWav->totalPCMFrameCount * drwav_get_bytes_per_pcm_frame(pWav);
        assert(totalSizeInBytes >= pWav->bytesRemaining);

        currentBytePos = totalSizeInBytes - pWav->bytesRemaining;
        targetBytePos  = targetFrameIndex * drwav_get_bytes_per_pcm_frame(pWav);

        if (currentBytePos < targetBytePos) {
            /* Offset forwards. */
            offset = (targetBytePos - currentBytePos);
        } else {
            /* Offset backwards. */
            if (!drwav_seek_to_first_pcm_frame(pWav)) {
                return false;
            }
            offset = targetBytePos;
        }

        while (offset > 0) {
            int offset32 = ((offset > INT_MAX) ? INT_MAX : (int)offset);
            if (!pWav->onSeek(pWav->pUserData, offset32, drwav_seek_origin_current)) {
                return false;
            }

            pWav->bytesRemaining -= offset32;
            offset -= offset32;
        }
    }

    return true;
}


extern size_t drwav_write_raw(drwav* pWav, size_t bytesToWrite, const void* pData) {
    if (pWav == NULL || bytesToWrite == 0 || pData == NULL) {
        return 0;
    }

    size_t bytesWritten = pWav->onWrite(pWav->pUserData, pData, bytesToWrite);
    pWav->dataChunkDataSize += bytesWritten;

    return bytesWritten;
}


extern uint64_t drwav_write_pcm_frames_le(drwav* pWav, uint64_t framesToWrite, const void* pData) {
    if (pWav == NULL || framesToWrite == 0 || pData == NULL) {
        return 0;
    }

    uint64_t bytesToWrite = ((framesToWrite * pWav->channels * pWav->bitsPerSample) / 8);

    uint64_t bytesWritten = 0;
    const uint8_t* pRunningData = (const uint8_t*)pData;

    while (bytesToWrite > 0) {
        size_t bytesJustWritten = drwav_write_raw(pWav, (size_t)bytesToWrite, pRunningData);
        if (bytesJustWritten == 0) {
            break;
        }

        bytesToWrite -= bytesJustWritten;
        bytesWritten += bytesJustWritten;
        pRunningData += bytesJustWritten;
    }

    return (bytesWritten * 8) / pWav->bitsPerSample / pWav->channels;
}

extern uint64_t drwav_write_pcm_frames_be(drwav* pWav, uint64_t framesToWrite, const void* pData) {
    if (pWav == NULL || framesToWrite == 0 || pData == NULL) {
        return 0;
    }

    uint64_t bytesToWrite = ((framesToWrite * pWav->channels * pWav->bitsPerSample) / 8);

    uint64_t bytesWritten = 0;
    const uint8_t* pRunningData = (const uint8_t*)pData;

    uint32_t bytesPerSample = drwav_get_bytes_per_pcm_frame(pWav) / pWav->channels;
    
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
        drwav__bswap_samples(temp, sampleCount, bytesPerSample, pWav->translatedFormatTag);

        bytesJustWritten = drwav_write_raw(pWav, (size_t)bytesToWriteThisIteration, temp);
        if (bytesJustWritten == 0) {
            break;
        }

        bytesToWrite -= bytesJustWritten;
        bytesWritten += bytesJustWritten;
        pRunningData += bytesJustWritten;
    }

    return (bytesWritten * 8) / pWav->bitsPerSample / pWav->channels;
}

extern uint64_t drwav_write_pcm_frames(drwav* pWav, uint64_t framesToWrite, const void* pData) {
    if (drwav__is_little_endian()) {
        return drwav_write_pcm_frames_le(pWav, framesToWrite, pData);
    } else {
        return drwav_write_pcm_frames_be(pWav, framesToWrite, pData);
    }
}


static uint64_t drwav_read_pcm_frames_s16__msadpcm(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint64_t totalFramesRead = 0;

    assert(pWav != NULL);
    assert(framesToRead > 0);

    /* TODO: Lots of room for optimization here. */

    while (framesToRead > 0 && pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount) {
        /* If there are no cached frames we need to load a new block. */
        if (pWav->msadpcm.cachedFrameCount == 0 && pWav->msadpcm.bytesRemainingInBlock == 0) {
            if (pWav->channels == 1) {
                /* Mono. */
                uint8_t header[7];
                if (pWav->onRead(pWav->pUserData, header, sizeof(header)) != sizeof(header)) {
                    return totalFramesRead;
                }
                pWav->msadpcm.bytesRemainingInBlock = pWav->fmt.blockAlign - sizeof(header);

                pWav->msadpcm.predictor[0]     = header[0];
                pWav->msadpcm.delta[0]         = drwav__bytes_to_s16(header + 1);
                pWav->msadpcm.prevFrames[0][1] = (int32_t)drwav__bytes_to_s16(header + 3);
                pWav->msadpcm.prevFrames[0][0] = (int32_t)drwav__bytes_to_s16(header + 5);
                pWav->msadpcm.cachedFrames[2]  = pWav->msadpcm.prevFrames[0][0];
                pWav->msadpcm.cachedFrames[3]  = pWav->msadpcm.prevFrames[0][1];
                pWav->msadpcm.cachedFrameCount = 2;
            } else {
                /* Stereo. */
                uint8_t header[14];
                if (pWav->onRead(pWav->pUserData, header, sizeof(header)) != sizeof(header)) {
                    return totalFramesRead;
                }
                pWav->msadpcm.bytesRemainingInBlock = pWav->fmt.blockAlign - sizeof(header);

                pWav->msadpcm.predictor[0] = header[0];
                pWav->msadpcm.predictor[1] = header[1];
                pWav->msadpcm.delta[0] = drwav__bytes_to_s16(header + 2);
                pWav->msadpcm.delta[1] = drwav__bytes_to_s16(header + 4);
                pWav->msadpcm.prevFrames[0][1] = (int32_t)drwav__bytes_to_s16(header + 6);
                pWav->msadpcm.prevFrames[1][1] = (int32_t)drwav__bytes_to_s16(header + 8);
                pWav->msadpcm.prevFrames[0][0] = (int32_t)drwav__bytes_to_s16(header + 10);
                pWav->msadpcm.prevFrames[1][0] = (int32_t)drwav__bytes_to_s16(header + 12);

                pWav->msadpcm.cachedFrames[0] = pWav->msadpcm.prevFrames[0][0];
                pWav->msadpcm.cachedFrames[1] = pWav->msadpcm.prevFrames[1][0];
                pWav->msadpcm.cachedFrames[2] = pWav->msadpcm.prevFrames[0][1];
                pWav->msadpcm.cachedFrames[3] = pWav->msadpcm.prevFrames[1][1];
                pWav->msadpcm.cachedFrameCount = 2;
            }
        }

        /* Output anything that's cached. */
        while (framesToRead > 0 && pWav->msadpcm.cachedFrameCount > 0 && pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount) {
            if (pBufferOut != NULL) {
                uint32_t iSample = 0;
                for (iSample = 0; iSample < pWav->channels; iSample += 1) {
                    pBufferOut[iSample] = (int16_t)pWav->msadpcm.cachedFrames[(drwav_countof(pWav->msadpcm.cachedFrames) - (pWav->msadpcm.cachedFrameCount*pWav->channels)) + iSample];
                }

                pBufferOut += pWav->channels;
            }

            framesToRead    -= 1;
            totalFramesRead += 1;
            pWav->compressed.iCurrentPCMFrame += 1;
            pWav->msadpcm.cachedFrameCount -= 1;
        }

        if (framesToRead == 0) {
            return totalFramesRead;
        }


        /*
        If there's nothing left in the cache, just go ahead and load more. If there's nothing left to load in the current block we just continue to the next
        loop iteration which will trigger the loading of a new block.
        */
        if (pWav->msadpcm.cachedFrameCount == 0) {
            if (pWav->msadpcm.bytesRemainingInBlock == 0) {
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

                if (pWav->onRead(pWav->pUserData, &nibbles, 1) != 1) {
                    return totalFramesRead;
                }
                pWav->msadpcm.bytesRemainingInBlock -= 1;

                /* TODO: Optimize away these if statements. */
                nibble0 = ((nibbles & 0xF0) >> 4); if ((nibbles & 0x80)) { nibble0 |= 0xFFFFFFF0UL; }
                nibble1 = ((nibbles & 0x0F) >> 0); if ((nibbles & 0x08)) { nibble1 |= 0xFFFFFFF0UL; }

                if (pWav->channels == 1) {
                    /* Mono. */
                    int32_t newSample0;
                    int32_t newSample1;

                    newSample0  = ((pWav->msadpcm.prevFrames[0][1] * coeff1Table[pWav->msadpcm.predictor[0]]) + (pWav->msadpcm.prevFrames[0][0] * coeff2Table[pWav->msadpcm.predictor[0]])) >> 8;
                    newSample0 += nibble0 * pWav->msadpcm.delta[0];
                    newSample0  = wav_clamp(newSample0, -32768, 32767);

                    pWav->msadpcm.delta[0] = (adaptationTable[((nibbles & 0xF0) >> 4)] * pWav->msadpcm.delta[0]) >> 8;
                    if (pWav->msadpcm.delta[0] < 16) {
                        pWav->msadpcm.delta[0] = 16;
                    }

                    pWav->msadpcm.prevFrames[0][0] = pWav->msadpcm.prevFrames[0][1];
                    pWav->msadpcm.prevFrames[0][1] = newSample0;


                    newSample1  = ((pWav->msadpcm.prevFrames[0][1] * coeff1Table[pWav->msadpcm.predictor[0]]) + (pWav->msadpcm.prevFrames[0][0] * coeff2Table[pWav->msadpcm.predictor[0]])) >> 8;
                    newSample1 += nibble1 * pWav->msadpcm.delta[0];
                    newSample1  = wav_clamp(newSample1, -32768, 32767);

                    pWav->msadpcm.delta[0] = (adaptationTable[((nibbles & 0x0F) >> 0)] * pWav->msadpcm.delta[0]) >> 8;
                    if (pWav->msadpcm.delta[0] < 16) {
                        pWav->msadpcm.delta[0] = 16;
                    }

                    pWav->msadpcm.prevFrames[0][0] = pWav->msadpcm.prevFrames[0][1];
                    pWav->msadpcm.prevFrames[0][1] = newSample1;


                    pWav->msadpcm.cachedFrames[2] = newSample0;
                    pWav->msadpcm.cachedFrames[3] = newSample1;
                    pWav->msadpcm.cachedFrameCount = 2;
                } else {
                    /* Stereo. */
                    int32_t newSample0;
                    int32_t newSample1;

                    /* Left. */
                    newSample0  = ((pWav->msadpcm.prevFrames[0][1] * coeff1Table[pWav->msadpcm.predictor[0]]) + (pWav->msadpcm.prevFrames[0][0] * coeff2Table[pWav->msadpcm.predictor[0]])) >> 8;
                    newSample0 += nibble0 * pWav->msadpcm.delta[0];
                    newSample0  = wav_clamp(newSample0, -32768, 32767);

                    pWav->msadpcm.delta[0] = (adaptationTable[((nibbles & 0xF0) >> 4)] * pWav->msadpcm.delta[0]) >> 8;
                    if (pWav->msadpcm.delta[0] < 16) {
                        pWav->msadpcm.delta[0] = 16;
                    }

                    pWav->msadpcm.prevFrames[0][0] = pWav->msadpcm.prevFrames[0][1];
                    pWav->msadpcm.prevFrames[0][1] = newSample0;


                    /* Right. */
                    newSample1  = ((pWav->msadpcm.prevFrames[1][1] * coeff1Table[pWav->msadpcm.predictor[1]]) + (pWav->msadpcm.prevFrames[1][0] * coeff2Table[pWav->msadpcm.predictor[1]])) >> 8;
                    newSample1 += nibble1 * pWav->msadpcm.delta[1];
                    newSample1  = wav_clamp(newSample1, -32768, 32767);

                    pWav->msadpcm.delta[1] = (adaptationTable[((nibbles & 0x0F) >> 0)] * pWav->msadpcm.delta[1]) >> 8;
                    if (pWav->msadpcm.delta[1] < 16) {
                        pWav->msadpcm.delta[1] = 16;
                    }

                    pWav->msadpcm.prevFrames[1][0] = pWav->msadpcm.prevFrames[1][1];
                    pWav->msadpcm.prevFrames[1][1] = newSample1;

                    pWav->msadpcm.cachedFrames[2] = newSample0;
                    pWav->msadpcm.cachedFrames[3] = newSample1;
                    pWav->msadpcm.cachedFrameCount = 1;
                }
            }
        }
    }

    return totalFramesRead;
}


static uint64_t drwav_read_pcm_frames_s16__ima(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
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

    assert(pWav != NULL);
    assert(framesToRead > 0);

    /* TODO: Lots of room for optimization here. */

    while (framesToRead > 0 && pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount) {
        /* If there are no cached samples we need to load a new block. */
        if (pWav->ima.cachedFrameCount == 0 && pWav->ima.bytesRemainingInBlock == 0) {
            if (pWav->channels == 1) {
                /* Mono. */
                uint8_t header[4];
                if (pWav->onRead(pWav->pUserData, header, sizeof(header)) != sizeof(header)) {
                    return totalFramesRead;
                }
                pWav->ima.bytesRemainingInBlock = pWav->fmt.blockAlign - sizeof(header);

                if (header[2] >= drwav_countof(stepTable)) {
                    pWav->onSeek(pWav->pUserData, pWav->ima.bytesRemainingInBlock, drwav_seek_origin_current);
                    pWav->ima.bytesRemainingInBlock = 0;
                    return totalFramesRead; /* Invalid data. */
                }

                pWav->ima.predictor[0] = drwav__bytes_to_s16(header + 0);
                pWav->ima.stepIndex[0] = header[2];
                pWav->ima.cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 1] = pWav->ima.predictor[0];
                pWav->ima.cachedFrameCount = 1;
            } else {
                /* Stereo. */
                uint8_t header[8];
                if (pWav->onRead(pWav->pUserData, header, sizeof(header)) != sizeof(header)) {
                    return totalFramesRead;
                }
                pWav->ima.bytesRemainingInBlock = pWav->fmt.blockAlign - sizeof(header);

                if (header[2] >= drwav_countof(stepTable) || header[6] >= drwav_countof(stepTable)) {
                    pWav->onSeek(pWav->pUserData, pWav->ima.bytesRemainingInBlock, drwav_seek_origin_current);
                    pWav->ima.bytesRemainingInBlock = 0;
                    return totalFramesRead; /* Invalid data. */
                }

                pWav->ima.predictor[0] = drwav__bytes_to_s16(header + 0);
                pWav->ima.stepIndex[0] = header[2];
                pWav->ima.predictor[1] = drwav__bytes_to_s16(header + 4);
                pWav->ima.stepIndex[1] = header[6];

                pWav->ima.cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 2] = pWav->ima.predictor[0];
                pWav->ima.cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 1] = pWav->ima.predictor[1];
                pWav->ima.cachedFrameCount = 1;
            }
        }

        /* Output anything that's cached. */
        while (framesToRead > 0 && pWav->ima.cachedFrameCount > 0 && pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount) {
            if (pBufferOut != NULL) {
                uint32_t iSample;
                for (iSample = 0; iSample < pWav->channels; iSample += 1) {
                    pBufferOut[iSample] = (int16_t)pWav->ima.cachedFrames[(drwav_countof(pWav->ima.cachedFrames) - (pWav->ima.cachedFrameCount*pWav->channels)) + iSample];
                }
                pBufferOut += pWav->channels;
            }

            framesToRead    -= 1;
            totalFramesRead += 1;
            pWav->compressed.iCurrentPCMFrame += 1;
            pWav->ima.cachedFrameCount -= 1;
        }

        if (framesToRead == 0) {
            return totalFramesRead;
        }

        /*
        If there's nothing left in the cache, just go ahead and load more. If there's nothing left to load in the current block we just continue to the next
        loop iteration which will trigger the loading of a new block.
        */
        if (pWav->ima.cachedFrameCount == 0) {
            if (pWav->ima.bytesRemainingInBlock == 0) {
                continue;
            } else {
                /*
                From what I can tell with stereo streams, it looks like every 4 bytes (8 samples) is for one channel. So it goes 4 bytes for the
                left channel, 4 bytes for the right channel.
                */
                pWav->ima.cachedFrameCount = 8;
                for (uint32_t iChannel = 0; iChannel < pWav->channels; ++iChannel) {
                    uint32_t iByte;
                    uint8_t nibbles[4];
                    if (pWav->onRead(pWav->pUserData, &nibbles, 4) != 4) {
                        pWav->ima.cachedFrameCount = 0;
                        return totalFramesRead;
                    }
                    pWav->ima.bytesRemainingInBlock -= 4;

                    for (iByte = 0; iByte < 4; ++iByte) {
                        uint8_t nibble0 = ((nibbles[iByte] & 0x0F) >> 0);
                        uint8_t nibble1 = ((nibbles[iByte] & 0xF0) >> 4);

                        int32_t step      = stepTable[pWav->ima.stepIndex[iChannel]];
                        int32_t predictor = pWav->ima.predictor[iChannel];

                        int32_t      diff  = step >> 3;
                        if (nibble0 & 1) diff += step >> 2;
                        if (nibble0 & 2) diff += step >> 1;
                        if (nibble0 & 4) diff += step;
                        if (nibble0 & 8) diff  = -diff;

                        predictor = wav_clamp(predictor + diff, -32768, 32767);
                        pWav->ima.predictor[iChannel] = predictor;
                        pWav->ima.stepIndex[iChannel] = wav_clamp(pWav->ima.stepIndex[iChannel] + indexTable[nibble0], 0, (int32_t)drwav_countof(stepTable)-1);
                        pWav->ima.cachedFrames[(drwav_countof(pWav->ima.cachedFrames) - (pWav->ima.cachedFrameCount*pWav->channels)) + (iByte*2+0)*pWav->channels + iChannel] = predictor;


                        step      = stepTable[pWav->ima.stepIndex[iChannel]];
                        predictor = pWav->ima.predictor[iChannel];

                                         diff  = step >> 3;
                        if (nibble1 & 1) diff += step >> 2;
                        if (nibble1 & 2) diff += step >> 1;
                        if (nibble1 & 4) diff += step;
                        if (nibble1 & 8) diff  = -diff;

                        predictor = wav_clamp(predictor + diff, -32768, 32767);
                        pWav->ima.predictor[iChannel] = predictor;
                        pWav->ima.stepIndex[iChannel] = wav_clamp(pWav->ima.stepIndex[iChannel] + indexTable[nibble1], 0, (int32_t)drwav_countof(stepTable)-1);
                        pWav->ima.cachedFrames[(drwav_countof(pWav->ima.cachedFrames) - (pWav->ima.cachedFrameCount*pWav->channels)) + (iByte*2+1)*pWav->channels + iChannel] = predictor;
                    }
                }
            }
        }
    }

    return totalFramesRead;
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
        unsigned int shift  = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift  += 8;
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

static uint64_t drwav_read_pcm_frames_s16__pcm(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint8_t sampleData[4096];

    /* Fast path. */
    if ((pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM && pWav->bitsPerSample == 16) || pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
    }
    
    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;
    
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__pcm_to_s16(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels), bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s16__ieee(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint8_t sampleData[4096];

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;
    
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__ieee_to_s16(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels), bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s16__alaw(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint8_t sampleData[4096];

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;
    
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_alaw_to_s16(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s16__mulaw(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint8_t sampleData[4096];

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_mulaw_to_s16(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

extern uint64_t drwav_read_pcm_frames_s16(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    if (pWav == NULL || framesToRead == 0) {
        return 0;
    }

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM) {
        return drwav_read_pcm_frames_s16__pcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
        return drwav_read_pcm_frames_s16__ieee(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW) {
        return drwav_read_pcm_frames_s16__alaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW) {
        return drwav_read_pcm_frames_s16__mulaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
        return drwav_read_pcm_frames_s16__msadpcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
        return drwav_read_pcm_frames_s16__ima(pWav, framesToRead, pBufferOut);
    }

    return 0;
}

extern uint64_t drwav_read_pcm_frames_s16le(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == false) {
        drwav__bswap_samples_s16(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
}

extern uint64_t drwav_read_pcm_frames_s16be(drwav* pWav, uint64_t framesToRead, int16_t* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == true) {
        drwav__bswap_samples_s16(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
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
        int x = ((int)(((unsigned int)(((const uint8_t*)pIn)[i*3+0]) << 8) | ((unsigned int)(((const uint8_t*)pIn)[i*3+1]) << 16) | ((unsigned int)(((const uint8_t*)pIn)[i*3+2])) << 24)) >> 8;
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
        unsigned int shift  = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift  += 8;
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


static uint64_t drwav_read_pcm_frames_f32__pcm(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint8_t sampleData[4096];

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__pcm_to_f32(pBufferOut, sampleData, (size_t)framesRead*pWav->channels, bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_f32__msadpcm(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    /*
    We're just going to borrow the implementation from the drwav_read_s16() since ADPCM is a little bit more complicated than other formats and I don't
    want to duplicate that code.
    */
    uint64_t totalFramesRead = 0;
    int16_t samples16[2048];
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, std::min(framesToRead, drwav_countof(samples16)/pWav->channels), samples16);
        if (framesRead == 0) {
            break;
        }

        drwav_s16_to_f32(pBufferOut, samples16, (size_t)(framesRead*pWav->channels));   /* <-- Safe cast because we're clamping to 2048. */

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_f32__ima(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    /*
    We're just going to borrow the implementation from the drwav_read_s16() since IMA-ADPCM is a little bit more complicated than other formats and I don't
    want to duplicate that code.
    */
    uint64_t totalFramesRead = 0;
    int16_t samples16[2048];
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, std::min(framesToRead, drwav_countof(samples16)/pWav->channels), samples16);
        if (framesRead == 0) {
            break;
        }

        drwav_s16_to_f32(pBufferOut, samples16, (size_t)(framesRead*pWav->channels));   /* <-- Safe cast because we're clamping to 2048. */

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_f32__ieee(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint8_t sampleData[4096];

    /* Fast path. */
    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT && pWav->bitsPerSample == 32) {
        return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
    }
    
    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__ieee_to_f32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels), bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_f32__alaw(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint8_t sampleData[4096];
    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_alaw_to_f32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_f32__mulaw(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint8_t sampleData[4096];

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_mulaw_to_f32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

extern uint64_t drwav_read_pcm_frames_f32(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    if (pWav == NULL || framesToRead == 0) {
        return 0;
    }

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM) {
        return drwav_read_pcm_frames_f32__pcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
        return drwav_read_pcm_frames_f32__msadpcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
        return drwav_read_pcm_frames_f32__ieee(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW) {
        return drwav_read_pcm_frames_f32__alaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW) {
        return drwav_read_pcm_frames_f32__mulaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
        return drwav_read_pcm_frames_f32__ima(pWav, framesToRead, pBufferOut);
    }

    return 0;
}

extern uint64_t drwav_read_pcm_frames_f32le(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_f32(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == false) {
        drwav__bswap_samples_f32(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
}

extern uint64_t drwav_read_pcm_frames_f32be(drwav* pWav, uint64_t framesToRead, float* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_f32(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == true) {
        drwav__bswap_samples_f32(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
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
        double x = (double)(((int32_t)(((uint32_t)(pIn[i*3+0]) << 8) | ((uint32_t)(pIn[i*3+1]) << 16) | ((uint32_t)(pIn[i*3+2])) << 24)) >> 8);
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
        unsigned int shift  = (8 - bytesPerSample) * 8;

        unsigned int j;
        for (j = 0; j < bytesPerSample; j += 1) {
            assert(j < 8);
            sample |= (uint64_t)(pIn[j]) << shift;
            shift  += 8;
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


static uint64_t drwav_read_pcm_frames_s32__pcm(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint8_t sampleData[4096];

    /* Fast path. */
    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM && pWav->bitsPerSample == 32) {
        return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
    }
    
    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__pcm_to_s32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels), bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s32__msadpcm(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    /*
    We're just going to borrow the implementation from the drwav_read_s16() since ADPCM is a little bit more complicated than other formats and I don't
    want to duplicate that code.
    */
    uint64_t totalFramesRead = 0;
    int16_t samples16[2048];
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, std::min(framesToRead, drwav_countof(samples16)/pWav->channels), samples16);
        if (framesRead == 0) {
            break;
        }

        drwav_s16_to_s32(pBufferOut, samples16, (size_t)(framesRead*pWav->channels));   /* <-- Safe cast because we're clamping to 2048. */

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s32__ima(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    /*
    We're just going to borrow the implementation from the drwav_read_s16() since IMA-ADPCM is a little bit more complicated than other formats and I don't
    want to duplicate that code.
    */
    uint64_t totalFramesRead = 0;
    int16_t samples16[2048];
    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, std::min(framesToRead, drwav_countof(samples16)/pWav->channels), samples16);
        if (framesRead == 0) {
            break;
        }

        drwav_s16_to_s32(pBufferOut, samples16, (size_t)(framesRead*pWav->channels));   /* <-- Safe cast because we're clamping to 2048. */

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s32__ieee(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint8_t sampleData[4096];

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav__ieee_to_s32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels), bytesPerFrame/pWav->channels);

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s32__alaw(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint8_t sampleData[4096];

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_alaw_to_s32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

static uint64_t drwav_read_pcm_frames_s32__mulaw(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint8_t sampleData[4096];

    uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
    if (bytesPerFrame == 0) {
        return 0;
    }

    uint64_t totalFramesRead = 0;

    while (framesToRead > 0) {
        uint64_t framesRead = drwav_read_pcm_frames(pWav, std::min(framesToRead, sizeof(sampleData)/bytesPerFrame), sampleData);
        if (framesRead == 0) {
            break;
        }

        drwav_mulaw_to_s32(pBufferOut, sampleData, (size_t)(framesRead*pWav->channels));

        pBufferOut      += framesRead*pWav->channels;
        framesToRead    -= framesRead;
        totalFramesRead += framesRead;
    }

    return totalFramesRead;
}

extern uint64_t drwav_read_pcm_frames_s32(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    if (pWav == NULL || framesToRead == 0) {
        return 0;
    }

    if (pBufferOut == NULL) {
        return drwav_read_pcm_frames(pWav, framesToRead, NULL);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM) {
        return drwav_read_pcm_frames_s32__pcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM) {
        return drwav_read_pcm_frames_s32__msadpcm(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
        return drwav_read_pcm_frames_s32__ieee(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW) {
        return drwav_read_pcm_frames_s32__alaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW) {
        return drwav_read_pcm_frames_s32__mulaw(pWav, framesToRead, pBufferOut);
    }

    if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) {
        return drwav_read_pcm_frames_s32__ima(pWav, framesToRead, pBufferOut);
    }

    return 0;
}

extern uint64_t drwav_read_pcm_frames_s32le(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_s32(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == false) {
        drwav__bswap_samples_s32(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
}

extern uint64_t drwav_read_pcm_frames_s32be(drwav* pWav, uint64_t framesToRead, int32_t* pBufferOut) {
    uint64_t framesRead = drwav_read_pcm_frames_s32(pWav, framesToRead, pBufferOut);
    if (pBufferOut != NULL && drwav__is_little_endian() == true) {
        drwav__bswap_samples_s32(pBufferOut, framesRead*pWav->channels);
    }

    return framesRead;
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
        unsigned int s0 = pIn[i*3 + 0];
        unsigned int s1 = pIn[i*3 + 1];
        unsigned int s2 = pIn[i*3 + 2];

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



static int16_t* drwav__read_pcm_frames_and_close_s16(drwav* pWav, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount) {
    assert(pWav != NULL);

    uint64_t sampleDataSize = pWav->totalPCMFrameCount * pWav->channels * sizeof(int16_t);

    int16_t* pSampleData = (int16_t*)malloc((size_t)sampleDataSize);
    if (pSampleData == NULL) {
        drwav_uninit(pWav);
        return NULL;    /* Failed to allocate memory. */
    }

    uint64_t framesRead = drwav_read_pcm_frames_s16(pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
    if (framesRead != pWav->totalPCMFrameCount) {
        free(pSampleData);
        drwav_uninit(pWav);
        return NULL;    /* There was an error reading the samples. */
    }

    drwav_uninit(pWav);

    if (sampleRate) {
        *sampleRate = pWav->sampleRate;
    }
    if (channels) {
        *channels = pWav->channels;
    }
    if (totalFrameCount) {
        *totalFrameCount = pWav->totalPCMFrameCount;
    }

    return pSampleData;
}

static float* drwav__read_pcm_frames_and_close_f32(drwav* pWav, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount) {
    assert(pWav != NULL);

    uint64_t sampleDataSize = pWav->totalPCMFrameCount * pWav->channels * sizeof(float);

    float* pSampleData = (float*)drwav__malloc_from_callbacks((size_t)sampleDataSize); /* <-- Safe cast due to the check above. */
    if (pSampleData == NULL) {
        drwav_uninit(pWav);
        return NULL;    /* Failed to allocate memory. */
    }

    uint64_t framesRead = drwav_read_pcm_frames_f32(pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
    if (framesRead != pWav->totalPCMFrameCount) {
        free(pSampleData);
        drwav_uninit(pWav);
        return NULL;    /* There was an error reading the samples. */
    }

    drwav_uninit(pWav);

    if (sampleRate) {
        *sampleRate = pWav->sampleRate;
    }
    if (channels) {
        *channels = pWav->channels;
    }
    if (totalFrameCount) {
        *totalFrameCount = pWav->totalPCMFrameCount;
    }

    return pSampleData;
}

static int32_t* drwav__read_pcm_frames_and_close_s32(drwav* pWav, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount) {
    assert(pWav != NULL);

    uint64_t sampleDataSize = pWav->totalPCMFrameCount * pWav->channels * sizeof(int32_t);

    int32_t* pSampleData = (int32_t*)drwav__malloc_from_callbacks((size_t)sampleDataSize); /* <-- Safe cast due to the check above. */
    if (pSampleData == NULL) {
        drwav_uninit(pWav);
        return NULL;    /* Failed to allocate memory. */
    }

    uint64_t framesRead = drwav_read_pcm_frames_s32(pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
    if (framesRead != pWav->totalPCMFrameCount) {
        free(pSampleData);
        drwav_uninit(pWav);
        return NULL;    /* There was an error reading the samples. */
    }

    drwav_uninit(pWav);

    if (sampleRate) {
        *sampleRate = pWav->sampleRate;
    }
    if (channels) {
        *channels = pWav->channels;
    }
    if (totalFrameCount) {
        *totalFrameCount = pWav->totalPCMFrameCount;
    }

    return pSampleData;
}



extern int16_t* drwav_open_and_read_pcm_frames_s16(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init(&wav, onRead, onSeek, pUserData)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_s16(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

extern float* drwav_open_and_read_pcm_frames_f32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init(&wav, onRead, onSeek, pUserData)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_f32(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

extern int32_t* drwav_open_and_read_pcm_frames_s32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init(&wav, onRead, onSeek, pUserData)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_s32(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

extern int16_t* drwav_open_file_and_read_pcm_frames_s16(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init_file(&wav, filename)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_s16(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

extern float* drwav_open_file_and_read_pcm_frames_f32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init_file(&wav, filename)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_f32(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

extern int32_t* drwav_open_file_and_read_pcm_frames_s32(const char* filename, unsigned int* channelsOut, unsigned int* sampleRateOut, uint64_t* totalFrameCountOut) {
    drwav wav;

    if (channelsOut) {
        *channelsOut = 0;
    }
    if (sampleRateOut) {
        *sampleRateOut = 0;
    }
    if (totalFrameCountOut) {
        *totalFrameCountOut = 0;
    }

    if (!drwav_init_file(&wav, filename)) {
        return NULL;
    }

    return drwav__read_pcm_frames_and_close_s32(&wav, channelsOut, sampleRateOut, totalFrameCountOut);
}
