//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

// #include <stdio.h>
//
// #include "m_misc.h"
// #include "w_file.h"
// #include "z_zone.h"
#include <xinu.h>

typedef struct
{
    wad_file_t wad;
    unsigned char *fstream;
    //FILE *fstream;
} stdc_wad_file_t;

extern wad_file_class_t stdc_wad_file;

extern unsigned char _binary_doom1_wad_start[];
extern unsigned char _binary_doom1_wad_end[];

static wad_file_t *W_StdC_OpenFile(char *path)
{
    stdc_wad_file_t *result;
    //FILE *fstream;

    //fstream = fopen(path, "rb");

    //if (fstream == NULL)
    //{
    //    return NULL;
    //}

    // Create a new stdc_wad_file_t to hold the file handle.
    (void) path;

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    //result->wad.length = M_FileLength(fstream);
    result->wad.length = (unsigned int) (_binary_doom1_wad_end - _binary_doom1_wad_start);
    kprintf("WAD LENGTH: %d\n", result->wad.length);
    //result->fstream = fstream;
    result->fstream = _binary_doom1_wad_start;

    return &result->wad;
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    //fclose(stdc_wad->fstream);
    Z_Free(stdc_wad);
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
    stdc_wad_file_t *stdc_wad;
    //size_t result;
    size_t remaining;
    size_t to_copy;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

    //fseek(stdc_wad->fstream, offset, SEEK_SET);

    // Read into the buffer.

    //result = fread(buffer, 1, buffer_len, stdc_wad->fstream);

    if (offset >= stdc_wad->wad.length)
    {
      return 0;
    }

    remaining = stdc_wad->wad.length - offset;
    to_copy = (buffer_len < remaining) ? buffer_len : remaining;

    memcpy(buffer, stdc_wad->fstream + offset, to_copy);

    //return result;
    return to_copy;
}


wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
};


