#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include "endian.h"

// https://psdevwiki.com/ps3/PARAM.SFO#Internal_Structure
struct sfo_header
{
    uint32_t magic;            /************ Always PSF */
    uint32_t version;          /********** Usually 1.1 */
    uint32_t key_table_start;  /** Start offset of key_table */
    uint32_t data_table_start; /* Start offset of data_table */
    uint32_t tables_entries;   /*** Number of entries in all tables */
};

struct sfo_index_table_entry
{
    uint16_t key_offset;   /*** param_key offset (relative to start offset of key_table) */
    uint16_t data_fmt;     /***** param_data data type */
    uint32_t data_len;     /***** param_data used bytes */
    uint32_t data_max_len; /* param_data total bytes */
    uint32_t data_offset;  /** param_data offset (relative to start offset of data_table) */
};

char *get_title(char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        SDL_Log("Unable to open %s", path);
        return NULL;
    }

    struct sfo_header header = {0};
    fread(&header, sizeof(struct sfo_header), 1, file);

    // byteswap since file is LE and we're on BE
    header.magic = _ES32(header.magic);
    header.version = _ES32(header.version);
    header.key_table_start = _ES32(header.key_table_start);
    header.data_table_start = _ES32(header.data_table_start);
    header.tables_entries = _ES32(header.tables_entries);

    // Check for header of \0PSF
    if (header.magic != _ES32(*((uint32_t *)"\0PSF")))
    {
        SDL_Log("Invalid magic: %x", header.magic);
        return NULL;
    }

    for (int i = 0; i < header.tables_entries; i++)
    {
        struct sfo_index_table_entry entry = {0};
        fread(&entry, sizeof(struct sfo_index_table_entry), 1, file);

        long old_position = ftell(file);

        // byteswap since file is LE and we're on BE
        entry.key_offset = _ES16(entry.key_offset);
        entry.data_fmt = _ES16(entry.data_fmt);
        entry.data_len = _ES32(entry.data_len);
        entry.data_max_len = _ES32(entry.data_max_len);
        entry.data_offset = _ES32(entry.data_offset);

        // Read key
        char key[256] = {0};
        fseek(file, header.key_table_start + entry.key_offset, SEEK_SET);
        fread(key, 256, 1, file);

        // Read value
        char value[256] = {0};
        fseek(file, header.data_table_start + entry.data_offset, SEEK_SET);
        fread(value, 256, 1, file);

        // SDL_Log("%s: %s", key, value);

        if (strcmp(key, "TITLE") == 0)
        {
            fclose(file);
            return strdup(value);
        }

        fseek(file, old_position, SEEK_SET);
    }

    SDL_Log("Unable to find TITLE");

    fclose(file);
    return NULL;
}