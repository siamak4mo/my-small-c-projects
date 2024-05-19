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
 *  file: codeM_py.c
 *  created on: 19 May 2024
 *
 *  Implement Python C API extension for the codeM.c library
 *
 *
 *  compilation:
 *    cc -Wall -Wextra -shared -fPIC codeM_py.c -o codeM.so
 *       -I /path/to/Python.h
 *    you can find path of Python.h by running
 *    `sysconfig.get_paths()` in any python shell,
 *    normally it's located in `/usr/include/python3.xx`
 *
 *  usage in python environments:
 *    import codeM
 *    help(codeM)
 **/
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <time.h>

#define CODEM_IMPLEMENTATION
#define CODEM_FUZZY_SEARCH_CITYNAME
#include "codeM.c"

/* internal macros */
#define UNUSED(arg) (void)(arg)
#define py_mkbuf_H(res, len, inp) PyByteArray_AS_STRING (( \
         res = PyByteArray_FromStringAndSize(inp, len)))

#ifndef PYCODEMDEF
#  define PYCODEMDEF static PyObject *
#endif

/* noise for random number generator function */
static size_t noise = 0;

/* internal function definitions */
static size_t ssrand (void);
/* external python methods */
//PYCODEMDEF brnd(PyObject *self, PyObject *args);
PYCODEMDEF py_rand2(PyObject *self, PyObject *args);
PYCODEMDEF py_rand(PyObject *self, PyObject *args);
PYCODEMDEF py_rand_suffix (PyObject *self, PyObject *args);
PYCODEMDEF py_validate (PyObject *self, PyObject *args);
PYCODEMDEF py_mkvalid (PyObject *self, PyObject *args);
PYCODEMDEF py_rand_ccode (PyObject *self, PyObject *args);
PYCODEMDEF py_cname_by_code (PyObject *self, PyObject *args);

static struct PyMethodDef funs[] = {
  {
    "rand2", py_rand2,
    METH_VARARGS,
    "create random codem"
  },{
    "rand", py_rand,
    METH_VARARGS,
    "like rand2, but city code might be invalid"
  },{
    "rand_suffix", py_rand_suffix,
    METH_VARARGS,
    "random codem with suffix"
  },{
    "rand_ccode", py_rand_ccode,
    METH_VARARGS,
    "make random city code"
  },{
    "validate", py_validate,
    METH_VARARGS,
    "validate the input, ignore city code"
  },{
    "mkvalid", py_mkvalid,
    METH_VARARGS,
    "make the input valid"
  },{
    "cname_by_ccode", py_cname_by_code,
    METH_VARARGS,
    "get city name by city code"
  },
  {NULL, NULL, 0, NULL}
};

/* the final python module definition */
struct PyModuleDef codeM_def = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "codeM",
  .m_doc = "__there_is_nothing_here_yet!__",
  .m_size = -1,
  .m_methods = funs,
  .m_traverse = NULL,
  .m_clear = NULL,
  .m_free = NULL
};


PYCODEMDEF
py_rand2 (PyObject *self, PyObject *args)
{
  UNUSED (self);
  UNUSED (args);

  PyObject *result;
  char *result_ptr = py_mkbuf_H (result, CODEM_LEN, NULL);

  codem_rand2 (result_ptr);

  return result;
}

PYCODEMDEF
py_rand (PyObject *self, PyObject *args)
{
  UNUSED (self);
  UNUSED (args);

  PyObject *result;
  char *result_ptr = py_mkbuf_H (result, CODEM_LEN, NULL);

  codem_rand (result_ptr);

  return result;
}

PYCODEMDEF
py_rand_suffix (PyObject *self, PyObject *args)
{
  UNUSED (self);

  const char *suffix;
  size_t offset;

  if (!PyArg_ParseTuple(args, "s#", &suffix, &offset))
    Py_RETURN_NONE;
  if (offset > CODEM_LEN)
    offset = CODEM_LEN;

  PyObject *result;
  char *result_ptr = py_mkbuf_H (result, CODEM_LEN, suffix);

  codem_rands (result_ptr, offset);

  return result;
}

PYCODEMDEF
py_validate (PyObject *self, PyObject *args)
{
  UNUSED (self);

  const char *code;
  size_t len;

  if (!PyArg_ParseTuple(args, "s#", &code, &len))
    Py_RETURN_NONE;

  if (len != CODEM_LEN)
    Py_RETURN_FALSE;

  return (codem_isvalidn (code)) ? Py_NewRef(Py_True)
    : Py_NewRef(Py_False);
}

PYCODEMDEF
py_mkvalid (PyObject *self, PyObject *args)
{
  UNUSED (self);

  const char *code;
  size_t len;

  if (!PyArg_ParseTuple(args, "s#", &code, &len))
    Py_RETURN_NONE;

  PyObject *result;
  char *result_ptr = py_mkbuf_H (result, CODEM_LEN, code);

  codem_norm (result_ptr);
  codem_set_ctrl_digit (result_ptr);

  return result;
}

PYCODEMDEF
py_rand_ccode (PyObject *self, PyObject *args)
{
  UNUSED (self);
  UNUSED (args);

  PyObject *result;
  char *result_ptr = py_mkbuf_H (result, CC_LEN, NULL);

  codem_rand_ccode (result_ptr);
  return result;
}

PYCODEMDEF
py_cname_by_code (PyObject *self, PyObject *args)
{
  UNUSED (self);

  const char *code;
  size_t len;

  if (!PyArg_ParseTuple(args, "s#", &code, &len))
    Py_RETURN_NONE;

  if (len != CC_LEN)
    Py_RETURN_NONE;

  const char *p = codem_cname (code);

  return PyByteArray_FromStringAndSize (p, strlen (p));
}

static size_t
ssrand ()
{
  size_t r = time (NULL) + noise++;

  for (int i=7; i>0; --i)
    {
      r *= 0x424242;
      r += 0x666666;
    }
  return r;
}

PyMODINIT_FUNC
PyInit_codeM()
{
  /* internal codeM initialization */
  codem_rand_init (ssrand);

  return PyModule_Create(&codeM_def);
}