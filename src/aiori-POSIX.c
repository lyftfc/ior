/******************************************************************************\
*                                                                              *
*        Copyright (c) 2003, The Regents of the University of California       *
*      See the file COPYRIGHT for a complete copyright notice and license.     *
*                                                                              *
********************************************************************************
*
* Implement of abstract I/O interface for POSIX.
*
\******************************************************************************/

#include "aiori.h"                                  /* abstract IOR interface */
#ifdef __linux__
#  include <sys/ioctl.h>                            /* necessary for: */
#  define __USE_GNU                                 /* O_DIRECT and */
#  include <fcntl.h>                                /* IO operations */
#  undef __USE_GNU
#endif /* __linux__ */
#include <errno.h>                                  /* sys_errlist */
#include <fcntl.h>                                  /* IO operations */
#include <stdio.h>                                  /* only for fprintf() */
#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_LUSTRE_LUSTRE_USER_H
#  include <lustre/lustre_user.h>
#endif /* HAVE_LUSTRE_LUSTRE_USER_H */

#ifndef   open64                                    /* necessary for TRU64 -- */
#  define open64  open                              /* unlikely, but may pose */
#endif /* not open64 */                             /* conflicting prototypes */

#ifndef   lseek64                                   /* necessary for TRU64 -- */
#  define lseek64 lseek                             /* unlikely, but may pose */
#endif /* not lseek64 */                            /* conflicting prototypes */

#ifndef   O_BINARY                                  /* Required on Windows    */
#  define O_BINARY 0
#endif


/**************************** P R O T O T Y P E S *****************************/
void *       IOR_Create_POSIX      (char *, IOR_param_t *);
void *       IOR_Open_POSIX        (char *, IOR_param_t *);
IOR_offset_t IOR_Xfer_POSIX        (int, void *, IOR_size_t *,
                                    IOR_offset_t, IOR_param_t *);
void         IOR_Close_POSIX       (void *, IOR_param_t *);
void         IOR_Delete_POSIX      (char *, IOR_param_t *);
void         IOR_SetVersion_POSIX  (IOR_param_t *);
void         IOR_Fsync_POSIX       (void *, IOR_param_t *);
IOR_offset_t IOR_GetFileSize_POSIX (IOR_param_t *, MPI_Comm, char *);

/************************** D E C L A R A T I O N S ***************************/

ior_aiori_t posix_aiori = {
    "POSIX",
    IOR_Create_POSIX,
    IOR_Open_POSIX,
    IOR_Xfer_POSIX,
    IOR_Close_POSIX,
    IOR_Delete_POSIX,
    IOR_SetVersion_POSIX,
    IOR_Fsync_POSIX,
    IOR_GetFileSize_POSIX
};

extern int      errno;
extern int      rank;
extern int      rankOffset;
extern int      verbose;
extern MPI_Comm testComm;

/***************************** F U N C T I O N S ******************************/

/******************************************************************************/
/*
 * Creat and open a file through the POSIX interface.
 */

void *
IOR_Create_POSIX(char        * testFileName,
                 IOR_param_t * param)
{
    int         fd_oflag = O_BINARY;
    int        *fd;

    fd = (int *)malloc(sizeof(int));
    if (fd == NULL) ERR("Unable to malloc file descriptor");

    if (param->useO_DIRECT == TRUE) {
/* note that TRU64 needs O_DIRECTIO, SunOS uses directio(),
   and everyone else needs O_DIRECT */
#ifndef O_DIRECT
#  ifndef O_DIRECTIO
     WARN("cannot use O_DIRECT");
#    define O_DIRECT 000000
#  else /* O_DIRECTIO */
#    define O_DIRECT O_DIRECTIO
#  endif /* not O_DIRECTIO */
#endif /* not O_DIRECT */
        fd_oflag |= O_DIRECT;
    }

#ifdef HAVE_LUSTRE_LUSTRE_USER_H
    if (param->lustre_set_striping) {
        /* In the single-shared-file case, task 0 has to creat the
           file with the Lustre striping options before any other processes
           open the file */
        if (!param->filePerProc && rank != 0) {
            MPI_CHECK(MPI_Barrier(testComm), "barrier error");
            fd_oflag |= O_RDWR;
            *fd = open64(testFileName, fd_oflag, 0664);
            if (*fd < 0) ERR("cannot open file");
        } else {
            struct lov_user_md opts = { 0 };

            /* Setup Lustre IOCTL striping pattern structure */
            opts.lmm_magic = LOV_USER_MAGIC;
            opts.lmm_stripe_size = param->lustre_stripe_size;
            opts.lmm_stripe_offset = param->lustre_start_ost;
            opts.lmm_stripe_count = param->lustre_stripe_count;

            /* File needs to be opened O_EXCL because we cannot set
               Lustre striping information on a pre-existing file. */
            fd_oflag |= O_CREAT | O_EXCL | O_RDWR | O_LOV_DELAY_CREATE;
            *fd = open64(testFileName, fd_oflag, 0664);
            if (*fd < 0) {
                fprintf(stdout, "\nUnable to open '%s': %s\n",
                        testFileName, strerror(errno));
                MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, -1), "MPI_Abort() error");
            } else if (ioctl(*fd, LL_IOC_LOV_SETSTRIPE, &opts)) {
                char *errmsg = "stripe already set";
                if (errno != EEXIST && errno != EALREADY)
                    errmsg = strerror(errno);
                fprintf(stdout, "\nError on ioctl for '%s' (%d): %s\n",
                        testFileName, *fd, errmsg);
                MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, -1), "MPI_Abort() error");
            }
            if (!param->filePerProc)
                MPI_CHECK(MPI_Barrier(testComm), "barrier error");
        }
    } else {
#endif /* HAVE_LUSTRE_LUSTRE_USER_H */
        fd_oflag |= O_CREAT | O_RDWR;
        *fd = open64(testFileName, fd_oflag, 0664);
        if (*fd < 0) ERR("cannot open file");
#ifdef HAVE_LUSTRE_LUSTRE_USER_H
    }
    
    if (param->lustre_ignore_locks) {
        int lustre_ioctl_flags = LL_FILE_IGNORE_LOCK;
        if (ioctl(*fd, LL_IOC_SETFLAGS, &lustre_ioctl_flags) == -1)
            ERR("cannot set ioctl");
    }
#endif /* HAVE_LUSTRE_LUSTRE_USER_H */

    return((void *)fd);
} /* IOR_Create_POSIX() */


/******************************************************************************/
/*
 * Open a file through the POSIX interface.
 */

void *
IOR_Open_POSIX(char        * testFileName,
               IOR_param_t * param)
{
    int         fd_oflag = O_BINARY;
    int        *fd;

    fd = (int *)malloc(sizeof(int));
    if (fd == NULL) ERR("Unable to malloc file descriptor");

    if (param->useO_DIRECT == TRUE) {
/* note that TRU64 needs O_DIRECTIO, SunOS uses directio(),
   and everyone else needs O_DIRECT */
#ifndef O_DIRECT
#  ifndef O_DIRECTIO
     WARN("cannot use O_DIRECT");
#    define O_DIRECT 000000
#  else /* O_DIRECTIO */
#    define O_DIRECT O_DIRECTIO
#  endif /* not O_DIRECTIO */
#endif /* not O_DIRECT */
        fd_oflag |= O_DIRECT;
    }

    fd_oflag |= O_RDWR;
    *fd = open64(testFileName, fd_oflag);
    if (*fd < 0) ERR("cannot open file");

#ifdef HAVE_LUSTRE_LUSTRE_USER_H
    if (param->lustre_ignore_locks) {
        int lustre_ioctl_flags = LL_FILE_IGNORE_LOCK;
        if (verbose >= VERBOSE_1) {
            fprintf(stdout, "** Disabling lustre range locking **\n");
        }
        if (ioctl(*fd, LL_IOC_SETFLAGS, &lustre_ioctl_flags) == -1)
            ERR("cannot set ioctl");
    }
#endif /* HAVE_LUSTRE_LUSTRE_USER_H */

    return((void *)fd);
} /* IOR_Open_POSIX() */


/******************************************************************************/
/*
 * Write or read access to file using the POSIX interface.
 */

IOR_offset_t
IOR_Xfer_POSIX(int            access,
               void         * file,
               IOR_size_t   * buffer,
               IOR_offset_t   length,
               IOR_param_t  * param)
{
    int            xferRetries = 0;
    long long      remaining  = (long long)length;
    char         * ptr = (char *)buffer;
    long long      rc;
    int            fd;

    fd = *(int *)file;

    /* seek to offset */
    if (lseek64(fd, param->offset, SEEK_SET) == -1)
        ERR("seek failed");

    while (remaining > 0) {
        /* write/read file */
        if (access == WRITE) { /* WRITE */
            if (verbose >= VERBOSE_4) {
                fprintf(stdout, "task %d writing to offset %lld\n",
                        rank, param->offset + length - remaining);
            }
            rc = write(fd, ptr, remaining);
            if (param->fsyncPerWrite == TRUE) IOR_Fsync_POSIX(&fd, param);
        } else {               /* READ or CHECK */
            if (verbose >= VERBOSE_4) {
                fprintf(stdout, "task %d reading from offset %lld\n",
                        rank, param->offset + length - remaining);
            }
            rc = read(fd, ptr, remaining);
            if (rc == 0)
                ERR("hit EOF prematurely");
        }
        if (rc == -1)
            ERR("transfer failed");
        if (rc != remaining) {
            fprintf(stdout,
                    "WARNING: Task %d requested transfer of %lld bytes,\n",
                    rank, remaining);
            fprintf(stdout,
                    "         but transferred %lld bytes at offset %lld\n",
                    rc, param->offset + length - remaining);
            if (param->singleXferAttempt == TRUE)
                MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, -1), "barrier error");
        }
        if (rc < remaining) {
            if (xferRetries > MAX_RETRY)
                ERR("too many retries -- aborting");
            if (xferRetries == 0) {
                if (access == WRITE) {
                  WARN("This file system requires support of partial write()s");
                } else {
                  WARN("This file system requires support of partial read()s");
                }
                fprintf(stdout,
              "WARNING: Requested xfer of %lld bytes, but xferred %lld bytes\n",
                        remaining, rc);
            }
            if (verbose >= VERBOSE_2) {
                fprintf(stdout, "Only transferred %lld of %lld bytes\n",
                        rc, remaining);
            }
        }
        if (rc > remaining) /* this should never happen */
            ERR("too many bytes transferred!?!");
        remaining -= rc;
        ptr += rc;
        xferRetries++;
    }
    return(length);
} /* IOR_Xfer_POSIX() */


/******************************************************************************/
/*
 * Perform fsync().
 */

void
IOR_Fsync_POSIX(void * fd, IOR_param_t * param)
{
    if (fsync(*(int *)fd) != 0) WARN("cannot perform fsync on file");
} /* IOR_Fsync_POSIX() */


/******************************************************************************/
/*
 * Close a file through the POSIX interface.
 */

void
IOR_Close_POSIX(void *fd,
                IOR_param_t * param)
{
    if (close(*(int *)fd) != 0) ERR("cannot close file");
    free(fd);
} /* IOR_Close_POSIX() */


/******************************************************************************/
/*
 * Delete a file through the POSIX interface.
 */

void
IOR_Delete_POSIX(char * testFileName, IOR_param_t * param)
{
    char errmsg[256];
    sprintf(errmsg,"[RANK %03d]:cannot delete file %s\n",rank,testFileName);
    if (unlink(testFileName) != 0) WARN(errmsg);
} /* IOR_Delete_POSIX() */


/******************************************************************************/
/*
 * Determine api version.
 */

void
IOR_SetVersion_POSIX(IOR_param_t *test)
{
    strcpy(test->apiVersion, test->api);
} /* IOR_SetVersion_POSIX() */


/******************************************************************************/
/*
 * Use POSIX stat() to return aggregate file size.
 */

IOR_offset_t
IOR_GetFileSize_POSIX(IOR_param_t * test,
                      MPI_Comm      testComm,
                      char        * testFileName)
{
    struct stat stat_buf;
    IOR_offset_t aggFileSizeFromStat,
                 tmpMin, tmpMax, tmpSum;

    if (stat(testFileName, &stat_buf) != 0) {
        ERR("cannot get status of written file");
    }
    aggFileSizeFromStat = stat_buf.st_size;

    if (test->filePerProc == TRUE) {
        MPI_CHECK(MPI_Allreduce(&aggFileSizeFromStat, &tmpSum, 1,
                                MPI_LONG_LONG_INT, MPI_SUM, testComm),
                  "cannot total data moved");
        aggFileSizeFromStat = tmpSum;
    } else {
        MPI_CHECK(MPI_Allreduce(&aggFileSizeFromStat, &tmpMin, 1,
                                MPI_LONG_LONG_INT, MPI_MIN, testComm),
                  "cannot total data moved");
        MPI_CHECK(MPI_Allreduce(&aggFileSizeFromStat, &tmpMax, 1,
                  MPI_LONG_LONG_INT, MPI_MAX, testComm),
                  "cannot total data moved");
        if (tmpMin != tmpMax) {
            if (rank == 0) {
                WARN("inconsistent file size by different tasks");
            }
            /* incorrect, but now consistent across tasks */
            aggFileSizeFromStat = tmpMin;
        }
    }

    return(aggFileSizeFromStat);
} /* IOR_GetFileSize_POSIX() */