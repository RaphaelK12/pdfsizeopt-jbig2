/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/


/*
 *  utils.c
 *
 *       Error, warning and info procs; all invoked by macros
 *           l_int32    returnErrorInt()
 *           l_float32  returnErrorFloat()
 *           void      *returnErrorPtr()
 *           void       l_error()
 *           void       l_errorString()
 *           void       l_errorInt()
 *           void       l_errorFloat()
 *           void       l_warning()
 *           void       l_warningString()
 *           void       l_warningInt()
 *           void       l_warningInt2()
 *           void       l_warningFloat()
 *           void       l_warningFloat2()
 *           void       l_info()
 *           void       l_infoString()
 *           void       l_infoInt()
 *           void       l_infoInt2()
 *           void       l_infoFloat()
 *           void       l_infoFloat2()
 *
 *       Safe string procs
 *           char      *stringNew()
 *           l_int32    stringCopy()
 *           l_int32    stringReplace()
 *           l_int32    stringLength()
 *           l_int32    stringCat()
 *           char      *stringJoin()
 *           char      *stringReverse()
 *           char      *strtokSafe()
 *           l_int32    stringSplitOnToken()
 *
 *       Find and replace string and array procs
 *           char      *stringRemoveChars()
 *           l_int32    stringFindSubstr()
 *           char      *stringReplaceSubstr()
 *           char      *stringReplaceEachSubstr()
 *           NUMA      *arrayFindEachSequence()
 *           l_int32    arrayFindSequence()
 *
 *       Safe realloc
 *           void      *reallocNew()
 *
 *       Read and write between file and memory
 *           l_uint8   *l_binaryRead()
 *           l_uint8   *l_binaryReadStream()
 *           l_int32    l_binaryWrite()
 *           l_int32    nbytesInFile()
 *           l_int32    fnbytesInFile()
 *
 *       Copy in memory
 *           l_uint8   *l_binaryCopy()
 *
 *       File copy operations
 *           l_int32    fileCopy()
 *           l_int32    fileConcatenate()
 *           l_int32    fileAppendString()
 *
 *       Test files for equivalence
 *           l_int32    filesAreIdentical()
 *
 *       Byte-swapping data conversion
 *           l_uint16   convertOnBigEnd16()
 *           l_uint32   convertOnBigEnd32()
 *           l_uint16   convertOnLittleEnd16()
 *           l_uint32   convertOnLittleEnd32()
 *
 *       Opening file streams
 *           FILE      *fopenReadStream()
 *           FILE      *fopenWriteStream()
 *
 *       Functions to avoid C-runtime boundary crossing with Windows DLLs
 *           FILE      *lept_fopen()
 *           l_int32    lept_fclose()
 *           void       lept_calloc()
 *           void       lept_free()
 *
 *       Cross-platform file system operations
 *           l_int32    lept_mkdir()
 *           l_int32    lept_rmdir()
 *           l_int32    lept_mv()
 *           l_int32    lept_rm()
 *           l_int32    lept_cp()
 *
 *       File name operations
 *           l_int32    splitPathAtDirectory()
 *           l_int32    splitPathAtExtension()
 *           char      *pathJoin()
 *           char      *genPathname()
 *           char      *genTempFilename()
 *           l_int32    extractNumberFromFilename()
 *
 *       Generate random integer in given range
 *           l_int32    genRandomIntegerInRange()
 *
 *       Leptonica version number
 *           char      *getLeptonicaVersion()
 *
 *       Timing
 *           void       startTimer()
 *           l_float32  stopTimer()
 *           L_TIMER    startTimerNested()
 *           l_float32  stopTimerNested()
 *           void       l_getCurrentTime()
 *           void       l_getFormattedDate()
 *
 *       Deprecated binary read functions  (don't use these!)
 *           l_uint8   *arrayRead()
 *           l_uint8   *arrayReadStream()
 *
 *
 *  Notes on cross-platform development
 *  -----------------------------------
 *  (1) With the exception of splitPathAtDirectory() and
 *      splitPathAtExtension(), all input pathnames must have unix separators.
 *  (2) The conversion from unix to windows pathnames happens in genPathname().
 *  (3) Use fopenReadStream() and fopenWriteStream() to open files,
 *      because these use genPathname() to find the platform-dependent
 *      filenames.  Likewise for l_binaryRead() and l_binaryWrite().
 *  (4) For moving, copying and removing files and directories,
 *      use the lept_*() file system shell wrappers:
 *         lept_mkdir(), lept_rmdir(), lept_mv(), lept_rm() and lept_cp().
 *  (5) Use the lept_*() C library wrappers:
 *         lept_fopen(), lept_fclose(), lept_calloc() and lept_free().
 */

#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif   /* _MSC_VER */
#include "allheaders.h"

#ifdef _WIN32
#include <windows.h>
static const char sepchar = '\\';
#else
#include <sys/stat.h>  /* for mkdir(2) */
#include <sys/types.h>
static const char sepchar = '/';
#endif


/*----------------------------------------------------------------------*
 *                 Error, warning and info message procs                *
 *                                                                      *
 *            ---------------------  N.B. ---------------------         *
 *                                                                      *
 *    (1) These functions all print messages to stderr.                 *
 *                                                                      *
 *    (2) They must be invoked only by macros, which are in             *
 *        environ.h, so that the print output can be disabled           *
 *        at compile time, using -DNO_CONSOLE_IO.                       *
 *                                                                      *
 *----------------------------------------------------------------------*/
/*!
 *  returnErrorInt()
 *
 *      Input:  msg (error message)
 *              procname
 *              ival (return val)
 *      Return: ival (typically 1)
 */
LEPTONICA_EXPORT l_int32
returnErrorInt(const char  *msg, 
               const char  *procname, 
               l_int32      ival)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return ival;
}


/*!
 *  returnErrorPtr()
 *
 *      Input:  msg (error message)
 *              procname
 *              pval  (return val)
 *      Return: pval (typically null)
 */
LEPTONICA_EXPORT void *
returnErrorPtr(const char  *msg,
               const char  *procname, 
               void        *pval)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return pval;
}


/*!
 *  l_error()
 *
 *      Input: msg (error message)
 *             procname
 */
LEPTONICA_EXPORT void
l_error(const char  *msg,
        const char  *procname)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return;
}

/*!
 *  l_errorInt()
 *
 *      Input: msg (error message; must include '%d')
 *             procname
 *             ival (embedded in error message via %d)
 */
LEPTONICA_EXPORT void
l_errorInt(const char  *msg,
           const char  *procname,
           l_int32      ival)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_errorInt()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_errorInt()", procname);
        return;
    }

    sprintf(charbuf, "Error in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival);

    FREE(charbuf);
    return;
}

/*!
 *  l_warning()
 *
 *      Input: msg (warning message)
 *             procname
 */
LEPTONICA_EXPORT void
l_warning(const char  *msg,
          const char  *procname)
{
    fprintf(stderr, "Warning in %s: %s\n", procname, msg);
    return;
}

/*!
 *  l_warningInt()
 *
 *      Input: msg (warning message; must include '%d')
 *             procname
 *             ival (embedded in warning message via %d)
 */
LEPTONICA_EXPORT void
l_warningInt(const char  *msg,
             const char  *procname,
             l_int32      ival)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_warningInt()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningInt()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival);

    FREE(charbuf);
    return;
}

/*--------------------------------------------------------------------*
 *                       Safe string operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  stringNew()
 *
 *      Input:  src string
 *      Return: dest copy of src string, or null on error
 */
LEPTONICA_EXPORT char *
stringNew(const char  *src)
{
l_int32  len;
char    *dest;

    PROCNAME("stringNew");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    
    len = strlen(src);
    if ((dest = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);

    stringCopy(dest, src, len);
    return dest;
}


/*!
 *  stringCopy()
 *
 *      Input:  dest (existing byte buffer)
 *              src string (can be null)
 *              n (max number of characters to copy)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Relatively safe wrapper for strncpy, that checks the input,
 *          and does not complain if @src is null or @n < 1.
 *          If @n < 1, this is a no-op.
 *      (2) @dest needs to be at least @n bytes in size.
 *      (3) We don't call strncpy() because valgrind complains about
 *          use of uninitialized values.
 */
LEPTONICA_EXPORT l_int32
stringCopy(char        *dest,
           const char  *src,
           l_int32      n)
{
l_int32  i;

    PROCNAME("stringCopy");

    if (!dest)
        return ERROR_INT("dest not defined", procName, 1);
    if (!src || n < 1)
        return 0;

        /* Implementation of strncpy that valgrind doesn't complain about */
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return 0;
}
    

/*!
 *  stringReplace()
 *
 *      Input:  &dest string (<return> copy)
 *              src string
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Frees any existing dest string
 *      (2) Puts a copy of src string in the dest
 *      (3) If either or both strings are null, does something reasonable.
 */
LEPTONICA_EXPORT l_int32
stringReplace(char       **pdest,
              const char  *src)
{
char    *scopy;
l_int32  len;

    PROCNAME("stringReplace");

    if (!pdest)
        return ERROR_INT("pdest not defined", procName, 1);

    if (*pdest)
        FREE(*pdest);
    
    if (src) {
        len = strlen(src);
        if ((scopy = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
            return ERROR_INT("scopy not made", procName, 1);
        stringCopy(scopy, src, len);
        *pdest = scopy;
    }
    else
        *pdest = NULL;

    return 0;
}

/*!
 *  strtokSafe()
 *
 *      Input:  cstr (input string to be sequentially parsed;
 *                    use NULL after the first call)
 *              seps (a string of character separators)
 *              &saveptr (<return> ptr to the next char after
 *                        the last encountered separator)
 *      Return: substr (a new string that is copied from the previous
 *                      saveptr up to but not including the next
 *                      separator character), or NULL if end of cstr.
 *
 *  Notes:
 *      (1) This is a thread-safe implementation of strtok.
 *      (2) It has the same interface as strtok_r.
 *      (3) It differs from strtok_r in usage in two respects:
 *          (a) the input string is not altered
 *          (b) each returned substring is newly allocated and must
 *              be freed after use.
 *      (4) Let me repeat that.  This is "safe" because the input
 *          string is not altered and because each returned string
 *          is newly allocated on the heap.
 *      (5) It is here because, surprisingly, some C libraries don't
 *          include strtok_r.
 *      (6) Important usage points:
 *          - Input the string to be parsed on the first invocation.
 *          - Then input NULL after that; the value returned in saveptr
 *            is used in all subsequent calls.
 *      (7) This is only slightly slower than strtok_k.
 */
LEPTONICA_EXPORT char *
strtokSafe(char        *cstr,
           const char  *seps,
           char       **psaveptr)
{
char     nextc;
char    *start, *substr;
l_int32  istart, i, j, nchars;

    PROCNAME("strtokSafe");

    if (!seps)
        return (char *)ERROR_PTR("seps not defined", procName, NULL);
    if (!psaveptr)
        return (char *)ERROR_PTR("&saveptr not defined", procName, NULL);

    if (!cstr)
        start = *psaveptr;
    else
        start = cstr;
    if (!start)  /* nothing to do */
        return NULL;

        /* First time, scan for the first non-sep character */
    istart = 0;
    if (cstr) {
        for (istart = 0;; istart++) {
            if ((nextc = start[istart]) == '\0') {
                *psaveptr = NULL;  /* in case caller doesn't check ret value */
                return NULL;
            }
            if (!strchr(seps, nextc))
                break;
        }
    }

        /* Scan through, looking for a sep character; if none is
         * found, 'i' will be at the end of the string. */
    for (i = istart;; i++) {
        if ((nextc = start[i]) == '\0')
            break;
        if (strchr(seps, nextc))
            break;
    }

        /* Save the substring */
    nchars = i - istart;
    substr = (char *)CALLOC(nchars + 1, sizeof(char));
    stringCopy(substr, start + istart, nchars);

        /* Look for the next non-sep character.
         * If this is the last substring, return a null saveptr. */
    for (j = i;; j++) {
        if ((nextc = start[j]) == '\0') {
            *psaveptr = NULL;  /* no more non-sep characters */
            break;
        }
        if (!strchr(seps, nextc)) {
            *psaveptr = start + j;  /* start here on next call */
                break;
        }
    }

    return substr;
}


/*--------------------------------------------------------------------*
 *                       Find and replace procs                       *
 *--------------------------------------------------------------------*/
/*!
 *  stringRemoveChars()
 *
 *      Input:  src (input string; can be of zero length)
 *              remchars  (string of chars to be removed from src)
 *      Return: dest (string with specified chars removed), or null on error
 */
LEPTONICA_EXPORT char *
stringRemoveChars(const char  *src,
                  const char  *remchars) 
{
char     ch;
char    *dest;
l_int32  nsrc, i, k;

    PROCNAME("stringRemoveChars");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    if (!remchars)
        return stringNew(src);
    
    if ((dest = (char *)CALLOC(strlen(src) + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);
    nsrc = strlen(src);
    for (i = 0, k = 0; i < nsrc; i++) {
        ch = src[i];
        if (!strchr(remchars, ch))
            dest[k++] = ch;
    }

    return dest;
}

/*--------------------------------------------------------------------*
 *                             Safe realloc                           *
 *--------------------------------------------------------------------*/
/*!
 *  reallocNew()
 *
 *      Input:  &indata (<optional>; nulls indata)
 *              size of input data to be copied (bytes)
 *              size of data to be reallocated (bytes)
 *      Return: ptr to new data, or null on error
 *
 *  Action: !N.B. (3) and (4)!
 *      (1) Allocates memory, initialized to 0
 *      (2) Copies as much of the input data as possible
 *          to the new block, truncating the copy if necessary
 *      (3) Frees the input data
 *      (4) Zeroes the input data ptr
 *
 *  Notes:
 *      (1) If newsize <=0, just frees input data and nulls ptr
 *      (2) If input ptr is null, just callocs new memory
 *      (3) This differs from realloc in that it always allocates
 *          new memory (if newsize > 0) and initializes it to 0,
 *          it requires the amount of old data to be copied,
 *          and it takes the address of the input ptr and
 *          nulls the handle.
 */
LEPTONICA_EXPORT void *
reallocNew(void   **pindata,
           l_int32  oldsize,
           l_int32  newsize)
{
l_int32  minsize;
void    *indata; 
void    *newdata;

    PROCNAME("reallocNew");

    if (!pindata)
        return ERROR_PTR("input data not defined", procName, NULL);
    indata = *pindata;

    if (newsize <= 0) {   /* nonstandard usage */
        if (indata) {
            FREE(indata);
            *pindata = NULL;
        }
        return NULL;
    }

    if (!indata)   /* nonstandard usage */
    {
        if ((newdata = (void *)CALLOC(1, newsize)) == NULL)
            return ERROR_PTR("newdata not made", procName, NULL);
        return newdata;
    }

        /* Standard usage */
    if ((newdata = (void *)CALLOC(1, newsize)) == NULL)
        return ERROR_PTR("newdata not made", procName, NULL);
    minsize = L_MIN(oldsize, newsize);
    memcpy((char *)newdata, (char *)indata, minsize);

    FREE(indata);
    *pindata = NULL;

    return newdata;
}
    


/*--------------------------------------------------------------------*
 *                 Read and write between file and memory             *
 *--------------------------------------------------------------------*/

/*!
 *  fnbytesInFile()
 *
 *      Input:  file stream
 *      Return: nbytes in file; 0 on error
 */
LEPTONICA_EXPORT size_t
fnbytesInFile(FILE  *fp)
{
size_t  nbytes, pos;

    PROCNAME("fnbytesInFile");

    if (!fp)
        return ERROR_INT("stream not open", procName, 0);

    pos = ftell(fp);          /* initial position */
    fseek(fp, 0, SEEK_END);   /* EOF */
    nbytes = ftell(fp);
    fseek(fp, pos, SEEK_SET);        /* back to initial position */
    return nbytes;
}


/*--------------------------------------------------------------------------*
 *   16 and 32 bit byte-swapping on big endian and little  endian machines  *
 *                                                                          * 
 *   These are typically used for I/O conversions:                          *
 *      (1) endian conversion for data that was read from a file            *
 *      (2) endian conversion on data before it is written to a file        *
 *--------------------------------------------------------------------------*/

#ifdef L_BIG_ENDIAN

LEPTONICA_EXPORT l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return ((shortin << 8) | (shortin >> 8));
}

#else     /* L_LITTLE_ENDIAN */

LEPTONICA_EXPORT l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return  shortin;
}

#endif  /* L_BIG_ENDIAN */



/*--------------------------------------------------------------------*
 *                        Opening file streams                        *
 *--------------------------------------------------------------------*/
/*!
 *  fopenReadStream()
 *
 *      Input:  filename 
 *      Return: stream, or null on error
 *
 *  Notes:
 *      (1) This wrapper also handles pathname conversions for Windows.
 *          It should be used whenever you want to run fopen() to
 *          read from a stream.
 */
LEPTONICA_EXPORT FILE *
fopenReadStream(const char  *filename)
{
char  *fname, *tail;
FILE  *fp;

    PROCNAME("fopenReadStream");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);

        /* Try input filename */
    fname = genPathname(filename, NULL);
    fp = fopen(fname, "rb");
    FREE(fname);
    if (fp) return fp;

        /* Else, strip directory and try locally */
    splitPathAtDirectory(filename, NULL, &tail);
    fp = fopen(tail, "rb");
    FREE(tail);
    
    if (!fp)
        return (FILE *)ERROR_PTR("file not found", procName, NULL);
    return fp;
}

/*--------------------------------------------------------------------*
 *                         File name operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  splitPathAtDirectory()
 *
 *      Input:  pathname  (full path; can be a directory)
 *              &dir  (<optional return> root directory name of
 *                     input path, including trailing '/')
 *              &tail (<optional return> path tail, which is either
 *                     the file name within the root directory or
 *                     the last sub-directory in the path)
 *      Return: 0 if OK, 1 on error
 *       
 *  Notes:
 *      (1) If you only want the tail, input null for the root directory ptr.
 *      (2) If you only want the root directory name, input null for the
 *          tail ptr.
 *      (3) This function makes decisions based only on the lexical
 *          structure of the input.  Examples:
 *            /usr/tmp/abc  -->  dir: /usr/tmp/   tail: abc
 *            /usr/tmp/  -->  dir: /usr/tmp/   tail: [empty string]
 *            /usr/tmp  -->  dir: /usr/   tail: tmp
 *      (4) N.B. The input pathname must have unix directory separators
 *          for unix and windows directory separators for windows.
 */
LEPTONICA_EXPORT l_int32
splitPathAtDirectory(const char  *pathname,
                     char       **pdir,
                     char       **ptail)
{
char  *cpathname, *lastslash;

    PROCNAME("splitPathAtDirectory");

    if (!pdir && !ptail)
        return ERROR_INT("null input for both strings", procName, 1);
    if (pdir) *pdir = NULL;
    if (ptail) *ptail = NULL;
    if (!pathname)
        return ERROR_INT("pathname not defined", procName, 1);

    cpathname = stringNew(pathname);
    if ((lastslash = strrchr(cpathname, sepchar))) {
        if (ptail)
            *ptail = stringNew(lastslash + 1);
        if (pdir) {
            *(lastslash + 1) = '\0';
            *pdir = cpathname;
        }
        else
            FREE(cpathname);
    }
    else {  /* no directory */
        if (pdir)
            *pdir = stringNew("");
        if (ptail)
            *ptail = cpathname;
        else
            FREE(cpathname);
    }

    return 0;
}


/*! 
 *  genPathname()
 *
 *      Input:  dir (directory name, with or without trailing '/')
 *              fname (<optional> file name within the directory)
 *      Return: pathname (either a directory or full path), or null on error
 *
 *  Notes:
 *      (1) Use unix-style pathname separators ('/').
 *      (2) This function can be used in several ways:
 *            * to generate a full path from a directory and a file name
 *            * to convert a unix pathname to a windows pathname
 *            * to convert from the unix '/tmp' directory to the
 *              equivalent windows temp directory.
 *          The windows name translation is:
 *                   /tmp  -->   <Temp>/leptonica
 *      (3) There are three cases for the input:
 *          (a) @dir is a directory and @fname is null: result is a directory
 *          (b) @dir is a full path and @fname is null: result is a full path
 *          (c) @dir is a directory and @fname is defined: result is a full path
 *      (4) In all cases, the resulting pathname is not terminated with a slash
 *      (5) The caller is responsible for freeing the pathname.
 */
LEPTONICA_EXPORT char *
genPathname(const char  *dir,
            const char  *fname)
{
char    *cdir, *pathout;
l_int32  dirlen, namelen, size;
    
    PROCNAME("genPathname");

    if (!dir)
        return (char *)ERROR_PTR("dir not defined", procName, NULL);

        /* Remove trailing slash in dir, except when dir == "/"  */
    cdir = stringNew(dir);
    dirlen = strlen(cdir);
    if (cdir[dirlen - 1] == '/' && dirlen != 1) {
        cdir[dirlen - 1] = '\0';
        dirlen--;
    }

    namelen = (fname) ? strlen(fname) : 0;
    size = dirlen + namelen + 256;
    if ((pathout = (char *)CALLOC(size, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("pathout not made", procName, NULL);

#ifdef _WIN32
    {
        char  dirt[MAX_PATH];
        if (stringFindSubstr(cdir, "/", NULL) > 0) {
            char    *tempdir;
            l_int32  tdirlen;
            tempdir = stringReplaceEachSubstr(cdir, "/", "\\", NULL);
            tdirlen = strlen(tempdir);
            if (strncmp(dir, "/tmp", 4) == 0) {  /* get temp directory */
                GetTempPath(sizeof(dirt), dirt);
                stringCopy(pathout, dirt, strlen(dirt) - 1);
                stringCat(pathout, size, "\\leptonica");
                if (tdirlen > 4)
                    stringCat(pathout, size, tempdir + 4);

                    /* Set an extra null byte.  Otherwise, when setting
                       sepchar later, no trailing null byte remains. */
                pathout[strlen(pathout) + 1] = '\0';
            }
            else {
                stringCopy(pathout, tempdir, tdirlen);
            }
            FREE(tempdir);
        }
        else {  /* no '/' characters; OK as is */
            stringCopy(pathout, cdir, dirlen);
        }
    }
#else
    stringCopy(pathout, cdir, dirlen);
#endif  /* _WIN32 */

    if (fname && strlen(fname) > 0) {
        dirlen = strlen(pathout);
        pathout[dirlen] = sepchar;  /* append sepchar */
        strncat(pathout, fname, namelen);
    }
    FREE(cdir);
    return pathout;
}
