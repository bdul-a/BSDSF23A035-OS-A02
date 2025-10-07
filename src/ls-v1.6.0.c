/*
 * ls-v1.6.0.c
 * Feature-7: Recursive Listing (-R Option)
 * - builds on v1.5.0 colorized output
 * - supports recursion into subdirectories when -R flag is given
 * - constructs full paths safely and skips . and ..
 *
 * Build:
 *   mkdir -p bin
 *   gcc -std=c11 -Wall -Wextra -O2 -o bin/ls-v1.6.0 src/ls-v1.6.0.c
 *
 * Test:
 *   ./bin/ls-v1.6.0 -R [path]
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>

#define ANSI_RESET    "\033[0m"
#define ANSI_BLUE     "\033[0;34m"
#define ANSI_GREEN    "\033[0;32m"
#define ANSI_RED      "\033[0;31m"
#define ANSI_PINK     "\033[0;35m"
#define ANSI_REVERSE  "\033[7m"

#define COL_SPACING 2

static int cmpstr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcasecmp(sa, sb);
}

static bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t lenstr = strlen(str);
    size_t lensuf = strlen(suffix);
    if (lensuf > lenstr) return false;
    return strcmp(str + lenstr - lensuf, suffix) == 0;
}

static bool is_archive_name(const char *name) {
    if (!name) return false;
    char lower[NAME_MAX + 1];
    size_t n = strlen(name);
    if (n > NAME_MAX) n = NAME_MAX;
    for (size_t i = 0; i < n; ++i) lower[i] = (char)tolower((unsigned char)name[i]);
    lower[n] = '\0';
    if (ends_with(lower, ".tar") || ends_with(lower, ".tar.gz") ||
        ends_with(lower, ".tgz") || ends_with(lower, ".gz") ||
        ends_with(lower, ".zip")) return true;
    return false;
}

static void print_name_colored(const char *dirpath, const char *name, bool color_enabled, int colwidth) {
    char fullpath[PATH_MAX];
    struct stat st;

    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, name);
    if (lstat(fullpath, &st) == -1) {
        printf("%s", name);
    } else {
        const char *start = NULL, *end = ANSI_RESET;
        if (!color_enabled) {
            start = end = NULL;
        } else if (S_ISLNK(st.st_mode)) start = ANSI_PINK;
        else if (S_ISDIR(st.st_mode)) start = ANSI_BLUE;
        else if (is_archive_name(name)) start = ANSI_RED;
        else if (S_ISREG(st.st_mode) && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) start = ANSI_GREEN;
        else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
                 S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode)) start = ANSI_REVERSE;
        if (start) printf("%s%s%s", start, name, end);
        else printf("%s", name);
    }

    int pad = colwidth - (int)strlen(name) + COL_SPACING;
    if (pad < 0) pad = COL_SPACING;
    for (int i = 0; i < pad; ++i) putchar(' ');
}

static void do_ls(const char *dirpath, bool recursive_flag, bool color_enabled) {
    DIR *d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "error: cannot open '%s': %s\n", dirpath, strerror(errno));
        return;
    }

    printf("%s:\n", dirpath);

    struct dirent *entry;
    size_t capacity = 128, count = 0;
    char **names = malloc(capacity * sizeof(char*));
    if (!names) { perror("malloc"); closedir(d); return; }

    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char*));
            if (!tmp) { perror("realloc"); break; }
            names = tmp;
        }
        names[count] = strdup(entry->d_name);
        if (!names[count]) { perror("strdup"); break; }
        count++;
    }
    closedir(d);

    if (count == 0) { free(names); return; }

    qsort(names, count, sizeof(char*), cmpstr);

    int maxlen = 0;
    for (size_t i = 0; i < count; ++i) {
        int l = (int)strlen(names[i]);
        if (l > maxlen) maxlen = l;
    }

    int term_width = 80;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) term_width = ws.ws_col;

    int total_col_w = maxlen + COL_SPACING;
    int cols = term_width / total_col_w;
    if (cols < 1) cols = 1;
    if (cols > (int)count) cols = (int)count;
    int rows = (int)((count + cols - 1) / cols);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx >= (int)count) continue;
            print_name_colored(dirpath, names[idx], color_enabled, maxlen);
        }
        putchar('\n');
    }

    if (recursive_flag) {
        for (size_t i = 0; i < count; ++i) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, names[i]);
            struct stat st;
            if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                printf("\n");
                do_ls(fullpath, true, color_enabled);
            }
        }
    }

    for (size_t i = 0; i < count; ++i) free(names[i]);
    free(names);
}

int main(int argc, char **argv) {
    int opt;
    bool recursive_flag = false;
    const char *dirpath = ".";
    while ((opt = getopt(argc, argv, "R")) != -1) {
        if (opt == 'R') recursive_flag = true;
        else {
            fprintf(stderr, "Usage: %s [-R] [directory]\n", argv[0]);
            return 1;
        }
    }
    if (optind < argc) dirpath = argv[optind];

    bool color_enabled = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;

    do_ls(dirpath, recursive_flag, color_enabled);
    return 0;
}
