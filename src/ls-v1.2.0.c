// src/ls-v1.2.0.c
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>

static int cmpstr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

int main(int argc, char *argv[]) {
    const char *dirpath = ".";
    if (argc > 1) dirpath = argv[1];

    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s': %s\n", dirpath, strerror(errno));
        return 2;
    }

    size_t cap = 64;
    size_t n = 0;
    char **names = malloc(cap * sizeof(char *));
    if (!names) { perror("malloc"); closedir(dir); return 2; }

    size_t maxlen = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip "." and ".." optionally? Standard ls shows them only with -a, so skip here:
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        if (n + 1 > cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (!tmp) { perror("realloc"); /* free previously allocated */ for (size_t i=0;i<n;i++) free(names[i]); free(names); closedir(dir); return 2; }
            names = tmp;
        }
        names[n] = strdup(ent->d_name);
        if (!names[n]) { perror("strdup"); for (size_t i=0;i<n;i++) free(names[i]); free(names); closedir(dir); return 2; }
        size_t len = strlen(names[n]);
        if (len > maxlen) maxlen = len;
        n++;
    }
    closedir(dir);

    if (n == 0) {
        // nothing to print
        free(names);
        return 0;
    }

    // Sort names (lexicographic)
    qsort(names, n, sizeof(char *), cmpstr);

    // Get terminal width via ioctl
    struct winsize ws;
    int term_width = 80; // fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        term_width = ws.ws_col;
    }

    int spacing = 2; // spaces between columns
    int col_width = (int)maxlen + spacing;
    if (col_width <= 0) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1; // at least one column
    if (cols > (int)n) cols = (int)n; // no more columns than items

    // rows = ceil(n / cols)
    int rows = (int)((n + cols - 1) / cols);

    // Print rows; for each row print element from each column
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx >= (int)n) continue;
            const char *name = names[idx];
            printf("%s", name);

            // pad, but don't pad after last column
            if (c != cols - 1) {
                int pad = col_width - (int)strlen(name);
                if (pad < 1) pad = 1;
                for (int p = 0; p < pad; ++p) putchar(' ');
            }
        }
        putchar('\n');
    }

    // free memory
    for (size_t i = 0; i < n; ++i) free(names[i]);
    free(names);
    return 0;
}
