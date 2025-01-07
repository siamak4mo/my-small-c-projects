/**
 *  file: mini-lexer.c
 *  created on: 16 Dec 2024
 *
 *  Minimal Lexer header-only library
 *  --Under Development--
 *
 */
#ifndef MINI_LEXER__H
#define MINI_LEXER__H

#include <string.h>
#include <stddef.h>


#ifdef _ML_DEBUG
#  include <stdio.h>
#  define fprintd(format, ...) \
  fprintf (stderr, format, ##__VA_ARGS__)
#  define logf(format, ...) \
  fprintd ("%s:%d: "format"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#  define fprintd(format, ...)
#  define logf(format, ...)
#endif

typedef struct Milexer_exp_
{
  const char *begin;
  const char *end;

  // int __tag; // internal nesting
} _exp_t;

/**
 *  Basic expression
 **/
typedef struct
{
  int len;
  const char **exp;
} Milexer_BEXP;

/**
 *  Advanced expression
 */
typedef struct
{
  int len;
  struct Milexer_exp_ *exp;
} Milexer_AEXP;


enum __buffer_state_t
  {
    /* space, \t, \r, \0, ... */
    SYN_DUMMY = 0,
    
    /* only backslah escape */
    SYN_ESCAPE,
    
    /* middle or beggining of a token */
    SYN_MIDDLE,

    /* when need to recover the previous puck */
    SYN_PUNC__,
    
    /* middle or beggining of a string or comment
       in this mode, nothing causes change of state
       but the specified end of expression */
    SYN_NO_DUMMY,
    SYN_NO_DUMMY__, /* when the prefix needs to be recovered */
    
    SYN_CHUNK, /* handling fragmentation */
    SYN_DONE /* token is ready */
  };

const char *milexer_buf_state[] = {
  [SYN_DUMMY]                   = "dummy",
  [SYN_ESCAPE]                  = "escape",
  [SYN_MIDDLE]                  = "inner token",
  [SYN_NO_DUMMY]                = "inner exp",
  [SYN_NO_DUMMY__]              = "recover exp",
  [SYN_PUNC__]                  = "recover punc",
  [SYN_DONE]                    = "done",
};

enum milexer_state_t
  {
    NEXT_MATCH = 0, /* got a token */
    
    /* no enough space left in the token's buffer
       keep reading the rest of the chunks */
    NEXT_CHUNK,
    
    /* parser has encountered a zero-byte \0
       you might want to end it up */
    NEXT_ZTERM,
    
    /* only in lazy loading
       you need to load the rest of your stuff */
    NEXT_NEED_LOAD,
    
    NEXT_END, /* nothing to do, end of parsing */

    /* either your buffer(s) is null or
       input buffer and output token buffer are the same */
    NEXT_ERR,
  };

#define NEXT_SHOULD_END(ret) (ret == NEXT_END || ret == NEXT_ERR)
#define NEXT_SHOULD_LOAD(ret) (ret == NEXT_NEED_LOAD || NEXT_SHOULD_END (ret))


#define __flag__(n) (1 << (n))
enum milexer_parsing_flag_t
  {
    /* Default behavior */
    PFLAG_DEFAULT = 0,

    /**
     *  To Retrieve the contents of expressions 
     *  without their prefix and suffix.
     */
    PFLAG_INEXP =      __flag__ (0),

    /**
     *  To allow space(s) in tokens
     *  The whitespace (0x20) character is no longer
     *  treated as a delimiter while using this flag
     */
    PFLAG_IGSPACE =    __flag__ (1),

    /**
     *  When `delim_ranges` is available, it will overwrite 
     *  the default delimiters (range 0, 0x20)
     *  To include them as well, use this flag
     */
    PFLAG_ALLDELIMS =  __flag__ (1),
  };


const char *milexer_state_cstr[] = {
  [NEXT_MATCH]                   = "Match",
  [NEXT_CHUNK]                   = "Chunk",
  [NEXT_ZTERM]                   = "zero-byte",
  [NEXT_NEED_LOAD]               = "Load",
  [NEXT_END]                     = "END",
  [NEXT_ERR]                     = "Error"
};

enum milexer_token_t
  {
    /* internal bug */
    TK_NOT_SET = 0,

    TK_PUNCS,
    TK_KEYWORD,

    /**
     *  An expression might be anything,
     *  strings like: "xxx" or constructs like: (xxx) or {xxx}
     */
    TK_EXPRESSION,

    /* basic comments (single line) */
    TK_BCOMMENT,

    /* advanced comments (multi-line comments) */
    TK_ACOMMENT,
  };

const char *milexer_token_type_cstr[] = {
  [TK_NOT_SET]                        = "NAN", /* unreachable */
  [TK_PUNCS]                          = "Punctuation",
  [TK_KEYWORD]                        = "Keyword",
  [TK_EXPRESSION]                     = "Expression",
  [TK_BCOMMENT]                       = "Comment",
  [TK_ACOMMENT]                       = "Comment",
};

typedef struct
{
  enum milexer_token_t type;

  /**
   *  Index of the token in the corresponding
   *  Milexer configuration field, punc/keyword/...
   *  -1 means the token is not recognized
   */
  int id;

  /**
   *  The user of this library is responsible for allocating
   *  and freeing the buffer @cstr of length `@len + 1`
   *  `TOKEN_ALLOC` macro does this using malloc
   *  We guarantee that @cstr is always null-terminated
   */
  char *cstr;
  size_t len;

  /* Internal */
  size_t __idx;
} Milexer_Token;

#define TOKEN_IS_KNOWN(t) ((t)->id >= 0)
#define TOKEN_ALLOC(n) (Milexer_Token){.cstr=malloc (n+1), .len=n}
#define TOKEN_FREE(t) if ((t)->cstr) {free ((t)->cstr);}
#define TOKEN_DROP(t) ((t)->__idx = 0, (t)->type = TK_NOT_SET)

/* Internal macros */
#define __get_last_exp(ml, src) \
  ((ml)->expression.exp + (src)->__last_exp_idx)
#define __get_last_punc(ml, src) \
  ((ml)->puncs.exp[(src)->__last_punc_idx])


typedef struct
{
  /* End of lazy loading */
  int eof_lazy;

  /* Internal state of the buffer, SYN_xxx */
  enum __buffer_state_t state, prev_state;

  /* buffer & index & capacity */
  const char *buffer;
  size_t cap, idx;

  /* Internal */
  /* TODO: for nesting, these should be arrays */
  int __last_exp_idx;
  int __last_punc_idx;
} Milexer_Slice;

#define SET_SLICE(src, buf, n) ((src)->buffer = buf, (src)->cap = n)
/* to indicate that lazy loading is over */
#define END_SLICE(src) ((src)->cap = 0, (src)->eof_lazy = 1)

typedef struct Milexer_t
{
  /* lazy mode */
  int lazy;

  /* Configurations */
  Milexer_BEXP escape;    // Not implemented
  Milexer_BEXP puncs;
  Milexer_BEXP keywords;
  Milexer_AEXP expression;
  Milexer_BEXP b_comment; // Not implemented
  Milexer_AEXP a_comment; // Not implemented
  /**
   *  Delimiter ranges; each entry in this field
   *  must be 1 or 2-byte string, which defines a range
   *  of characters that will be treated as token delimiters
   *  For example: "\x01\x12" describes the range [0x01, 0x12],
   *  while "\x42" represents a single character 0x42
   */
  Milexer_BEXP delim_ranges;

  /**
   *  Retrieves the next token
   *
   *  @src is the input buffer source and The result 
   *  will be stored in @t
   *
   *  Memory management for @src and @t is up to
   *  the user of this library
   *  Their allocated buffers *MUST* be distinct
   */
  int (*next)(const struct Milexer_t *,
              Milexer_Slice *src,
              Milexer_Token *t,
              int flags);
} Milexer;

#define __gen_lenof(arr) (sizeof (arr) / sizeof ((arr)[0]))
#define GEN_MKCFG(exp_ptr) {.exp=exp_ptr, .len=__gen_lenof (exp_ptr)}

/* initialize */
int milexer_init (Milexer *);


#ifdef ML_IMPLEMENTATION
/**
 **  Internal functions
 **  Only use Milexer.next()
 **/
static inline int
__handle_delims (const Milexer *ml, const Milexer_Slice *src,
                 unsigned char p, int flags)
{
  switch (src->state)
    {
    case SYN_ESCAPE:
    case SYN_NO_DUMMY:
    case SYN_NO_DUMMY__:
      return 0;

    default:
      if (ml->delim_ranges.len == 0 || (flags & PFLAG_ALLDELIMS))
        {
          /* default delimiters */
          if (p < ' ' ||
              (p == ' ' && !(flags & PFLAG_IGSPACE)))
            {
              return p;
            }
        }
      else
        {
          for (int i=0; i < ml->delim_ranges.len; ++i)
            {
              const char *__p = ml->delim_ranges.exp[i];
              if (__p[1] != '\0')
                {
                  if (p >= __p[0] && p <= __p[1])
                    return p;
                }
              else if (p == __p[0])
                return p;
            }
        }
    }
  return 0;
}

static inline char *
__handle_puncs (const Milexer *ml, Milexer_Slice *src,
                Milexer_Token *res)
{
  switch (src->state)
    {
    case SYN_ESCAPE:
    case SYN_NO_DUMMY:
    case SYN_NO_DUMMY__:
      return NULL;

    default:
      int longest_match_idx = -1;
      size_t longest_match_len = 0;
      for (int i=0; i < ml->puncs.len; ++i)
        {
          const char *punc = ml->puncs.exp[i];
          size_t len = strlen (punc);
          if (res->__idx < len)
            continue;
          char *p = res->cstr + res->__idx - len;
          if (strncmp (punc, p, len) == 0)
            {
              if (len >= longest_match_len)
                {
                  longest_match_len = len;
                  longest_match_idx = i;
                }
            }
        }
      if (longest_match_idx != -1)
        {
          src->__last_punc_idx = longest_match_idx;
          res->id = longest_match_idx;
          return res->cstr + (res->__idx - longest_match_len);
        }
      return NULL;
    }
}

static inline char *
__handle_expression (const Milexer *ml, Milexer_Slice *src,
                     Milexer_Token *res) 
{
#define Return(n) do {                          \
    if (n) {                                    \
      logf ("match, d[.%lu]={%.*s} @%s",        \
            res->__idx,                         \
            (int)res->__idx,                    \
            res->cstr,                          \
            milexer_buf_state[ml->state]);      \
    } return n;                                 \
  } while (0)

  /* strstr function works with \0 */
  res->cstr[res->__idx] = '\0';
  
  switch (src->state)
    {
    case SYN_ESCAPE:
      Return (res->cstr + res->__idx);

    case SYN_NO_DUMMY:
    case SYN_NO_DUMMY__:
      /* looking for closing, O(1) */
        {
          _exp_t *e = __get_last_exp (ml, src);
          size_t len = strlen (e->end);

          if (res->__idx < len)
            break;
          char *p = res->cstr + res->__idx - len;
          if (strncmp (p, e->end, len) == 0)
            {
              res->id = src->__last_exp_idx;
              Return (p);
            }
        }
       Return (NULL);
      break;

    default:
      /* looking for opening, O(ml->expression.len) */
      for (int i=0; i < ml->expression.len; ++i)
        {
          char *p;
          _exp_t *e = ml->expression.exp + i;
          if ((p = strstr (res->cstr, e->begin)))
            {
              if (p == res->cstr && src->state == SYN_MIDDLE)
                Return (NULL);
              src->__last_exp_idx = i;
              Return (p);
            }
        }
      Return (0);
      break;
    }

#undef Return
  return NULL;
}

static inline void
__handle_token_id (const Milexer *ml, Milexer_Token *res)
{
  if (res->type != TK_KEYWORD)
    return;
  if (ml->keywords.len > 0)
    {
      for (int i=0; i < ml->keywords.len; ++i)
        {
          const char *p = ml->keywords.exp[i];
          if (strcmp (p, res->cstr) == 0)
            {
              res->id = i;
              return;
            }
        }
      res->id = -1;
    }
}


static int
__next_token (const Milexer *ml, Milexer_Slice *src,
              Milexer_Token *res, int flags)
{
  (void)(ml);
  (void)(src);
  (void)(res);
  (void)(flags);
  return 0;
}

static int
__next_token_lazy (const Milexer *ml, Milexer_Slice *src,
                   Milexer_Token *res, int flags)
{
#define LD_STATE(src) (src)->state = (src)->prev_state
#define ST_STATE(slice, new_state) do {         \
    (slice)->prev_state = (slice)->state;       \
    (slice)->state = new_state;                 \
  } while (0)

  if (res->cstr == NULL || res->len <= 0
      || res->cstr == src->buffer)
    return NEXT_ERR;
  
  if (src->state == SYN_NO_DUMMY__)
    {
      if (!(flags & PFLAG_INEXP) && src->__last_exp_idx != -1)
        {
          /* certainly the token type is expression */
          res->type = TK_EXPRESSION;
          const char *le = __get_last_exp (ml, src)->begin;
          char *__p = mempcpy (res->cstr, le, strlen (le));
          res->__idx += __p - res->cstr;
        }
      src->state = SYN_NO_DUMMY;
    }
  else if (src->state == SYN_PUNC__)
    {
      const char *lp = __get_last_punc (ml, src);
      *((char *)mempcpy (res->cstr, lp, strlen (lp))) = '\0';

      LD_STATE (src);
      res->type = TK_PUNCS;
      res->id = src->__last_punc_idx;
      return NEXT_MATCH;
    }
    
  if (src->idx > src->cap)
    {
      if (src->eof_lazy)
        return NEXT_END;
      src->idx = 0;
      return NEXT_NEED_LOAD;
    }

  res->type = TK_NOT_SET;
  for (unsigned char p; src->idx < src->cap; )
    {
      char *__startof_exp, *__startof_punc;
      
      p = src->buffer[src->idx++];
      res->cstr[res->__idx++] = p;

      //-- handling delimiters ---------//
      if (__handle_delims (ml, src, p, flags))
        {
          /* normal token */
          if (res->__idx > 1)
            {
              ST_STATE (src, SYN_DUMMY);
              res->type = TK_KEYWORD;
              res->cstr[res->__idx - 1] = '\0';
              res->__idx = 0;
              __handle_token_id (ml, res);
              if (p == '\0')
                return NEXT_ZTERM;
              return NEXT_MATCH;
            }
          else
            {
              /* ignore empty stuff */
              res->__idx = 0;
            }
        }
      //-- handling expressions --------//
      if ((__startof_exp = __handle_expression (ml, src, res)))
        {
          res->type = TK_EXPRESSION;
          if (src->state == SYN_ESCAPE)
            {
              LD_STATE (src);
            }
          else if (src->state == SYN_NO_DUMMY)
            {
              if (flags & PFLAG_INEXP)
                *__startof_exp = '\0';
              ST_STATE (src, SYN_DONE);
              res->cstr[res->__idx] = 0;
              res->__idx = 0;
              return NEXT_MATCH;
            }
          else
            {
              /* begginign of an expression */
              if (__startof_exp == res->cstr)
                {
                  if (flags & PFLAG_INEXP)
                    res->__idx = 0;
                  ST_STATE (src, SYN_NO_DUMMY);
                }
              else
                {
                  /**
                   *  xxx`expression`yyy
                   *  the expression is adjacent to another token
                   */
                  ST_STATE (src, SYN_NO_DUMMY__);
                  *__startof_exp = 0;
                  res->__idx = 0;
                  res->type = TK_KEYWORD;
                  __handle_token_id (ml, res);
                  return NEXT_MATCH;
                }
            }
        }
      //-- handling escape -------------//
      else if (p == '\\')
        {
          ST_STATE (src, SYN_ESCAPE);
        }
      //-- handling puncs --------------//
      else if ((__startof_punc = __handle_puncs (ml, src, res)))
        {
          /* punc */
          const char *p = __get_last_punc (ml, src);
          size_t len = strlen (p);
          if (len == res->__idx)
            {
              /* just a simple punc */
              ST_STATE (src, SYN_DUMMY);
              res->type = TK_PUNCS;
              res->cstr[res->__idx] = '\0';
              res->__idx = 0;
              return NEXT_MATCH;
            }
          else
            {
              /* the punc have got some adjacents */
              ST_STATE (src, SYN_PUNC__);
              *__startof_punc = '\0';
              res->__idx = 0;
              res->type = TK_KEYWORD;
              __handle_token_id (ml, res);
              return NEXT_MATCH;
            }
        }
      //-- detect & reset chunks -------//
      if (res->__idx == res->len)
        {
          if (res->type == TK_NOT_SET ||
              src->state == SYN_DUMMY || src->state == SYN_DONE)
            {
              /**
               *  we assume your keywords are smaller than
               *  the length of res->cstr buffer
               */
              res->id = -1;
              res->type = TK_KEYWORD;
            }
          /* max token len reached */
          ST_STATE (src, SYN_CHUNK);
          res->cstr[res->__idx + 1] = '\0';
          res->__idx = 0;
          return NEXT_CHUNK;
        }
      if (src->state == SYN_CHUNK)
        LD_STATE (src);
      //--------------------------------//
    }

  if (src->eof_lazy)
    {
      if (res->__idx > 1)
        {
          if (res->type == TK_NOT_SET)
            {
              res->type = TK_KEYWORD;
              __handle_token_id (ml, res);
            }
          return NEXT_END;
        }
      else if (res->__idx == 1 && *res->cstr > ' ')
        {
          res->type = TK_KEYWORD;
          return NEXT_END;
        }
      else
        {
          res->type = TK_NOT_SET;
          return NEXT_END;
        }
    }
  src->idx = 0;
  return NEXT_NEED_LOAD;
}


int
milexer_init (Milexer *ml)
{
  if (ml->lazy)
    {
      /* lazy loading */
      ml->next = __next_token_lazy;
    }
  else
    {
      /**
       *  assume the entire data is loaded
       *  into ml->src->buffer
       */
      ml->next = __next_token;
    }

  return 0;
}

#endif /* ML_IMPLEMENTATION */

#undef logf
#undef fprintd
#endif /* MINI_LEXER__H */


/**
 **  Test1 Program
 **/
#ifdef ML_EXAMPLE_1
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

//-- The language ----------------------//
enum LANG
  {
    /* keywords */
    LANG_IF = 0,
    LANG_ELSE,
    LANG_FI,
    /* punctuations */
    PUNC_PLUS = 0,
    PUNC_MINUS,
    PUNC_MULT,
    PUNC_DIV,
    PUNC_COMMA,
    PUNC_EQUAL,
    PUNC_NEQUAL,
    /* expressions */
    EXP_PAREN = 0,
    EXP_BRACE,
    EXP_STR,
    EXP_STR2,
  };
static const char *Keys[] = {
  [LANG_IF]         = "if",
  [LANG_ELSE]       = "else",
  [LANG_FI]         = "fi",
};
static const char *Puncs[] = {
  [PUNC_PLUS]       = "+",
  [PUNC_MINUS]      = "-",
  [PUNC_MULT]       = "*",
  [PUNC_DIV]        = "/",
  [PUNC_COMMA]      = ",",
  [PUNC_EQUAL]      = "=", /* you cannot have "==" */
  [PUNC_NEQUAL]     = "!=", /* also "!===" */
};
static struct Milexer_exp_ Exp[] = {
  [EXP_PAREN]       = {"(", ")"},
  [EXP_BRACE]       = {"{", "}"},
  [EXP_STR]         = {"\"", "\""},
  [EXP_STR2]        = {"'", "'"},
};
static const char *puncs_cstr[] = {
  [PUNC_PLUS]       = "Plus",
  [PUNC_MINUS]      = "Minus",
  [PUNC_MULT]       = "Times",
  [PUNC_DIV]        = "Division",
  [PUNC_COMMA]      = "Comma",
  [PUNC_EQUAL]      = "Equal",
  [PUNC_NEQUAL]     = "~Equal",
};
static const char *exp_cstr[] = {
  [EXP_PAREN]       = "(*)",
  [EXP_BRACE]       = "{*}",
  [EXP_STR]         = "\"*\"",
  [EXP_STR2]        = "'*'",
};
//--------------------------------------//


int
main (void)
{
  /* input source */
  Milexer_Slice src = {0};
  /* token type */
  Milexer_Token tk = TOKEN_ALLOC (32);

  /* Milexer initialization */
  Milexer ml = {
    .lazy = 1,

    .puncs       = GEN_MKCFG (Puncs),
    .keywords    = GEN_MKCFG (Keys),
    .expression  = GEN_MKCFG (Exp),
  };
  milexer_init (&ml);

  char *line = NULL;
  const int flg = PFLAG_INEXP;
  for (int ret = 0; !NEXT_SHOULD_END (ret); )
    {
      /* Get the next token */
      ret = ml.next (&ml, &src, &tk, flg);
      switch (ret)
        {
        case NEXT_NEED_LOAD:
          ssize_t n;
          printf (">>> ");
          if ((n = getline (&line, (size_t *)&n, stdin)) < 0)
            END_SLICE (&src);
          else
            SET_SLICE (&src, line, n);
          break;

        case NEXT_MATCH:
        case NEXT_CHUNK:
        case NEXT_ZTERM:
          {
            /* print the type of the token @t */
            printf ("%.*s", 3, milexer_token_type_cstr[tk.type]);
            switch (tk.type)
              {
              case TK_KEYWORD:
                printf ("[%c]  `%s`", TOKEN_IS_KNOWN (&tk) ?'*':'-', tk.cstr);
                break;
              case TK_PUNCS:
                printf ("[*]   %s", puncs_cstr[tk.id]);
                break;
              case TK_EXPRESSION:
                printf ("%s", exp_cstr[tk.id]);
                if (tk.id != EXP_PAREN) /* is not parenthesis */
                  printf ("   `%s`", tk.cstr);
                else
                  {
                    /**
                     *  This example parses the contents of expressions
                     *  enclosed in parenthesis: given: '(xxx)' -> parsing 'xxx'
                     *  We use `t.cstr` as the input buffer in `second_src`
                     *  Therefore, we must allocate a new token, otherwise
                     *  the parser will fail to parse the input and store
                     *  the result in the same location simultaneously
                     */
                    puts (":");
                    Milexer_Slice second_src = {0};
                    Milexer_Token tmp = TOKEN_ALLOC (32);
                    for (;;)
                      {
                        /**
                         *  When the inner parentheses token is not a chunk,
                         *  the parser should not expect additional chunks
                         */
                        second_src.eof_lazy = (ret != NEXT_CHUNK);
                        /* prepare the new input source buffer */
                        SET_SLICE (&second_src, tk.cstr, strlen (tk.cstr));

                        for (int _ret = 0; !NEXT_SHOULD_LOAD (_ret); )
                          {
                            /* allow space character in tokens */
                            _ret = ml.next (&ml, &second_src, &tmp, PFLAG_IGSPACE);

                            if (tmp.type == TK_KEYWORD)
                              printf ("%s", tmp.cstr);
                            else if (tmp.type == TK_PUNCS && tmp.id == PUNC_COMMA)
                              puts ("");
                          }

                        /* load the remaining chunks of the inner parentheses, if any */
                        if (ret != NEXT_CHUNK)
                          break;
                        ret = ml.next (&ml, &src, &tk, flg);
                      }
                    TOKEN_FREE (&tmp);
                  }
                break;

              default: /* t.type */
                break;
              }

            /* detecting chunk */
            if (ret == NEXT_CHUNK)
              puts ("    <-- chunk");
            else
              puts ("");
          }
          break;
        }
    }

  TOKEN_FREE (&tk);
  if (line)
    free (line);
  puts ("Bye");

  return 0;
}

#endif /* ML_EXAMPLE_1 */
