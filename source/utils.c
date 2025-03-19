#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dirent.h>

struct directory* directory_listing(char* path)
{
    struct directory* directory = calloc(1, sizeof(*directory));
    directory->entry_count = 0;
    directory->entries = calloc(256, sizeof(*directory->entries));

    DIR* dp = opendir(path);
    if (dp == NULL) {
        goto cleanup;
    }

    struct dirent* ent;
    while ((ent = readdir(dp)) != NULL) {
        if (ent->d_type != DT_REG && ent->d_type != DT_DIR) {
            continue;
        }

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char* fullpath = calloc(256, sizeof(*fullpath));
        strcat(fullpath, path);
        strcat(fullpath, ent->d_name);

        char* filename = calloc(256, sizeof(*filename));
        strcat(filename, ent->d_name);

        if (ent->d_type == DT_DIR) {
            strcat(fullpath, "/");
            strcat(filename, "/");
        }

        directory->entries[directory->entry_count++] = (struct directory_entry) {
            .path = fullpath,
            .name = filename,
            .type = ent->d_type == DT_REG ? ENTRY_FILE : ENTRY_DIR
        };
    }
    closedir(dp);

cleanup:

    return directory;
}

void directory_listing_free(struct directory* dir)
{
    for (uint32_t i = 0; i < dir->entry_count; i++) {
        free(dir->entries[i].path);
    }
    free(dir->entries);
    free(dir);
}
