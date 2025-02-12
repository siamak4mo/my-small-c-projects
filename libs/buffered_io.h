/** file: buffered_io.h
    created on: 5 Sep 2024
  
    Buffered I/O
    It helps reduce the number of write system calls
    when writing multiple short lines (on stdout or other files)
  
    usage:
    ```c
    #include <stdio.h>
    #include <stdlib.h>
    #define BIO_IMPLEMENTATION
    #include "buffered_io.h"
  
    #define BMAX 64
  
    int
    main (void)
    {
      BIO_t bio = bio_newf (BMAX, malloc (BMAX), stdout);
  
      // do something here
      // like calling bio_putxx in a loop
      // and check for errors via bio_errxx
  
      // at the end flush the buffer and then free it
      bio_flushln (&bio);
      free (bio.buffer);
  
      return 0;
    }
    ```
 **/
#ifndef BUFFERED_IO__H
#define BUFFERED_IO__H
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifndef uchar
#  define uchar unsigned char
#endif

struct BIO {
  uchar *buffer;
  int len; /* @buffer length */
  int __len; /* occupied length */
  int __errno; /* write syscall errno */

  /* output file */
  int outfd;
};
typedef struct BIO BIO_t;

#ifndef BIODEFF
#  define BIODEFF static inline
#endif

// to set the output file descriptor
#define bio_out(bio, fd) (bio)->outfd = fd
#define bio_fout(bio, file) bio_out (bio, fileno (file))

// @out: output file descriptor
#define bio_new(cap, mem, out)                  \
  (BIO_t){.buffer=mem, .len=cap,                \
      .outfd=out, .__len=0, .__errno=0}
// out_f: *FILE
#define bio_newf(cap, mem, out_f) \
  bio_new(cap, mem, fileno (out_f)) 

#define bio_has_more(bio) ((bio)->__len > 0)
#define bio_is_empty(bio) ((bio)->__len == 0)

// to flush the buffer and zero out __len
#define bio_flush(bio) do {                             \
    if (write ((bio)->outfd,                            \
               (bio)->buffer, (bio)->__len) < 0)        \
      { (bio)->__errno = errno; }                       \
    (bio)->__len = 0;                                   \
  } while (0)

// safe flush, only sets __len=0 on successful write syscall
#define bio_sflush(bio) do {                            \
    if (write ((bio)->outfd,                            \
               (bio)->buffer, (bio)->__len) < 0) {      \
      (bio)->__errno = errno;                           \
    } else {                                            \
      (bio)->__len = 0;                                 \
    }} while (0)

// flush & print newline
BIODEFF int bio_flushln(BIO_t *bio);

// put char, bio is *BIO_t
#define bio_putc(bio, c) do {                   \
    (bio)->buffer[(bio)->__len++] = c;         \
    if ((bio)->__len >= (bio)->len) {           \
      bio_flush(bio);                           \
    }} while (0)

#define bio_err(bio) ((bio)->__errno != 0)
#define bio_errno(bio) ((bio)->__errno)

// function version of bio_putc macro
// returns errno on failure and 0 on success
BIODEFF int bio_fputc (BIO_t *bio, uchar c);

#define bio_ln(bio) bio_putc (bio, '\n')

// put buffer @ptr[@ptr_len]
BIODEFF int bio_put (BIO_t *bio, const char *ptr, int ptr_len);
// with an extra newline
BIODEFF int bio_putln (BIO_t *bio, const char *ptr, int ptr_len);

// puts string
#define bio_puts(bio, str) bio_putln (bio, str, strlen (str))
// puts without \n
#define bio_fputs(bio, str) bio_put (bio, str, strlen (str))


#ifdef BIO_IMPLEMENTATION

BIODEFF int
bio_fputc (BIO_t *bio, uchar c)
{
  bio_putc (bio, c);
  return bio->__errno;
}

// internal helper macro for unistd write
#define do_write(bio, p, p_len, failure)        \
  if (write ((bio)->outfd, p, p_len) < 0) {     \
    (bio)->__errno = errno;                     \
    failure;                                    \
  }
  

BIODEFF int
bio_put (BIO_t *bio, const char *ptr, int ptr_len)
{
  if (ptr_len + bio->__len < bio->len)
    {
      memcpy (bio->buffer + bio->__len, ptr, ptr_len);
      bio->__len += ptr_len;
      return 0;
    }
  else
    {
      bio_flush (bio);
      if (bio->__errno != 0)
        return bio->__errno;

      do_write (bio, ptr, ptr_len, {
          return bio->__errno;
        });
      return 0;
    }
}

BIODEFF int
bio_putln (BIO_t *bio, const char *ptr, int ptr_len)
{
   if (ptr_len + bio->__len + 1 < bio->len)
    {
      memcpy (bio->buffer + bio->__len, ptr, ptr_len);
      bio->__len += ptr_len;
      bio->buffer[bio->__len++] = '\n';
      return 0;
    }
  else
    {
      bio_flush (bio);
      if (bio->__errno != 0)
        return bio->__errno;

      do_write (bio, ptr, ptr_len, {
          return bio->__errno;
        });
      do_write (bio, "\n", 1, {
          return bio->__errno;
        });
      return 0;
    }
}

BIODEFF int
bio_flushln(BIO_t *bio)
{
  bio_flush (bio);
  if (bio->__errno != 0)
    return bio->__errno;

  do_write (bio, "\n", 1, {
      return bio->__errno;
    });
  return 0;
}
#endif /* BIO_IMPLEMENTATION */
#endif /* BUFFERED_IO__H */
