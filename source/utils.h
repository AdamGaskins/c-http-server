#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum entry_type { ENTRY_DIR,
    ENTRY_FILE };

struct directory {
    uint32_t entry_count;
    struct directory_entry* entries;
};

struct directory_entry {
    enum entry_type type;
    char* path;
    char* name;
};

struct directory* directory_listing(char* path);
void directory_listing_free(struct directory *);
