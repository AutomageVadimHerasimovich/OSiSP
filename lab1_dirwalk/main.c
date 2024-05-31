#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>

typedef struct parameters_s
{
    char *path;
    bool dirs;
    bool files;
    bool symlinks;
    bool sort;
} parameters_t;

typedef enum enum_entry_type
{
    Dir,
    File,
    Symlink
} enum_entry_type;

typedef struct entry_s
{
    char *path;
    int size;
    enum_entry_type type;
} entry_t;

void traverse(char *current_dir, parameters_t params, entry_t **entries, int *num_entries)
{
    struct dirent *entry = NULL;
    char pathName[PATH_MAX + 1];
    DIR *dir = opendir(current_dir);
    if (dir == NULL)
    {
        printf("Error opening %s: %s\n", current_dir, strerror(errno));
        return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        struct stat entryInfo;
        if ((strncmp(entry->d_name, ".", PATH_MAX) == 0) || (strncmp(entry->d_name, "..", PATH_MAX) == 0))
            continue;
        (void)strncpy(pathName, current_dir, PATH_MAX);
        (void)strncat(pathName, "/", PATH_MAX);
        (void)strncat(pathName, entry->d_name, PATH_MAX);
        if (lstat(pathName, &entryInfo) == -1)
            printf("Error stating %s: %s\n", pathName, strerror(errno));
        if ((params.dirs && S_ISDIR(entryInfo.st_mode)) || (params.files && S_ISREG(entryInfo.st_mode)) || (params.symlinks && S_ISLNK(entryInfo.st_mode)))
        {
            if (*num_entries == 0)
                *entries = malloc(sizeof(entry_t));
            else
                *entries = realloc(*entries, sizeof(entry_t) * (*num_entries + 1));
            enum_entry_type type;
            if (S_ISDIR(entryInfo.st_mode))
                type = Dir;
            if (S_ISREG(entryInfo.st_mode))
                type = File;
            if (S_ISLNK(entryInfo.st_mode))
                type = Symlink;
            entry_t new_entry = {malloc(strlen(pathName) + 1), entryInfo.st_size, type};
            strcpy(new_entry.path, pathName);
            (*entries)[(*num_entries)++] = new_entry;
        }
        if (S_ISDIR(entryInfo.st_mode))
            traverse(pathName, params, entries, num_entries);
    }
    (void)closedir(dir);
}

char *entry_desc(enum_entry_type type)
{
    switch (type)
    {
    case Dir:
        return "-d";
    case File:
        return "-f";
    case Symlink:
        return "-l";
    default:
        printf("Unexpected entry type\n");
        exit(1);
    }
}

int compare_entries(const void *a, const void *b)
{
    return strcoll(((entry_t *)a)->path, ((entry_t *)b)->path);
}

int main(int argc, char *argv[])
{
    int opt;
    parameters_t params = {0}; // zero-initialize
    params.path = ".";
    while ((opt = getopt(argc, argv, "ldfs")) != EOF)
    {
        switch (opt)
        {
        case 'l':
            params.symlinks = true;
            break;
        case 'd':
            params.dirs = true;
            break;
        case 'f':
            params.files = true;
            break;
        case 's':
            params.sort = true;
            break;
        case '?':
            printf("unknown option : %c\n", optopt);
            break;
        }
    }
    if (!params.dirs && !params.files && !params.symlinks)
    {
        params.dirs = true;
        params.files = true;
        params.symlinks = true;
    }
    if (optind < argc)
        params.path = argv[optind];
    entry_t *entries = NULL;
    int num_entries = 0;
    traverse(params.path, params, &entries, &num_entries);
    if (params.sort)
        qsort(entries, num_entries, sizeof(entry_t), compare_entries);
    for (int i = 0; i < num_entries; ++i)
    {
        if (entries[i].type == Dir)
            printf("\t%s %s\n", entry_desc(entries[i].type), entries[i].path);
        else
            printf("\t%s %s (%d bytes) \n", entry_desc(entries[i].type), entries[i].path, entries[i].size);
        free(entries[i].path);
    }
    free(entries);
    return 0;
}
