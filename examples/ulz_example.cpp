//MIT License
//
//Copyright(c) 2019 Alex Kasitskyi
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

#include <c4/ulz.hpp>

#include <c4/cmd_opts.hpp>

#include <cstdio>

int main(int argc, char* argv[]) {
    try
    {
        if (argc < 3) {
            fprintf(stderr,
                "ULZ - An ultra-fast LZ77 compressor, v1.01 BETA\n"
                "Written and placed in the public domain by Ilya Muravyov\n"
                "\n"
                "Usage: ULZ command infile [outfile]\n"
                "\n"
                "Commands:\n"
                "  c[#] Compress; Set the level of compression (1..9)\n"
                "  d    Decompress\n");
            exit(1);
        }

        clock_t start = clock();

        FILE* in = fopen(argv[2], "rb");
        if (!in)
        {
            perror(argv[2]);
            exit(1);
        }

        char out_name[FILENAME_MAX];
        if (argc < 4)
        {
            strcpy(out_name, argv[2]);
            if (*argv[1] == 'd')
            {
                const int p = strlen(out_name) - 4;
                if (p > 0 && strcmp(&out_name[p], ".ulz") == 0)
                    out_name[p] = '\0';
                else
                    strcat(out_name, ".out");
            } else
                strcat(out_name, ".ulz");
        } else
            strcpy(out_name, argv[3]);

        FILE* out = fopen(out_name, "wb");
        if (!out) {
            perror(out_name);
            exit(1);
        }

        if (*argv[1] == 'c') {
            int level = (argv[1][1] != '\0') ? atoi(&argv[1][1]) : 1;

            ASSERT_TRUE(level >= 1 && level <= 9);

            printf("Compressing %s:\n", argv[2]);

            const int magic = ULZ_MAGIC;
            fwrite(&magic, 1, sizeof(magic), out);

            compress(in, out, level);
        } else if (*argv[1] == 'd') {
            int magic;
            fread(&magic, 1, sizeof(magic), in);
            ASSERT_EQUAL(magic, ULZ_MAGIC);

            printf("Decompressing %s:\n", argv[2]);

            if (decompress(in, out) != 0)
            {
                fprintf(stderr, "%s: Corrupt input\n", argv[2]);
                exit(1);
            }
        } else
        {
            fprintf(stderr, "Unknown command: %s\n", argv[1]);
            exit(1);
        }

        printf("%lld -> %lld in %1.3f sec\n", _ftelli64(in), _ftelli64(out),
            double(clock() - start) / CLOCKS_PER_SEC);

        fclose(in);
        fclose(out);

    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
