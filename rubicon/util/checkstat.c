/*************************************************************************
 *   checkstat.c		* <comment>
 *************************************************************************
 *
 * Description:
 *   Return stat informatoin on a file
 *
 *
 *************************************************************************
 * Copyright (c) 2000 The Pragmatic Programmers, LLC
 *************************************************************************/

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
  
  if (argc) {
    struct stat s;

    if (stat(argv[1], &s) != 0)
      exit(errno);

    printf("%d %d %d %d %d %d %d %d %d ",
           s.st_dev, s.st_ino, s.st_mode, s.st_nlink,
           s.st_uid,  s.st_gid, s.st_rdev, s.st_size, s.st_blksize);
    printf("%d %d %d %d\n",
           s.st_blocks, s.st_atime, s.st_mtime, s.st_ctime);
  }
  else
    return -1;

  return 0;
}
