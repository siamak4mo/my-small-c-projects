/* This file is part of my-small-c-projects
   
  This program is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>. 
 */

/**
 *   file: codeM.c
 *   created on: 1 Oct 2023
 *
 *   Common Iranian ID number (code-e-melli) single-file library
 *
 *   `codem` in the following is referred to the term `code-e-melli`.
 *   this library performs validating and making random codem's.
 *
 *   compilation:
 *     to compile the CLI program:
 *       cc -Wall -Wextra -D CODEM_IMPLEMENTATION
 *                        -D CODEM_CLI -o codeM codeM.c
 *
 *     to compile the test program:
 *       cc -Wall -Wextra -D CODEM_IMPLEMENTATION
 *          -D CODEM_TEST -D CODEM_DEBUG -o test codeM.c
 *
 *     other compilation options:
 *       `-D CODEM_NO_CITY_DATA`:
 *          to compile without data of cites (ignore codeM_data.h)
 *       `-D CODEM_DEBUG`:
 *          to enable printing some debug information
 *       `-D CODEM_FUZZY_SEARCH_CITYNAME`:
 *          to enable fuzzy search, you need to provide
 *          the `leven.c` file (available in the same repository)
 *
 *     to include in c files:
 *       `
 *         #define CODEM_IMPLEMENTATION
 *         #include "codeM.c"
 *
 *         size_t my_rand_fun (void) {...}
 *         int main(...)
 *         {
 *           char codem[CODEM_BUF_LEN] = {0};
 *           // if you need to use any of the *_rand_* functions
 *           codem_rand_init (my_rand_fun);
 *         }
 *       `
 **/
#ifndef codeM__H__
#define codeM__H__
#include <string.h>

/* city_name and city_code data */
#include "codeM_data.h"

#ifdef CODEM_FUZZY_SEARCH_CITYNAME
#define LEVEN_IMPLEMENTATION /* fuzzy search feature */
#include "leven.c" /* provide leven.c */
#endif

typedef size_t(*RandFunction)(void);
/**
 *  codem_srand is the main pseudo-random number
 *  generator function used by this library
 *  initialization of codem_srand is necessary for using
 *  any of *_rand_* functions and macros
 */
RandFunction codem_srand = NULL;

#ifndef CODEMDEF
#define CODEMDEF static inline
#endif

/* codem is a numeric string of length 10 */
#define CODEM_LEN 10
/* control digit of codem is the last digit of it */
#define CTRL_DIGIT_IDX 9
/* it's important to allocate your buffers for codem of */
/* length 11, 10 char for codem and a 0-byte at the end */
#define CODEM_BUF_LEN 11

#define char2num(c) ((c) - '0')
#define num2char(x) ((x) + '0')
#define isanumber(c) (('0' <= (c)) && ('9' >= (c)))
/* macro to initialize codem_srand */
#define codem_rand_init(randfun) codem_srand = &(randfun)

/**
 *  internal macro to calculate the control-digit of the @codem
 *  only use `codem_*_ctrl_digit` functions
 */
#define ctrl_digit__H(res, codem) do{                      \
    (res) = 0;                                             \
    for (int __idx=CODEM_LEN-1; __idx--!=0;)               \
      (res) += (10 - __idx) * char2num ((codem)[__idx]);   \
    (res) %= 11;                                           \
    if ((res) >= 2) (res) = 11 - (res); } while(0)

/* get the name of city code @code */
#ifndef CODEM_NO_CITY_DATA
#define codem_cname_byidx(idx)                             \
  ({ int __idx = idx;                                      \
  ((__idx) == CC_NOT_FOUND) ? CCERR_NOT_FOUND              \
  : ((__idx) < 0) ? CCERR : city_name[__idx]; })
#define codem_cname(code)                                  \
  codem_cname_byidx(codem_ccode_idx (code))
#else
#define codem_cname_byidx(idx) CCERR_NOT_IMPLEMENTED
#define codem_cname(code) CCERR_NOT_IMPLEMENTED
#endif

/* get the codes of city at index @idx */
#ifndef CODEM_NO_CITY_DATA
#define codem_ccode(idx)                                   \
  ({ int __idx = idx;                                      \
    (__idx == CC_NOT_FOUND) ? CCERR_NOT_FOUND              \
      : (__idx < 0) ? CCERR                                \
      : city_code[idx]; })
#else
#define codem_ccode(idx) CCERR_NOT_IMPLEMENTED
#endif

/* validate only city code of @codem */
#define codem_ccode_isvalid(codem)                         \
  (codem_ccode_idx (codem) != CC_NOT_FOUND)

/* validate codem and it's city code */
#define codem_isvalid2(codem)                              \
  (codem_ccode_isvalid (codem) && codem_isvalid (codem))

/**
 *  internal macro to make random city indexes
 *  only use `codem_rand_ccode` function
 */
#define city_rand_idx__H() (int)((codem_srand ()) % CITY_COUNT)

/**
 *  return the correct control digit of codem
 *  ignore the current one
 */
CODEMDEF int codem_find_ctrl_digit (const char *codem);

/* set the control digit of @codem to the correct value */
CODEMDEF void codem_set_ctrl_digit (char *codem);

/**
 *  normalize @src and write the result on @dest
 *  normalized codem has exactly 10 digits
 *  return -1 when not possible
 */
CODEMDEF int codem_normcpy (char *dest, const char *src);

/* make @src normalized */
CODEMDEF int codem_norm (char *src);

/**
 *  validate the control digit of @codem
 *  after making it normalized,
 *  return 0 on normalization and validation failure
 */
CODEMDEF int codem_isvalid (const char *codem);

/**
 *  validate the control digit of @codem
 *  @codem should be normalized
 */
CODEMDEF int codem_isvalidn (const char *codem);

/**
 *  make a random 10-digit valid codem
 *  city code is not necessarily valid
 */
CODEMDEF void codem_rand (char *codem);

/* make a random valid codem with a valid city code */
CODEMDEF void codem_rand2 (char *codem);

/**
 *  generate random codem with suffix
 *  @offset is the length of the suffix
 */
CODEMDEF void codem_rands (char *codem, int offset);

/* write a valid random city code on @dest */
CODEMDEF void codem_rand_ccode (char *dest);

/**
 *  return the index of @codem[0:3] in city_code
 *  only use the `codem_cname_byidx` function
 *  to get the name of the city
 */
CODEMDEF int codem_ccode_idx (const char *codem);

/**
 *  search the @search among city names
 *  returns index of the best match
 */
CODEMDEF int
codem_cname_search (const char *search);
#endif /* codeM__H__ */


/* implementation */
#ifdef CODEM_IMPLEMENTATION

/**
 *  internal function
 *  returns 1 when @codem is numeric otherwise 0
 */
int
is_numeric(const char *codem)
{
  for (int idx = 0; idx < CODEM_LEN; ++idx)
    if (!isanumber(codem[idx]))
      return 0;
  return 1;
}

CODEMDEF int
codem_find_ctrl_digit (const char *codem)
{
  int res;

  ctrl_digit__H (res, codem);
  return res;
}

CODEMDEF void
codem_set_ctrl_digit (char *codem)
{
  int res;
  
  ctrl_digit__H (res, codem);
  codem[CTRL_DIGIT_IDX] = num2char (res);
}

CODEMDEF int
codem_normcpy (char *dest, const char *src)
{
  size_t l = strlen (src);

  if (l > CODEM_BUF_LEN-1)
    return -1; // error
  
  memset (dest, '0', CODEM_LEN - l);
  memcpy (dest + (CODEM_LEN - l), src, l);
  /* make dest null-terminated */
  dest[CODEM_LEN] = '\0';
  return 0;
}

CODEMDEF int
codem_norm (char *src)
{
  int ret;
  char res[CODEM_BUF_LEN];

  ret = codem_normcpy (res, src);
  strcpy (src, res);
  
  return ret;
}

CODEMDEF int
codem_isvalidn (const char *codem)
{
  if (!is_numeric (codem))
    return 0;

  return (codem[CTRL_DIGIT_IDX] ==
          num2char (codem_find_ctrl_digit (codem)));
}

CODEMDEF int
codem_isvalid (const char *codem)
{
  char codem_n[CODEM_BUF_LEN];
  
  if (0 != codem_normcpy (codem_n, codem))
    return 0;
  
  return codem_isvalidn (codem_n);
}

CODEMDEF void
codem_rand_gen (char *res, int len)
{
  unsigned long long rand = codem_srand ();
  
  while (0 != len--)
    {
      res[len] = num2char (rand % 10);
      rand /= 10;
    }
}

CODEMDEF void
codem_rand_ccode (char *dest)
{
#ifndef CODEM_NO_CITY_DATA
  int code_count = CC_LEN;
  int idx = city_rand_idx__H ();
  size_t rand = codem_srand ();
  const char *p = city_code[idx];
  const char *q = p;

  /* make a random choice between code at idx */
  while ('\0' != *q)
    {
      p = q;
      q += CC_LEN;
      code_count += CC_LEN;
      /* randomly break the loop -- code_count >= 6 */
      if (0 == rand % code_count)
        break;
    }
  strncpy (dest, p, CC_LEN);
#else
  size_t rand = codem_srand ();
  for (int idx = CC_LEN-1; idx >= 0; --idx)
    {
      dest[idx] = (rand%10) + '0';
      rand /= 10;
    }
#endif
}

CODEMDEF void
codem_rand (char *codem)
{
  codem_rand_gen (codem, CODEM_LEN - 1);
  codem_set_ctrl_digit (codem);
}

CODEMDEF void
codem_rand2 (char *codem)
{
  /* write a random city code */
  codem_rand_ccode (codem);
  /* fill the rest by random numbers */
  codem_rand_gen (codem + CC_LEN, CODEM_LEN - CC_LEN - 1);
  codem_set_ctrl_digit (codem);
}

CODEMDEF void
codem_rands (char *codem, int offset)
{
  if (offset < 9)
    codem_rand_gen (codem + offset, 9 - offset);
  codem_set_ctrl_digit (codem);
}

CODEMDEF int
codem_ccode_idx (const char *codem)
{
#ifndef CODEM_NO_CITY_DATA
  int idx = 0;
  const char *p;
  
  while (idx < CITY_COUNT)
    {
      p = city_code[idx];
      do{
        if (0 == strncmp (p, codem, CC_LEN))
          return idx;
        p += CC_LEN;
      } while (*p != '\0');
      p++;
      idx++;
    }

  return CC_NOT_FOUND;
#else
  (void) codem; /* prevent compiler warning */
  return CC_NOT_IMPLEMENTED;
#endif
}

CODEMDEF int
codem_cname_search (const char *search)
{
#ifdef CODEM_FUZZY_SEARCH_CITYNAME
  size_t min_dist = -1, min_dist_idx = CC_NOT_FOUND;
#endif

#ifndef CODEM_NO_CITY_DATA
  size_t n = strlen (search);
  const char *p;
  for (size_t idx = 0; idx < CITY_COUNT; ++idx)
    {
      p = city_name[idx];
#ifdef CODEM_FUZZY_SEARCH_CITYNAME /* fuzz */
      char *tmp = malloc (50);
      strncpy (tmp, p, (n>=50)?50:n);
      size_t LD = leven_imm (tmp, search);
      if (LD < min_dist)
        {
          min_dist = LD;
          min_dist_idx = idx;
        }
      free (tmp);
#else /* normal search */
      if (strncmp (search, p, n) == 0)
        return idx;
#endif /* CODEM_FUZZY_SEARCH_CITYNAME */
    }

#else
  return CC_NOT_IMPLEMENTED;
#endif /* CODEM_NO_CITY_DATA */

#ifdef CODEM_FUZZY_SEARCH_CITYNAME
  if (min_dist > leven_strlen(search) / 2)
    return CC_NOT_FOUND;
  return min_dist_idx;
#endif

  return CC_NOT_FOUND; /* unreachable */
}
#endif /* CODEM_IMPLEMENTATION */



/*------------------*/
/* the test program */
/*------------------*/
#ifdef CODEM_TEST
#include <stdio.h>
#include <assert.h>

#ifdef CODEM_DEBUG
#define DEBUG(fmt, ...) printf (fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...) do{} while (0)
#endif

/* assert char x is a number '0', ..., '9' */
#define assert_isnumber(x) assert ((x)>='0' && (x)<='9')

/* assert x is a 10-digit numeric string */
#define assert_10numeric(x) do{                             \
    char *__tmp = x; size_t __count=0;                      \
    while ('\0' != *__tmp) {                                \
      assert_isnumber (*__tmp);                             \
      __tmp++; __count++;                                   \
    } assert (10 == __count); } while(0);

#define FUN_TEST(fun, comment)                              \
  printf (" * "#fun" -- %s", comment);                      \
  fun ();                                                   \
  puts (" * PASS");


static inline int
validate (const char *codem)
{
  int r = codem_isvalidn (codem);

  if (r)
    DEBUG ("code %s is valid.\n", codem);
  else
    DEBUG ("code %s is not valid.\n", codem);
  
  return r;
}

/**
 *    Test Type 1
 *    
 *    tests:  codem_isvalidn,
 *            codem_set_ctrl_digit,
 *            codem_norm
 **/
static void
test_1_1 ()
{
  /* code doesn't need to be normalized */
  char code[CODEM_BUF_LEN] = "1234567890";
  /* code is not valid */
  assert (!validate (code));

  codem_set_ctrl_digit (code);
  /* code must be 1234567891 */
  DEBUG ("codem_set_ctrl_digit: %s\n", code);

  /* code must be valid */
  assert (validate (code));
}

static void
test_1_2 ()
{
  /* code needs to be normalized */
  char code[CODEM_BUF_LEN] = "567890";
  DEBUG ("before codem_norm: %s\n", code);
  
  codem_norm (code);
  DEBUG ("codem_norm: %s\n", code);
  /* code must be 0000567890 */
  assert (0 == strncmp (code, "0000567890", 10));
  
  /* code is not valid */
  assert (!validate (code));
  
  codem_set_ctrl_digit (code);
  /* code must be 0000567892 */
  DEBUG ("set_ctrl_digit: %s\n", code);

  /* code must be valid */
  assert (validate (code));
}

/**
 *    Test Type 2
 *    
 *    tests:  codem_rand, codem_rand2
 *            codem_rands
 *            codem_ccode_idx
 **/
static void
test_2_1 ()
{
  char code[CODEM_BUF_LEN] = {0};

  codem_rand (code);
  DEBUG ("codem_rand: %s\n", code);

  /* code must be a 10-digit numeric string */
  assert_10numeric (code);
  
  /* code must be valid */
  assert (validate (code));
}

static void
test_2_2 ()
{
  char code[CODEM_BUF_LEN] = "666";
  DEBUG ("suffix: %s, ", code);
  
  codem_rands (code, 3);
  DEBUG ("codem_rands: %s\n", code);

  /* check the suffix is intact */
  assert (0 == strncmp (code, "666", 3));

  /* code must be a 10-digit numeric string */
  assert_10numeric (code);

  /* code must be valid */
  assert (validate (code));
}

static void
test_2_3 ()
{
  char code[CODEM_BUF_LEN] = "";

  codem_rand2 (code);
  DEBUG ("codem_rand2: %s\n", code);

  int idx = codem_ccode_idx (code);

  /* check city code is valid */
  assert (idx != CC_NOT_FOUND);

  DEBUG ("city name: %s\n", city_name[idx]);
  /* idx must be 462 */
  assert (idx == 462);

  /* codem_isvalid2 must return 1 */
  assert (codem_isvalid2 (code));
}

/* pseudo random number generator function */
/* which always returns const 4242424242UL */
static inline size_t 
rand ()
{
  return 4242424242UL;
}

int
main (void)
{
  codem_rand_init (rand);
  
  /**   test type 1  **/
  puts ("/* Running test type 1 *******************/");
  FUN_TEST (test_1_1, "\n")
  FUN_TEST (test_1_2, "normalize\n");

  /**   test type 2  **/
  puts ("\n/* Running test type 2 *******************/");
  FUN_TEST (test_2_1, "random code generator\n");
  FUN_TEST (test_2_2, "random code with suffix\n");
  FUN_TEST (test_2_3, "random code with city code\n");
  
  return 0;
}
#endif /* CODEM_TEST */



/*-----------------*/
/* the CLI program */
/*-----------------*/
#ifdef CODEM_CLI
#ifdef CODEM_TEST
#error "cannot make CLI and test programs together"
#else
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#ifdef CODEM_DEBUG
/* debug macro to print codeM buffer */
#define printd(param)                           \
  printf ("[debug %s:%d] %s=[%s]\n",            \
          __func__, __LINE__, #param, param);
#define dprintf(format, ...)                    \
  printf ("[debug %s:%d] "format,               \
          __func__, __LINE__, __VA_ARGS__);
#else
#define printd(param) do{} while (0)
#define dprintf(format, ...) do{} while (0)
#endif

#define CNAME_MAX_BUFF 64
/* scanf formats */
#define CNAME_FORMAT "%64s"
#define CODEM_FORMAT "%10s"
/* sscanf regex to read a city name (non-ascii) */
#define CNAME_REGEX " %64[^;#]%n"
/* sscanf regex to read a 10-digit code */
#define CODEM_REGEX " %10[^;#]%n"
/* normalize character to prevent printing non-ascii characters */
#define NORMCHAR(c) ((c>0) ? ((c!='\n' && c!='\r') ? c : ' ') : '!')

static const char *PROMPT = "> ";
static const char *RD_PROMPT = "enter code: ";
static const char *CN_PROMPT = "enter name: ";
const char *__progname__;
/* a nonce for random number generator */
static size_t nonce = 0;

struct Opt {
  bool silent_mode;
  bool command_mode;
  bool prompt;
  bool EOO; /* End Of Options */
  char *commands; /* only in command_mode */
};

static void
help (struct Opt *opt)
{
  FILE *out_file;
  out_file = (!isatty (fileno (stdout))) ? stderr : stdout;

  if (opt->command_mode)
    fprintf (out_file,
             "Usage: ./codeM -c \"[COMMAND]\"\n"
             "COMMAND: sequence of shell mode commands\n"
             "commands could have one argument"
             " (Ex. `R 1234` ~ `R1234`)\n"
             "separate commands by space or `;` or "
             "`\\n` if you wish.\n\n");
  else
    fprintf (out_file,
             "v: validate            -  V: make my code valid\n"
             "c: randon city code    -  C: find my city name\n"
             "r: make random codem   -  R: make random codem with prefix\n"
             "f: find my city code   -  F: search my city name\n"
             "q: quit                -  h: help\n\n");
}

/* super simple pseudo random number generator */
static size_t
ssrand ()
{
  size_t r = time (NULL) + nonce++;

  for (int i=7; i>0; --i)
    {
      r += 0x666666;
      r *= 0x424242;
    }
  return r;
}

/**
 *  internal function to be used by codem_scanf and cname_scanf
 *  this function updates opt->commands when command_mode is true
 */
static inline int
scan__H (const char *restrict message, char *restrict dest,
         const char *restrict scan_format,
         const char *restrict sscan_regex, struct Opt *opt)
{
  int n;

  if (!opt->command_mode)
    {
      if (opt->prompt)
        printf (message);
      scanf (scan_format, dest);
      sscanf (dest, sscan_regex, dest, &n);
    }
  else
    sscanf (opt->commands, sscan_regex, dest, &n);
  opt->commands += n;
  return n;
}

static int
codem_scanf (const char *restrict message,
             char *restrict dest, struct Opt *opt)
{
  return scan__H (message, dest, CODEM_FORMAT, CODEM_REGEX, opt);
}

static int
cname_scanf (const char *restrict message,
             char *restrict dest, struct Opt *opt)
{
  return scan__H (message, dest, CNAME_FORMAT, CNAME_REGEX, opt);
}

/**
 *  returns 1 when program should be exited otherwise 0
 *  use @argv for commands that have argument otherwise
 *  pass it NULL to read from stdin
 */
static int
exec_command (char prev_comm, char comm, struct Opt *opt)
{
  dprintf("running: (%c), prev_command: (%c)\n",
          NORMCHAR(comm), NORMCHAR(prev_comm));
  int res, off;
  const char *p;
  char tmp[CODEM_BUF_LEN] = {0};
  char name_tmp[CNAME_MAX_BUFF] = {0};

  switch (comm)
    {
      /* validation */
    case 'v':
      codem_scanf (RD_PROMPT, tmp, opt);
      if (0 != codem_norm (tmp))
        assert ( 0 && "Cannot be Normalized" );
      printd(tmp);
      if (codem_isvalidn (tmp))
        {
          puts ("OK.");
          if (!codem_ccode_isvalid (tmp))
            puts ("city code was not found.");
        }
      else
        puts ("Not Valid.");
      break;

      /* make a code valid */
    case 'V':
      codem_scanf (RD_PROMPT, tmp, opt);
      if (0 != codem_norm (tmp))
        assert ( 0 && "Cannot be Normalized" );
      printd(tmp);
      codem_set_ctrl_digit (tmp);
      puts (tmp);
      break;

      /* make a random city code */
    case 'c':
      codem_rand_ccode (tmp);
      printf ("%.3s\n", tmp);
      break;
        
      /* find city name */
    case 'C':
      codem_scanf (RD_PROMPT, tmp, opt);
      printd(tmp);
      puts (codem_cname (tmp));
      break;
        
      /* make a random code */
    case 'r':
      codem_rand2 (tmp);
      puts (tmp);
      break;

      /* make a random code by prefix */
    case 'R':
      off = codem_scanf (RD_PROMPT, tmp, opt);
      printd(tmp);
      if (off > CODEM_LEN)
        assert (0 && "Invalid Offset of codem_scanf");
      else
        {
          codem_rands (tmp, off);
          puts (tmp);
        }
      break;

      /* find city name */
    case 'f':
      cname_scanf (CN_PROMPT, name_tmp, opt);
      res = codem_cname_search (name_tmp);
      printd(name_tmp);
      p = codem_ccode(res);
      if (res < 0)
          puts (p);
      else
        for (; *p != 0; p += CC_LEN)
          printf("%.3s\n", p);
      break;

      /* search city name */
    case 'F':
      cname_scanf (CN_PROMPT, name_tmp, opt);
      printd(name_tmp);
      res = codem_cname_search (name_tmp);
      p = codem_cname_byidx(res);
      puts(p);
      break;

      /* print help */
    case 'h':
      help (opt);
      break;

    case 'q':
      return 1;

    case '\n':
    case '\r':
    case '\\':
    case ' ': /* separator */
    case ';': /* separator */
    case '#': /* comment */
      return 0;

      /* invalid command */
    default:
      if ((prev_comm == '\n' || prev_comm == '\0' || prev_comm == ' ' ||
           prev_comm == ';'  || opt->command_mode) && prev_comm != '#')
        fprintf (stderr, "Invalid command -- (%c)\n", comm);
    }
  return 0;
}

static inline void
normalize_command (char *restrict prev_comm,
                   char *restrict comm)
{
  if ('\\' == *prev_comm && '\\' != *comm)
    {
      *prev_comm = ' ';
      switch (*comm)
        {
        case 'n':
          *comm = '\n';
          break;

        case 'r':
          *comm = '\r';
          break;

        default:
          *comm = '\0';
          break;
        }
    }
}

static inline int
pars_options (int argc, char **argv, struct Opt *opt)
{
  __progname__ = argv[0];
  for (argc--, argv++; argc > 0; argc--, argv++)
    {
      if (!opt->EOO && argv[0][0] == '-')
        {
          switch(argv[0][1])
            {
            case 's':
              opt->silent_mode = true;
              break;

            case 'S':
              opt->prompt = false;
              break;

            case 'c':
              if (argc == 1)
                {
                  fprintf (stderr, "Not enough arguments");
                  return -1;
                }
              else
                {
                  opt->silent_mode = true;
                  opt->prompt = false;
                  opt->commands = *(argv+1);
                  opt->command_mode = true;
                }
              break;

            default:
              fprintf (stderr, "Invalid option (%s)", argv[0]);
              return -2;
            }
        }
    }
  return 0;
}

int
main (int argc, char **argv)
{
  char comm = '\0', prev_comm = comm;
  static struct Opt opt = {
    .silent_mode = false,
    .command_mode = false,
    .prompt = true,
    .commands = NULL,
    .EOO = false,
  };
  /* initialize codeM random number generator function */
  codem_rand_init (ssrand);
  /* parsing cmdline arguments */
  if (pars_options (argc, argv, &opt))
    {
      fprintf (stderr, " -- exiting.\n");
      return 1;
    }
  /* disable the prompt when `stdin` is not a tty (using pipe) */
  if (!isatty (fileno (stdin)))
    {
      opt.silent_mode = true;
      opt.prompt = false;
    }
  if (opt.command_mode)
    {
      /* run commands from cmdline args, available in opt->commands */
      while (*opt.commands != '\0')
        {
          prev_comm = comm;
          comm = *opt.commands;
          opt.commands++;
          /* interpretation of backslash escapes */
          normalize_command (&prev_comm, &comm);
          if (exec_command (prev_comm, comm, &opt))
            return 0;
        }
    }
  else /* shell mode */
    {
      if (!opt.silent_mode && opt.prompt)
        {
          printf ("codeM Shell Mode!\n"
                  "Usage: %s [OPTIONS] [COMMANDS]\n"
                  "OPTIONS:\n"
                  "   -s:    silent mode\n"
                  "   -S:    disable the prompt (when using pipe)\n"
                  "   -c:    pass COMMANDS to be executed,\n"
                  "          use: -c \"h\" to get help\n\n", __progname__);
          help (&opt);
        }
      while (1)
      {
        if (('\0' == comm || '\n' == comm) && opt.prompt)
          fprintf (stdout, "%s", PROMPT);

        prev_comm = comm;
        if (EOF == scanf ("%c", &comm))
          {
            if (opt.prompt)
              puts("");
            return 0;
          }
        if (exec_command (prev_comm, comm, &opt))
          return 0;
      }
    }
  return 0;
}

#endif /* CODEM_TEST */
#endif /* CODEM_CLI */
