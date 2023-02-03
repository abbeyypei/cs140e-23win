#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // How: 
    //    - use stat to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer.  
    //    - make sure any padding bytes have zeros.
    //    - return it.   
    struct stat sb;
    if (stat(name, &sb) < 0) fatal("stat error");
    unsigned nbytes = (unsigned) sb.st_size;
    *size = nbytes;

    int fd = open(name, O_RDONLY);
    if (fd == -1) fatal("open error");

    unsigned buf_size = (nbytes / 4) * 4 + (4 - nbytes % 4);
    void *buf = calloc(4+buf_size, sizeof(char));
    if (buf == NULL) fatal("calloc error");
    
    unsigned bytes_read = 0;
    unsigned stat;
    while (bytes_read < nbytes) {
        stat = read(fd, buf, nbytes);
        if (stat < 0) fatal("read error");
        bytes_read += stat;
    }
    close(fd);
    return buf;
}
