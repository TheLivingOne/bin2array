// Simple, but very fast converter of any file to C++ array; written in old school C.
// About 40 times faster than bin2c.py and about 6 times faster than xxd.
// Compilation: gcc|clang|etc -O3 bin2array.c -o bin2array
// (C) Sergey A. Galin, 2019, sergey.galin@gmail.com, sergey.galin@yandex.ru
// This file is a Public Domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char * argv[])
{
    if (argc != 4) {
        printf("USAGE: %s <input file> <output file base name> <array name>\n", argv[0]);
        return 1;
    }

    const char * in = argv[1];
    const char * out = argv[2];
    const char * array = argv[3];

    // Hello stack overflow :)
    char out_h[4096], out_cpp[4096];
    sprintf(out_h, "%s.h", out);
    sprintf(out_cpp, "%s.cpp", out);

    const char * out_h_slashpos = strrchr(out_h, '/');
    #if defined(_MSC_VER)
        if (!out_h_slashpos) out_h_slashpos = strrchr(out_h, '\\');
    #endif
    const char * out_h_filename = (out_h_slashpos) ? (out_h_slashpos + 1) : out_h;

    printf("Input: %s, output basename: %s, array name: %s\n", in, out, array);

    //
    // Working with the input file
    //
    FILE * fin = fopen(in, "rb");
    if (!fin) {
        printf("Error opening input file!\n");
        return 2;
    }
    fseek(fin, 0, SEEK_END);
    size_t size = (size_t)ftell(fin);
    fseek(fin, 0, SEEK_SET);
    printf("Input data size: %ld\n", (long)size);
    // Loading all input data into memory at once
    unsigned char * data = malloc(size);
    if (fread(data, size, 1, fin) != 1) {
        printf("Failed to read input file!\n");
        free(data);
        fclose(fin);
        return 2;
    }
    fclose(fin);

    //
    // Converting input data to C code
    //
    unsigned char * text = malloc(size * 5);
    unsigned char * out_ptr = text;
    unsigned char * in_ptr = data;
    for (size_t i = 1, col = 1; i <= size; i++, in_ptr++) {
        unsigned char x = *in_ptr;
        // Converting unsigned char to code - much faster than itoa/printf/and etc.
        // Using decimal code because it's much more compact than hex code.
        if (x >= 10) {
            if (x >= 100) {
                *out_ptr++ = '0' + ((x / 100u) % 10u);
            }
            *out_ptr++ = '0' + ((x / 10u) % 10u);
        }
        *out_ptr++ = '0' + (x % 10u);
        if (i != size) {
            *out_ptr++ = ',';
        }
        if (col == 20) {
            *out_ptr++ = '\n';
            col = 1;
        } else
            col++;
    }
    free(data);
    // *out_ptr = 0; not necessary as we're using fwrite()

    //
    // Writing output .h & .cpp files
    //
    FILE * fout_h = fopen(out_h, "wb"); // Always using '\n'
    fprintf(
        fout_h,
        "#pragma once\n"
        "#include <cstddef>\n"
        "\n"
        "extern const size_t %s_size;\n"
        "extern const unsigned char %s[];\n",
        array,
        array);
    fclose(fout_h);
    FILE * fout = fopen(out_cpp, "wb");
    if (!fout) {
        printf("Error opening output file!\n");
        free(text);
        return 2;
    }
    fprintf(
        fout,
        "#include \"%s\"\n"
        "\n"
        "#if defined(_MSC_VER)\n"
        "    #define BIN2ARRAY_ALIGN __declspec(align(8))\n"
        "#else\n"
        "    #define BIN2ARRAY_ALIGN __attribute__((__aligned__(8)))\n"
        "#endif\n"
        "\n"
        "const size_t %s_size = %ldu;\n"
        "\n"
        "const unsigned char BIN2ARRAY_ALIGN %s[%s_size] = {\n",
        out_h_filename,
        array,
        size,
        array,
        array);
    if (fwrite(text, out_ptr - text, 1, fout) != 1) {
        printf("Error writing output file!");
        free(text);
        fclose(fout);
        return 2;
    }
    fprintf(fout, "};\n");
    fclose(fout);
    free(text);
    return 0;
}

