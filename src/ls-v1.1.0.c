#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

/* Print a permission string (like "drwxr-xr-x") followed by a space */
static void print_permissions(mode_t mode) {
    char perms[11];
    perms[0] = S_ISDIR(mode) ? 'd' :
               S_ISLNK(mode) ? 'l' :
               S_ISCHR(mode) ? 'c' :
               S_ISBLK(mode) ? 'b' :
               S_ISSOCK(mode) ? 's' :
               S_ISFIFO(mode) ? 'p' : '-';

    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? ((mode & S_ISUID) ? 's' : 'x') : ((mode & S_ISUID) ? 'S' : '-');
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? ((mode & S_ISGID) ? 's' : 'x') : ((mode & S_ISGID) ? 'S' : '-');
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? ((mode & S_ISVTX) ? 't' : 'x') : ((mode & S_ISVTX) ? 'T' : '-');
    perms[10] = '\0';

    printf("%s ", perms);
}

/* Print a long listing for each (non-hidden) entry in path */
static void display_long_listing(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "opendir '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Skip hidden files (behaviour like plain ls without -a) */
        if (entry->d_name[0] == '.')
            continue;

        char fullpath[PATH_MAX];
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name) >= (int)sizeof(fullpath)) {
            fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
            continue;
        }

        struct stat st;
        if (lstat(fullpath, &st) == -1) {
            fprintf(stderr, "lstat '%s': %s\n", fullpath, strerror(errno));
            continue;
        }

        /* permissions */
        print_permissions(st.st_mode);

        /* number of links */
        printf("%2lu ", (unsigned long)st.st_nlink);

        /* user and group: fallback to uid/gid if name not found */
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        if (pw)
            printf("%-8s ", pw->pw_name);
        else
            printf("%-8lu ", (unsigned long)st.st_uid);

        if (gr)
            printf("%-8s ", gr->gr_name);
        else
            printf("%-8lu ", (unsigned long)st.st_gid);

        /* size */
        printf("%8lu ", (unsigned long)st.st_size);

        /* time: format "Mon dd HH:MM" (same as ls for recent files) */
        char timebuf[64];
        struct tm *mt = localtime(&st.st_mtime);
        if (mt)
            strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", mt);
        else
            snprintf(timebuf, sizeof(timebuf), "??? ?? ??:??");
        printf("%s ", timebuf);

        /* name */
        printf("%s", entry->d_name);

        /* if symlink, print "-> target" */
        if (S_ISLNK(st.st_mode)) {
            char linktarget[PATH_MAX + 1];
            ssize_t r = readlink(fullpath, linktarget, sizeof(linktarget) - 1);
            if (r >= 0) {
                linktarget[r] = '\0';
                printf(" -> %s", linktarget);
            }
        }

        printf("\n");
    }

    closedir(dir);
}

/* Simple non -l listing (prints names in one line) */
static void display_simple(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "opendir '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    int first = 1;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (!first) printf("  ");
        printf("%s", entry->d_name);
        first = 0;
    }
    printf("\n");
    closedir(dir);
}

int main(int argc, char *argv[]) {
    int opt;
    int long_format = 0;

    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    if (long_format)
        display_long_listing(path);
    else
        display_simple(path);

    return EXIT_SUCCESS;
}
