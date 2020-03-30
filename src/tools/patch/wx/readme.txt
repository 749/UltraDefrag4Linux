This folder contains configuration files and patches for wxWidgets library:

  debughlp.h    - takes into account global configuration to compile out wxDbgHelpDLL
                  class which cannot be compiled for IA64 platform at the moment
  filefn.cpp    - defines HAVE_WGETCWD to make wxGetCwd() Unicode friendly
  setup.h       - configures wxWidgets for use with UltraDefrag

Don't forget to recompile the library whenever they get changed.
