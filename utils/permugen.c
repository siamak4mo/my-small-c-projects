/* This file is part of my-small-c-projects <https://gitlab.com/SI.AMO/>

  Permugen is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  Permugen is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 *  file: permugen.c
 *  created on: 4 Sep 2024
 *
 *  Permugen, permutation generator utility
 *  to generate customizable permutations of given seeds
 *
 *  Usage:  ./permugen [OPTIONS] [ARGUMENTS]
 *    for more details see help function via `-h` option:
 *    $ ./permugen -h
 *
 *  Some usage examples:
 *    - To make permutations of A,B,C, and a,...,f
 *    ./permugen -s "[ABC] [a-f]" -d4             # with length 4
 *    ./permugen -s "[ABCa-f]" -d4                # equivalent
 *    ./permugen -s "[ABCa-f]" -D4                # with length 1,...,4
 *    ./permugen --min-depth 3 --max-depth 5      # with length 3,...,5
 *
 *    - To include word(s) in permutations
 *    ./permugen -s "{foo,bar}"
 *    ./permugen -s "[0-4] {foo,bar,baz}"         # to also include 0,...,4
 *    ./permugen -s "[xyz0-4] {foo,bar,baz}"      # to also include x,y,z
 *    ./permugen -s "[0-4] [x-z] {foo,bar,baz}"   # equivalent
 *
 *    - Output formatting (separator `-p` and format `-f`)
 *      to disable backslash interpretation (default) use `-E`
 *    ./permugen --delim ", "                     # comma separated
 *    ./permugen --delim "\t"                     # tab separated
 *    # using `www.` as prefix and `.com` as suffix
 *    # you may use --prefix, --suffix instead for more control
 *    ./permugen --format "www. .com"
 *
 *
 *  Compilation:
 *    to compile with `buffered_io.h`:
 *      cc -ggdb -O3 -Wall -Wextra -Werror \
 *         -D_USE_BIO -I../libs \
 *         -o permugen permugen.c
 *
 *  Compilation options:
 *    - To compile without buffered IO (less performance)
 *      remove `-D_USE_BIO`
 *
 *    - define `-D_BMAX="1024 * 1"` (in bytes) to change
 *      the default buffered IO buffer length
 *
 **/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static const char *__progname__;
static const char *__PROGVERSION__ = "v1.2";

/**
 *  using buffered_io.h for better performance
 *  this file should be available in this repo
 */
#ifdef _USE_BIO
#  ifndef _BMAX
#    define _BMAX (sysconf (_SC_PAGESIZE) / 2) // 2kb
#  endif
#  define BIO_IMPLEMENTATION
#  include "buffered_io.h"
#endif

/* for using unescape function */
#define UNESCAPE_IMPLEMENTATION
#include "unescape.h"

/* for using dynamic array */
#define DYNA_IMPLEMENTATION
#include "dyna.h"

/* default permutaiton depth */
#ifndef DEF_DEPTH
#  define DEF_DEPTH 3
#endif

#ifdef _DEBUG
#undef dprintf
#define dprintf(format, ...) fprintf (stderr, format, ##__VA_ARGS__)
/* use printd_arr */
#  define printd_arr__H(arr, T, len, sep, end)  \
  for (int __idx = 0; __idx < len; __idx++) {   \
    dprintf (T"%s", arr[__idx],                 \
             (__idx < len-1) ? sep : end);      \
  }
/**
 * debug printf for arrays
 * pass printf format for each element of @arr with @T
 * Ex: to print `int arr[7]`  -->  printd_arr (arr, "%d", 7);
 */
#  define printd_arr(arr, T, len)               \
  if (len > 0) {                                \
    dprintf ("* "#arr"[.%d] = {", len);         \
    printd_arr__H (arr, T, len, ", ", "}\n");   \
  } else {                                      \
    dprintf ("- "#arr" is empty\n");            \
  }
#endif /* _DEBUG */


#undef errorf
#define errorf(format, ...) \
  fprintf (stderr, format"\n", ##__VA_ARGS__)
#define argerr(arg, message) \
  errorf ("(%s) was ignored -- "message, arg)
// prints perror at the end
#undef perrorf
#define perrorf(format, ...)                                    \
  (fprintf (stderr, format, ##__VA_ARGS__), perror(NULL))

struct char_seed
{
  const char *c;
  int len;
};
static const struct char_seed charseed_az = {"abcdefghijklmnopqrstuvwxyz", 26};
// static const struct char_seed charseed_AZ = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26};
static const struct char_seed charseed_09 = {"0123456789", 10};

/**
 *  seeds container
 *  wseed within this struct is dynamic array
 *  to handle it properly, only use `xxx_uniappd`
 *  and regex parser functions
 *  also to have length of it, use `da_sizeof (wseed)`
 */
struct Seed
{
  /* char seed */
  char *cseed;
  int cseed_len;
  /* word seed, dynamic array */
  char **wseed;
};
#define CSEED_MAXLEN 256
/* unique append to seed functions */
int cseed_uniappd (struct Seed *, const char *src, int len);
void wseed_uniappd (struct Seed *, char *str_word);
/* simple seed regex handler function */
void parse_seed_regex (struct Seed *, const char *str_regex);

struct Opt
{
  /* main seed configuration */
  struct Seed *global_seeds;

  /* output format */
  int escape_disabled; /* to disable backslash interpretation */
  char *__pref; /* prefix */
  char *__suff; /* suffix */
  char *__sep; /* separator */

  /* output conf */
  FILE *outf; /* output file */
  int from_depth; /* min depth */
  int to_depth; /* max depth */

  /* buffered_io */
#ifdef _USE_BIO
  BIO_t *bio;
#endif
};

#define _strcmp(s1, s2)                         \
  ((s1) != NULL && (s2) != NULL &&              \
   strncmp ((s1), (s2), strlen (s2)) == 0)

/* is non white-space ascii-printable */
#define IS_ASCII_PR(c) (c >= 0x21 && c <= 0x7E)


void
usage ()
{
  fprintf (stdout,
           "Permugen %s, permutation generator utility\n"
           "Usage: %s [OPTIONS] [ARGUMENTS]\n\n"
           "OPTIONS:\n"
           "      -E                      disable backslash interpretation\n"
           "      -e                      enable backslash interpretation (default)\n"
           "      -d, --depth             specify depth\n"
           "      -D, --depth-range       depth range\n"
           "     -df, --depth-from        specify min depth\n"
           "          --min-depth\n"
           "     -dt, --depth-to          specify max depth\n"
           "          --max-depth\n"
           "      -o, --output            output file\n"
           "  -a,-oA, --append            append to file\n"
           "      -p, --delimiter         permutations component separator\n"
           "      -S, --seed-path         word seed path\n"
           "                              pass - to read from stdin\n"
           "      -s, --seed              to configure global seeds (see ARGUMENTS)\n"
           "          --raw-seed          to configure character seeds\n"
           "          --raw-wseed         to add a single word to global seeds\n"
           "      -f, --format            output format (see ARGUMENTS)\n"
           "          --prefix            output prefix\n"
           "          --suffix            output suffix\n\n"
           "ARGUMENTS:\n"
           "  Argument values of --format, --prefix, --suffix, --raw-xxx, and --delimiter\n"
           "  will be backslash-interpreted by default, (disable it by `-E`)\n\n"
           "  format: `AAA`:     to use AAA as the output prefix\n"
           "          `AAA BBB`  to use AAA as prefix and BBB as suffix\n"
           "          ` BBB`     to use BBB as suffix\n"
           "          BBB might contain white-space character(s)\n"
           "          to have white-space in AAA, either use `\\x20` or --prefix\n\n"
           "    seed: this option accepts a simple regex as argument\n"
           "          `[XYZ]`:          to use characters X,Y,Z as character seed\n"
           "          `[a-f]`:          to use characters a,b,...,f\n"
           "          `[\\[\\]]`:         to use `[`,`]` characters\n"
           "          `{word1,word2}`   to include `word1` and `word2` in word seeds\n"
           "                            use `--raw-wseed` if your words contain comma\n"
           "          example:\n"
           "            to include a,b and 0,...,9  also words foo,bar:\n"
           "             `[ab0-9] {foo,bar}`  or equivalently  `[ab] {foo,bar} [0-9]`\n"
           ,__PROGVERSION__, __progname__);
}

/**
 *  output of characters and strings
 *  these functions write on the corresponding
 *  output stream given in @opt->outf
 *
 *  Pfputs:  writes @str without its terminating null byte
 *  Pputs:   writes @str like @Pfputs and a trailing newline
 *  Pputc:   writes a character @c (as unsigned char)
 *  Pputln:  writes a newline
 */
#ifndef _USE_BIO
#  define Pfputs(str, opt) fputs (str, opt->outf)
#  define Pfputc(c, opt) putc (c, opt->outf)
#  define Pputln(opt) Pfputc ('\n', opt)
#  define Pputs(str, opt) (Pfputs (str, opt), Pputln(opt))
#else
#  define Pfputs(str, opt) bio_fputs (opt->bio, str)
#  define Pfputc(c, opt) bio_putc (opt->bio, c)
#  define Pputln(opt) bio_ln (opt->bio);
#  define Pputs(str, opt) bio_puts (opt->bio, str)
#endif

/**
 *  permutation generator main logic
 *  you need to call it in a loop from
 *  depth=min_depth to depth=max_depth
 */
int
perm (const int depth, const struct Opt *opt)
{
  int idxs[depth];
  memset (idxs, 0, depth * sizeof (int));

 Perm_Loop: /* O(seeds^depth) */
  int i = 0;
  if (opt->__pref)
    Pfputs (opt->__pref, opt);
 Print_Loop: /* O(depth) */
  {
    int idx = idxs[i];
    if (idx < opt->global_seeds->cseed_len)
      {
        /* range of character seeds */
        Pfputc (opt->global_seeds->cseed[idx], opt);
      }
    else
      {
        /* range of word seeds */
        idx -= opt->global_seeds->cseed_len;
        Pfputs (opt->global_seeds->wseed[idx], opt);
      }
    i++;
  }
  if (i < depth)
    {
      if (opt->__sep)
        Pfputs (opt->__sep, opt);
      goto Print_Loop;
    }
  /* End of Printing the current permutation */
  if (opt->__suff)
    Pputs (opt->__suff, opt);
  else
    Pputln (opt);


  int pos;
  for (pos = depth - 1;
       pos >= 0 &&
         idxs[pos] == opt->global_seeds->cseed_len - 1 +
         (int)da_sizeof (opt->global_seeds->wseed);
       pos--)
    {
      idxs[pos] = 0;
    }

  if (pos < 0) /* End of Permutations */
    {
#ifdef _USE_BIO
      if (bio_err (opt->bio))
        {
          /* buffered_io write error */
          return bio_errno (opt->bio);
        }
      else
        return 0;
#else
      return 0;
#endif
    }

  idxs[pos]++;
  goto Perm_Loop;
}

/**
 *  uniquely appends char(s) from @src to @dest
 *  until reaches \0 or non-printable characters
 *  @dest with *CAPACITY* 256 is always enough
 *  time: O(src_len * dest_len);
 *  updates @dest_len and returns number of bytes written
 */
int
cseed_uniappd (struct Seed *s, const char *src, int len)
{
  int rw = 0;
  while (len > 0 && *src)
    {
      if (!IS_ASCII_PR (*src))
        break;
      for (int __i = s->cseed_len - 1; __i >= 0; __i--)
        {
          if (*src == s->cseed[__i])
            goto END_OF_LOOP;
        }
      s->cseed[s->cseed_len] = *src;
      s->cseed_len++;
      rw++;

    END_OF_LOOP:
      src++;
      len--;
    }

  return rw;
}

/**
 *  uniquely appends reference of @word
 *  to @opt->wseed (dynamic array)
 *  use strdup when @word gets dereferenced
 */
void
wseed_uniappd (struct Seed *s, char *str_word)
{
  if (!s->wseed || !str_word)
    return;
  for (idx_t i=0; i < da_sizeof (s->wseed); ++i)
    {
      if (_strcmp (s->wseed[i], str_word))
        return;
    }
  da_appd (s->wseed, str_word);
}

/**
 *  safe file open (fopen)
 *  only if it could open @pathname changes @dest[0]
 *  @mode is the same as fopen mode
 */
void
safe_fopen (FILE **dest,
            const char *restrict pathname,
            const char *restrict mode)
{
  FILE *__tmp;
  if (!pathname || !mode)
    {
      errorf ("Invalud filename");
      return;
    }
  if (!(__tmp = fopen (pathname, mode)))
    {
      perrorf ("Could not open file (%s:%s) -- ", mode, pathname);
      return;
    }
  *dest = __tmp;
}

/* CLI options, getopt */
const struct option lopts[] = {
  /* seeds */
  {"seed", required_argument, NULL, 's'},
  {"raw-seed", required_argument, NULL, '0'},
  {"raw-wseed", required_argument, NULL, '5'},
  {"seed-path", required_argument, NULL, 'S'},
  {"wseed-path", required_argument, NULL, 'S'},
  /* output file */
  {"output", required_argument, NULL, 'o'},
  {"append", required_argument, NULL, 'a'},
  {"oA", required_argument, NULL, 'a'},
  {"help", no_argument, NULL, 'h'},
  /* delimiter */
  {"delim", required_argument, NULL, 'p'},
  {"delimiter", required_argument, NULL, 'p'},
  /* depth */
  {"depth", required_argument, NULL, 'd'},
  {"depth-range", required_argument, NULL, 'D'},
  {"range-depth", required_argument, NULL, 'D'},
  {"range", required_argument, NULL, 'D'},
  {"df", required_argument, NULL, '1'},
  {"depth-from", required_argument, NULL, '1'},
  {"from-depth", required_argument, NULL, '1'},
  {"min-depth", required_argument, NULL, '1'},
  {"dt", required_argument, NULL, '2'},
  {"depth-to", required_argument, NULL, '2'},
  {"to-depth", required_argument, NULL, '2'},
  {"max-depth", required_argument, NULL, '2'},
  /* format */
  {"format", required_argument, NULL, 'f'},
  {"pref", required_argument, NULL, '3'},
  {"prefix", required_argument, NULL, '3'},
  {"suff", required_argument, NULL, '4'},
  {"suffix", required_argument, NULL, '4'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

int
init_opt (int argc, char **argv, struct Opt *opt)
{
  int idx = 0, flag, using_default_seed = 1;
  while (1)
    {
      /* we use 0,1,2,... as `helper` options and only to use getopt */
      flag = getopt_long (argc, argv,
                          "s:S:o:a:p:d:f:D:0:1:2:3:4:5:hEe", lopts, &idx);
        if (flag == -1)
          {
            /* End of Options */
            break;
          }

      switch (flag)
        {
        case 'h':
          usage ();
          return 1;
        case 'E':
          opt->escape_disabled = 1;
          break;
        case 'e':
          opt->escape_disabled = 0;
          break;
        case 'o': /* outout */
          safe_fopen (&opt->outf, optarg, "w");
          break;
        case 'a': /* append */
          safe_fopen (&opt->outf, optarg, "a");
          break;
        case 'd': /* depth */
          opt->from_depth = atoi (optarg);
          break;
        case 'D': /* depth range */
          opt->from_depth = 1;
          opt->to_depth = atoi (optarg);
          break;
        case 'p': /* delimiter */
          opt->__sep = optarg;
          break;
        case '3': /* prefix */
          opt->__pref = optarg;
          break;
        case '4': /* suffix */
          opt->__suff = optarg;
          break;
        case '1': /* depth from */
          opt->from_depth = atoi (optarg);
          break;
        case '2': /* depth to */
          opt->to_depth = atoi (optarg);
          break;
        case 'f': /* format */
          {
            opt->__pref = optarg;
            for (char *p = optarg; *p != '\0'; ++p)
              {
                if (*p == ' ')
                  {
                    *(p++) = '\0';
                    if (*p != '\0')
                      opt->__suff = p;
                    break;
                  }
              }
            if (opt->__pref && opt->__pref[0] == '\0')
              opt->__pref = NULL;
            break;
          }
        case 'S': /* wseed file / stdin */
          {
            size_t __len;
            char *__line = NULL;
            FILE *wseed_f = stdin;
            /* using optarg value as filepath otherwise stdin */
            if (!_strcmp (optarg, "-"))
              safe_fopen (&wseed_f, optarg, "r");
            if (wseed_f == stdin && isatty (fileno (stdin)))
              fprintf (stderr, "reading words from stdin:\n");

            while (1)
              {
                if (getline (&__line, &__len, wseed_f) < 0)
                  break;
                if (__line && *__line &&
                    __line[0] != '\n' &&  // empty line
                    __line[0] != '#') // commented line
                  {
                    __line[strlen (__line) - 1] = '\0';
                    wseed_uniappd (opt->global_seeds, strdup (__line));
                  }
              }
            if (__line)
              free (__line);
            if (wseed_f != stdin)
              fclose (wseed_f);
          }
          break;

        case 's': /* seed configuration */
          {
            using_default_seed = 0;
            /* this option disables the default seed config */
            parse_seed_regex (opt->global_seeds, optarg);
          }
          break;

        case '0': /* raw seed */
          using_default_seed = 0;
          if (!opt->escape_disabled)
            unescape (optarg);
          cseed_uniappd (opt->global_seeds, optarg, strlen (optarg));
          break;

        case '5': /* raw word seed */
          if (!opt->escape_disabled)
            unescape (optarg);
          wseed_uniappd (opt->global_seeds, optarg);

        default:
          break;
        }
    }

  /**
   *  Initializing the default values
   *  when they are not specified by the user
   */
  {
    if (opt->outf == NULL)
      opt->outf = stdout;

    if (opt->global_seeds->cseed_len == 0 && using_default_seed)
      {
        /* initializing with the default seed [a-z0-9] */
        cseed_uniappd (opt->global_seeds,
                       charseed_az.c, charseed_az.len);
        cseed_uniappd (opt->global_seeds,
                       charseed_09.c, charseed_09.len);
      }

    if (opt->from_depth <= 0 && opt->to_depth <= 0)
      {
        /* using the default values when not specified */
        opt->from_depth = DEF_DEPTH;
        opt->to_depth = DEF_DEPTH;
      }
    else if (opt->to_depth <= 0)
      {
        /* only from_depth is specified OR `-D` is being used */
        opt->to_depth = opt->from_depth;
      }
    if (opt->from_depth > opt->to_depth)
      {
        /* invalid min and max depths */
        opt->to_depth = opt->from_depth;
      }
  }

  /* interpreting backslash character(s) */
  if (!opt->escape_disabled)
    {
      if (opt->__pref != NULL)
        unescape (opt->__pref);
      if (opt->__suff != NULL)
        unescape (opt->__suff);
      if (opt->__sep != NULL)
        unescape (opt->__sep);
    }

  return 0;
}

struct Seed *
mk_seed ()
{
  struct Seed *s = malloc (sizeof (struct Seed));
  if (!s)
    return NULL;
  memset (s, 0, sizeof (struct Seed));
  s->cseed = malloc (CSEED_MAXLEN);
  s->wseed = da_new (char *);
  return s;
}

int
main (int argc, char **argv)
{
    struct Opt opt = {0};
  __progname__ = *argv;

  { /* initializing options */
    opt.global_seeds = mk_seed ();
    if (init_opt (argc, argv, &opt))
      goto EndOfMain;

    if (opt.global_seeds->cseed_len == 0 &&
        da_sizeof (opt.global_seeds->wseed) == 0)
      {
        errorf ("Warning -- Empty permutation!");
        goto EndOfMain;
      }
  }

#ifdef _USE_BIO
  int cap = _BMAX;
  BIO_t __bio = bio_new (cap, malloc (cap), fileno (opt.outf));
  opt.bio = &__bio;
#endif

#ifdef _DEBUG
  /* print some debug information */
  printd_arr (opt.global_seeds->cseed, "`%c`", opt.global_seeds->cseed_len);
  printd_arr (opt.global_seeds->wseed, "`%s`", (int) da_sizeof (opt.global_seeds->wseed));
  if (opt.escape_disabled)
    dprintf ("- backslash interpretation is disabled\n");
  if (opt.__sep)
    dprintf ("* delimiter: `%s`\n", opt.__sep);
  dprintf ("* depth: from %d to %d\n", opt.from_depth, opt.to_depth);

# ifdef _USE_BIO
  dprintf ("* buffered_io buffer length: %ld bytes\n", _BMAX);
# else
  dprintf ("- compiled without buffered_io\n");
# endif /* _USE_BIO */
  dprintf ("* permutations:\n");
#endif /* _DEBUG */


  { /* organizing permutation loop */
    int rw_err = 0;
    for (int d = opt.from_depth; d <= opt.to_depth; ++d)
      {
        if ((rw_err = perm (d, &opt)) != 0)
          break;
      }
  }


#ifdef _USE_BIO
  bio_flush (opt.bio);
  free (opt.bio->buffer);
#endif


 EndOfMain:
  if (opt.global_seeds->cseed)
    free (opt.global_seeds->cseed);
  if (opt.global_seeds->wseed)
    da_free (opt.global_seeds->wseed);
  /* close any non-stdout file descriptors */
  if (opt.outf && fileno (opt.outf) != 1)
    fclose (opt.outf);
  free (opt.global_seeds);

  return 0;
}


/**
 *  internal regex functions
 *  these functions are used to parse `-s` argument
 *  regex which is used to configure seeds
 */

/** wseed provider
 *  inside {...} - comma-separated values
 *  comma is not allowed in wseeds
 */
const char *
__preg_wseed_provider (struct Seed *s, const char *p)
{
  const char *next_p = p + 1;
  const char *start = p;
  size_t len = 0;

#undef __forward
#define __forward(n) (p += n, next_p += n)
#undef seedout
#define seedout() do {                          \
    char *str = malloc (++len);                 \
    memcpy (str, start, len);                   \
    str[len - 1] = '\0';                        \
    wseed_uniappd (s, str);                     \
  } while (0)

  for (; *p != '\0'; __forward (1))
    {
      switch (*p)
        {
        case '\0':
          return p;

        case '}':
          if (len > 0)
            seedout ();
          return p+1;

        case ',':
          if (len > 0)
            seedout ();
          start = p+1, len = 0;
          break;

        default:
          len++;
        }
    }
  return p;
}

/** character seed provider
 *  inside [...]
 *  X-Y: means range X to Y
 *  `\[` and `\]`: means `[`,`]` characters
 */
const char *
__preg_cseed_provider (struct Seed *s, const char *p)
{
  const char *next_p = p + 1;
  static char seed = 0;

#undef __forward
#define __forward(n) (p += n, next_p += n)
#undef seedout
#define seedout() cseed_uniappd (s, &seed, 1)

  for (; *p != '\0'; __forward (1))
    {
      switch (*p)
        {
        case '\0':
          /* End of Regex */
          return p;

        case '\\':
          if (*next_p == ']' || *next_p == '[')
            {
              seed = *next_p;
              __forward (1);
              goto Put_a_char;
            }
          break;

        case ']':
          return p;

        case '-':
          if (*(p - 1) != '[')
            {
              if (*next_p != ']' && *next_p != '\\')
                {
                  for (seed = *(p - 1); seed <= *next_p; ++seed)
                    {
                      seedout ();
                    }
                  __forward (1);
                }
              else
                {
                  seed = *p;
                  goto Put_a_char;
                }
            }
          else
            {
              seed = *p;
              goto Put_a_char;
            }
          break;

        default:
          {
            switch (*next_p)
              {
              case '-':
                break;

              case '\0':
              case ']':
              default:
                seed = *p;
              Put_a_char:
                seedout ();
              }
          }
        }
    }
  return p;
}

// main seed regex parser function
void
parse_seed_regex (struct Seed *s, const char *input)
{
  // input: "  [..\[..]  {..\{..} "
  char prev_p = 0;
  for (; *input != '\0'; prev_p = *input, ++input)
    {
      if (*input == '[' && prev_p != '\\')
        {
          input = __preg_cseed_provider (s, ++input);
        }
      else if (*input == '{' && prev_p != '\\')
        {
          input = __preg_wseed_provider (s, ++input);
        }
    }
}
