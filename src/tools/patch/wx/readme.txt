This folder contains configuration files and patches for wxWidgets library:

  debughlp.h       - takes into account global configuration to compile out wxDbgHelpDLL
                     class which cannot be compiled for IA64 platform at the moment
  filefn.cpp       - defines HAVE_WGETCWD to make wxGetCwd() Unicode friendly
  languageinfo.cpp - language identifiers adjusted, for compatibility with transifex
  menu.cpp         - partially fixes wxMenu::Break functionality broken in wxWidgets 3.1.0
  setup.h          - configures wxWidgets for use with UltraDefrag
  statusbar.cpp    - adjusts status bar height on Windows XP to center text vertically inside

Don't forget to recompile the library whenever they get changed.
