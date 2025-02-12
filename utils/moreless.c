/* This file is part of my-small-c-projects <https://gitlab.com/SI.AMO/>

  Moreless is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  Moreless is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/** file: moreless.c
    created on: 11 Feb 2025

   More Less
   A simple tool to view program's output
   through the less command

   ** This only works with the GNU C Library (glibc)
      For other libc implementations, you need to overwrite
      the equivalent `__libc_start_main` function **

   Usage:
     For a single command:
       $ LD_PRELOAD="/path/to/moreless.so" ls -ltrh
     Or make it permanent:
       $ export LD_PRELOAD="/path/to/moreless.so"
       Then run your commands.

     To exclude command(s) from moreless:
       To append to the default excludes:
       $ export MORELESS_EXCLUDE=":ls:mpv"

       To overwrite the default excludes:
       $ export MORELESS_EXCLUDE="less:tmux:mpv"

       See the default excludes in `DEFAULT_EXCLUDES`.


   Here is a simple bash function to toggle moreless:
     ```{bash}
     function moreless ()
       {
         case $LD_PRELOAD in
         *"moreless.so"*)
           unset MORELESS_EXCLUD
           unset LD_PRELOAD
           echo "moreless disabled."
           ;;
         *)
           export LD_PRELOAD="/path/to/moreless.so"
           echo "moreless enabled."
           ;;
         esac
       }
     ```
     You might want to eliminate /path/to/moreless.so
     from LD_PRELOAD instead of using `unset LD_PRELOAD`.


   Compilation:
     cc -O3 -Wall -Wextra -fPIC -shared \
        -o moreless.so moreless.c

     For debugging:
       define -D_DEBUG
     To disable buffering in pipes:
       define -D IMMID_PIPE

   Known issues:
     - The ps command exits with exit code 1
       (probably because of it's unwanted child)
     - Redirecting 2>&1 does not work
 **/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>

#ifndef LESS
#  define LESS "less"
#endif

#ifndef _DEFAULT_EXCLUDES
#  define _DEFAULT_EXCLUDES \
  "less:man" \
  ":cp:mv:dd:rm:rmdir:chmod:chown:sudo" \
  ":tmux:screen" \
  ":vi:vim:nvim:nano:hexedit" \
  ":mpv:mplayer"
#endif

static const char *default_excludes = _DEFAULT_EXCLUDES;

#undef UNUSED
#define UNUSED(x) (void)(x)

/* Annotations */
#define __Parent__
#define __Child__

enum parent_t
  {
    P_CHILD = 0,
    P_PARENT =1,
    P_ESCAPED
  };

/* These functions call the real main function */
typedef int(*pre_main_t)(int argc, char **argv, char **envp);

/* Belongs to the actual binary (parent) */
static __Parent__ pre_main_t original_main;


static inline void
safe_fclose (FILE *f)
{
  int fd;
  if (f)
    {
      if ((fd = fileno (f)) < 0)
        return;
      if (fcntl (fd, F_GETFD) < 0)
        return;
      fflush (f);
      fclose (f);
    }
}

__Parent__ void
__attribute__((constructor)) init()
{
}

__Parent__ void
__attribute__((destructor)) cleanup()
{
  safe_fclose (stdout);
  safe_fclose (stderr);
  wait (NULL);
}

__Child__ int
alter_main (int argc, char **argv, char **envp)
{
  UNUSED (argc);
  UNUSED (argv);
  UNUSED (envp);

#ifdef _DEBUG
  int c;
  puts ("===============");
  while ((c = getc (stdin)) > 0)
    {
      printf ("%c", (c > 0) ? c : '?');
    }
  puts ("======EOF======\n");
  return 0;
#else /* _DEBUG */

  const char *less = LESS;
  /**
   *  DO *NOT* DELETE ME
   *  This will prevent recursive moreless
   *  Without unsetenv, moreless will be injected again
   *  in the less command and will call itself recursively
   *
   *  TODO: This must only delete /path/to/moreless.so
   *        not the whole of the env variable.
   */
  unsetenv ("LD_PRELOAD");

  // TODO: provide a way to pass less options
  int ret = execlp (less, less, "-S", NULL);
  if (ret < 0)
    {
      fprintf (stderr, LESS" itself failed.\n");
      return -ret;
    }

#endif /* _DEBUG */

  return -1; /* unreachable */
}

/**
 *  The `strchrnull` function is like `strchr` except  that
 *  if @c is not found in @s, then it returns a pointer to
 *  the null byte at the end of @s, rather than NULL.
 */
static inline const char *
strchrnull (const char *s, int c)
{
  for (int chr = *s; chr != 0; chr = *(++s))
    {
      if (chr == c)
        return s;
    }
  return s; /* s[0] must be equal to 0 */
}

/**
 *  Checks if the specified string @needle is found
 *  in the colon-separated string @haystack
 */
static inline int
excludestr (const char *restrict haystack,
            const char *restrict needle)
{
  const char *p = haystack;
  for (ssize_t n = 0;
       haystack != NULL && *p != 0;
       haystack += n+1)
    {
      p = strchrnull (haystack, ':');
      n = p - haystack;
      if (n > 0 &&
          0 == strncmp (needle, haystack, n))
        {
          return 1;
        }
    }
  return 0;
}

__Parent__ __Child__ int
main_hook (int argc, char **argv, char **envp)
{
  pid_t pid;
  int pipefd[2];
  int mode = P_CHILD;
  const char *cmd = argv[0];

  /**
   *  Exclude commands that need tty and
   *  included in MORELESS_EXCLUDE environment variable
   *  If MORELESS_EXCLUDE starts with `:`, then
   *  also exclude the default_excludes
   */
  const char *excludes = getenv ("MORELESS_EXCLUDE");

  if (!excludes)
    excludes = default_excludes;
  if (*excludes == ':')
    {
      if (excludestr (excludes, cmd) ||
          excludestr (default_excludes, cmd))
        {
        __do_escape:
          {
            mode = P_ESCAPED;
            unsetenv ("LD_PRELOAD");
            goto __original_main;
          }
        }
    }
  else if (excludestr (excludes, cmd))
    goto __do_escape;


  /**
   *  When stdout is not a tty, there is a pipe already
   *  so we should not affect moreless there
   */
  if (!isatty (STDOUT_FILENO))
    {
      mode = P_PARENT;
      goto __original_main;
    }

  /* Create pipe and do fork */
  if (pipe (pipefd) < 0)
    {
      perror ("pipe");
      return EXIT_FAILURE;
    }
  if ((pid = fork ()) < 0)
    {
      perror ("fork");
      return EXIT_FAILURE;
    }

  if (pid == 0) __Child__  /* Child process */
    {
      close (pipefd[1]);
      dup2 (pipefd[0], STDIN_FILENO);
      close (pipefd[0]);
    }
  else __Parent__ /* Parent process */
    {
      close (pipefd[0]);
      dup2 (pipefd[1], STDOUT_FILENO);
      close (pipefd[1]);
      mode = P_PARENT;

#ifdef IMMID_PIPE
      setvbuf (stdout, NULL, _IONBF, 0);
#endif
    }

  if (mode) __Parent__
    {
    __original_main:
#ifdef _DEBUG
      if (mode == P_ESCAPED)
        fprintf (stderr, "moreless[escaped] -> %s\n", *argv);
      else
        fprintf (stderr, "moreless[parent] --> %s\n", *argv);
#endif
      /* Continue to the real main function */
      return original_main (argc, argv, envp);
    }
  else __Child__
    {
#ifdef _DEBUG
      fprintf (stderr, "moreless[child] less %s\n", *argv);
#endif
      /* The less command lives here */
      return alter_main (argc, argv, envp);
    }
}

/**
 *  Libc-dependent function name
 *
 *  This section should overwrite the function
 *  that calls the actual main function.
 *
 *  It should initialize the global variable `original_main`
 *  with the address of the actual main function and
 *  then execute it's original (super) function, which
 *  can be found this way: `dlsym(RTLD_NEXT, "func_name")`.
 */
#if defined (__GLIBC__) /* GNU Libc (glibc) */
/**
 *  This code is stolen from:
 *   <https://gist.github.com/apsun/1e144bf7639b22ff0097171fa0f8c6b1>
 *
 *  Wrapper for __libc_start_main() that replaces the real main
 *  function with our hooked version.
 */
__Parent__ __Child__ int
__libc_start_main (
    pre_main_t main, // the real main function
    int argc, char **argv,
    pre_main_t init,
    void (* fini)(void),
    void (* rtld_fini)(void),
    void *stack_end)
{
    /* Save the real main function address */
    original_main = main;

    /* Find the real __libc_start_main()... */
    typeof (&__libc_start_main) super =
      dlsym (RTLD_NEXT, "__libc_start_main");
    /* ... and call it with our custom main function */
    return super (main_hook, argc, argv,
                  init, fini, rtld_fini, stack_end);
}
#else
#  error "Your libc is not supported!"
// TODO: support them
#endif
