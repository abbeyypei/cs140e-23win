// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
    "ttyACM",   // linux
	"cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    // if (d->d_name == NULL) return 0;
    for (int i = 0; i < 3; i++) {
        // output("comparing %s and %s\n", ttyusb_prefixes[i], d->d_name);
        if (strncmp(ttyusb_prefixes[i], d->d_name, strlen(ttyusb_prefixes[i])) == 0) return 1;
    }
    return 0;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    struct dirent **namelist;
    int n;

    n = scandir("/dev", &namelist, filter, alphasort);
    if (n < 0) fatal("scandir error");
    // if (n == 0 || n > 1) panic("expected 1 device, got %d", n);
    char *name = malloc((strlen("/dev/") + strlen(namelist[0]->d_name) + 1) *sizeof(char *));
    char *dir = "/dev/";
    strncpy(name, dir, strlen(dir));
    strcat(name, namelist[0]->d_name);
    return name;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    // return find_ttyusb();

    struct dirent **namelist;
    int n;

    n = scandir("/dev", &namelist, filter, alphasort);
    if (n < 0) fatal("scandir error");
    struct stat sb;
    char *dir = "/dev/";
    char *fname;

    struct timespec *last_modified;
    struct timespec *mtime;
    int minn = 0;
    for (int i = 0; i < n; i++) {
        // output("ith: %s\n", namelist[i]->d_name);
        fname = strdupf("/dev/%s", namelist[i]->d_name);
        if (stat(fname, &sb) < 0) fatal("stat error");
        // printf("Last file modification:   %s", ctime(&sb.st_mtime));
        mtime = (struct timespec *) (&sb.st_mtime);

        if (i==0) {
            last_modified = mtime;
            minn = i;
        }
        if (last_modified->tv_sec > mtime->tv_sec) {
            last_modified = mtime;
            minn = i;
        } else {
            if (last_modified->tv_sec == mtime->tv_sec && last_modified->tv_nsec > mtime->tv_nsec) {
                last_modified = mtime;
                minn = i;
            }
        }
        
    }

    char *name = malloc((strlen("/dev/") + strlen(namelist[minn]->d_name) + 1) *sizeof(char *));
    strncpy(name, dir, strlen(dir));
    strcat(name, namelist[minn]->d_name);
    if (stat(name, &sb) < 0) fatal("stat error");
    return name;
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    struct dirent **namelist;
    int n;

    n = scandir("/dev", &namelist, filter, alphasort);
    if (n < 0) fatal("scandir error");
    struct stat sb;
    char *dir = "/dev/";
    char *fname;

    struct timespec *last_modified;
    struct timespec *mtime;
    int maxn = 0;
    for (int i = 0; i < n; i++) {
        // output("ith: %s\n", namelist[i]->d_name);
        fname = strdupf("/dev/%s", namelist[i]->d_name);
        if (stat(fname, &sb) < 0) fatal("stat error");
        // printf("Last file modification:   %s", ctime(&sb.st_mtime));
        mtime = (struct timespec *) (&sb.st_mtime);

        if (i==0) {
            last_modified = mtime;
            maxn = i;
        }
        if (last_modified->tv_sec < mtime->tv_sec) {
            last_modified = mtime;
            maxn = i;
        } else {
            if (last_modified->tv_sec == mtime->tv_sec && last_modified->tv_nsec < mtime->tv_nsec) {
                last_modified = mtime;
                maxn = i;
            }
        }
        
    }

    char *name = malloc((strlen("/dev/") + strlen(namelist[maxn]->d_name) + 1) *sizeof(char *));
    strncpy(name, dir, strlen(dir));
    strcat(name, namelist[maxn]->d_name);
    if (stat(name, &sb) < 0) fatal("stat error");
    return name;
}
