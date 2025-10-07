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
    int show_all = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            show_all = 1;
        } else {
            dirpath = argv[i];
        }
    }

    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s': %s\n", dirpath, strerror(errno));
        return 2;
    }

    size_t cap = 64;
    size_t n = 0;
    char **names = malloc(cap * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return 2;
    }

    size_t maxlen = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip "." and ".." unless -a
        if (!show_all && (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0))
            continue;

        // Skip hidden files if not showing all
        if (!show_all && ent->d_name[0] == '.')
            continue;

        if (n + 1 > cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                for (size_t i = 0; i < n; i++) free(names[i]);
                free(names);
                closedir(dir);
                return 2;
            }
            names = tmp;
        }

        names[n] = strdup(ent->d_name);
        if (!names[n]) {
            perror("strdup");
            for (size_t i = 0; i < n; i++) free(names[i]);
            free(names);
            closedir(dir);
            return 2;
        }

        size_t len = strlen(names[n]);
        if (len > maxlen) maxlen = len;
        n++;
    }
    closedir(dir);

    if (n == 0) {
        free(names);
        return 0;
    }

    // Sort lexicographically
    qsort(names, n, sizeof(char *), cmpstr);

    // Detect terminal width
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        term_width = ws.ws_col;
    }

    int spacing = 2;
    int col_width = (int)maxlen + spacing;
    if (col_width <= 0) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    if (cols > (int)n) cols = (int)n;

    int rows = (int)((n + cols - 1) / cols);

    // Display files "down then across"
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx >= (int)n) continue;
            const char *name = names[idx];
            printf("%s", name);

            if (c != cols - 1) {
                int pad = col_width - (int)strlen(name);
                if (pad < 1) pad = 1;
                for (int p = 0; p < pad; ++p) putchar(' ');
            }
        }
        putchar('\n');
    }

    for (size_t i = 0; i < n; ++i) free(names[i]);
    free(names);
    return 0;
}
