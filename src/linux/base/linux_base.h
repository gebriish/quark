#ifndef LINUX_BASE_H
#define LINUX_BASE_H

////////////////////////////////////////////
// @section: linux system includes

#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

////////////////////////////////////////////
// @section: internal lnx base types


////////////////////////////////////////////
// @section: internal lnx helpers

function void lnx_file_properties_from_stat(struct stat st, FileProperty *props);

#endif // LINUX_BASE_H
