/*
** Simple Lua interpreter with graphical interface.
** Based on Lua stand-alone interpreter source code.
** See Copyright Notice in ../lua5.1/lua.h
*/

/* Modified by dmitriar: interactive mode removed; MessageBox added */

#include <windows.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>

#define lua_c

#include "../lua5.1/lua.h"

#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"


static lua_State *globalL = NULL;

static const char *progname = LUA_PROGNAME;

static int silent_mode = 0;

static void lstop (lua_State *L, lua_Debug *ar) {
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);
  luaL_error(L, "interrupted!");
}


static void __cdecl laction (int i) {
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
                              terminate process (default action) */
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}


static void print_usage (void) {
  if(silent_mode) return;
  MessageBox(0,
  "usage: lua5.1a_gui [options] [script [args]].\n"
  "Available options are:\n"
  "  -e stat  execute string " LUA_QL("stat") "\n"
  "  -l name  require library " LUA_QL("name") "\n"
  "  -v       show version information\n"
  "  -s       silent mode (errors not displayed on the screen\n"
  "  --       stop handling options\n"
  ,
  "Lua GUI",MB_OK);
}


static void l_message (const char *pname, const char *msg) {
  if(silent_mode) return;
  if (!pname) pname = "Lua GUI";
  /*
  * MB_TOPMOST and MB_SETFOREGROUND flags are needed 
  * for UltraDefrag GUI options verification.
  */
  MessageBox(0,msg,pname,MB_OK | MB_TOPMOST | MB_SETFOREGROUND);
}


static int report (lua_State *L, int status) {
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message(progname, msg);
    lua_pop(L, 1);
  }
  return status;
}


static int traceback (lua_State *L) {
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}


static int docall (lua_State *L, int narg, int clear) {
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  signal(SIGINT, laction);
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  signal(SIGINT, SIG_DFL);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}


static void print_version (void) {
  l_message(NULL, LUA_RELEASE "  " LUA_COPYRIGHT);
}


static int getargs (lua_State *L, char **argv, int n) {
  int narg;
  int i;
  int argc = 0;
  while (argv[argc]) argc++;  /* count total number of arguments */
  narg = argc - (n + 1);  /* number of arguments to the script */
  luaL_checkstack(L, narg + 3, "too many arguments to script");
  for (i=n+1; i < argc; i++)
    lua_pushstring(L, argv[i]);
  lua_createtable(L, narg, n + 1);
  for (i=0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - n);
  }
  return narg;
}


static int dofile (lua_State *L, const char *name) {
  int status = luaL_loadfile(L, name) || docall(L, 0, 1);
  return report(L, status);
}


static int dostring (lua_State *L, const char *s, const char *name) {
  int status = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, status);
}


static int dolibrary (lua_State *L, const char *name) {
  lua_getglobal(L, "require");
  lua_pushstring(L, name);
  return report(L, lua_pcall(L, 1, 0, 0));
}


static int handle_script (lua_State *L, char **argv, int n) {
  int status;
  const char *fname;
  int narg = getargs(L, argv, n);  /* collect arguments */
  lua_setglobal(L, "arg");
  fname = argv[n];
  if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0) 
    fname = NULL;  /* stdin */
  status = luaL_loadfile(L, fname);
  lua_insert(L, -(narg+1));
  if (status == 0)
    status = docall(L, narg, 0);
  else
    lua_pop(L, narg);      
  return report(L, status);
}


/* check that argument has no extra characters at the end */
#define notail(x)    {if ((x)[2] != '\0') return -1;}


static int collectargs (char **argv, int *pi, int *pv, int *pe) {
  int i;
  for (i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] != '-')  /* not an option? */
        return i;
    switch (argv[i][1]) {  /* option */
      case '-':
        notail(argv[i]);
        return (argv[i+1] != NULL ? i+1 : 0);
      case '\0':
        return i;
      case 'i':
        notail(argv[i]);
        *pi = 1;  /* go through */
      case 'v':
        notail(argv[i]);
        *pv = 1;
        break;
      case 's':
      case 'S':
        notail(argv[i]);
        break;
      case 'e':
        *pe = 1;  /* go through */
      case 'l':
        if (argv[i][2] == '\0') {
          i++;
          if (argv[i] == NULL) return -1;
        }
        break;
      default: return -1;  /* invalid option */
    }
  }
  return 0;
}


static int runargs (lua_State *L, char **argv, int n) {
  int i;
  for (i = 1; i < n; i++) {
    if (argv[i] == NULL) continue;
    lua_assert(argv[i][0] == '-');
    switch (argv[i][1]) {  /* option */
      case 'e': {
        const char *chunk = argv[i] + 2;
        if (*chunk == '\0') chunk = argv[++i];
        lua_assert(chunk != NULL);
        if (dostring(L, chunk, "=(command line)") != 0)
          return 1;
        break;
      }
      case 'l': {
        const char *filename = argv[i] + 2;
        if (*filename == '\0') filename = argv[++i];
        lua_assert(filename != NULL);
        if (dolibrary(L, filename))
          return 1;  /* stop if file fails */
        break;
      }
      default: break;
    }
  }
  return 0;
}


static int handle_luainit (lua_State *L) {
  const char *init = getenv(LUA_INIT);
  if (init == NULL) return 0;  /* status OK */
  else if (init[0] == '@')
    return dofile(L, init+1);
  else
    return dostring(L, init, "=" LUA_INIT);
}


struct Smain {
  int argc;
  char **argv;
  int status;
};


static int pmain (lua_State *L) {
  struct Smain *s = (struct Smain *)lua_touserdata(L, 1);
  char **argv = s->argv;
  int script;
  int has_i = 0, has_v = 0, has_e = 0;
  globalL = L;
  if (argv[0] && argv[0][0]) progname = argv[0];
  lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(L);  /* open libraries */
  lua_gc(L, LUA_GCRESTART, 0);
  s->status = handle_luainit(L);
  if (s->status != 0) return 0;
  script = collectargs(argv, &has_i, &has_v, &has_e);
  if (script < 0) {  /* invalid args? */
    print_usage();
    s->status = 1;
    return 0;
  }
  if (has_v) print_version();
  s->status = runargs(L, argv, (script > 0) ? script : s->argc);
  if (s->status != 0) return 0;
  if (script)
    s->status = handle_script(L, argv, script);
  if (s->status != 0) return 0;
  return 0;
}


BOOL CALLBACK EmptyDlgProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg){
  case WM_INITDIALOG:
    /* kill our window before showing them :) */
    (void)EndDialog(hWnd,1);
    return FALSE;
  case WM_CLOSE:
    /* this code - for extraordinary cases */
    (void)EndDialog(hWnd,1);
     return TRUE;
  }
  return FALSE;
}

int __cdecl main (int argc, char **argv) {
  int i, status;
  struct Smain s;
  lua_State *L = NULL;
  
  /* strongly required! to be compatible with manifest */
  InitCommonControls();

  /*
  * To disable sand glass on the cursor
  * we must show a window on startup.
  */
  (void)DialogBox((HINSTANCE)GetModuleHandle(NULL),
    MAKEINTRESOURCE(100),NULL,(DLGPROC)EmptyDlgProc);

  /* check for silent mode */
  for(i = 0; i < argc; i++){
      if(!strcmp(argv[i],"-s") || !strcmp(argv[i],"-S"))
          silent_mode = 1;
  }
  
  L = lua_open();  /* create state */
  if (L == NULL) {
    l_message(argv[0], "cannot create state: not enough memory");
    return EXIT_FAILURE;
  }
  s.argc = argc;
  s.argv = argv;
  status = lua_cpcall(L, &pmain, &s);
  report(L, status);
  lua_close(L);
  return (status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS;
}
