#---------------------------------------------------------------------
# PyMOL Makefile Rules 
#---------------------------------------------------------------------
#
#- Choose One Set ----------------------------------------------------
#--- Build for unix as an embedded Python interprete
#XLIB_DIR = -L/usr/X11R6/lib 
#LIBS = -lpython1.5 -ltk8.0 -ltcl8.0 -lglut -lGL -lGLU -ldl -lX11 -lXext -lXmu -lXi -lpng $(ZLIB) -lm
#MODULE = 
#BUILD = -o pymol.exe
#--- Build for unix as an importable module
XLIB_DIR = -L/usr/X11R6/lib
LIBS = -lpython1.5 -lglut -lGL -lGLU -ldl -lpng -lXmu $(ZLIB) -lm
MODULE = -D_PYMOL_MODULE
BUILD = -shared -o modules/_pm.so
#--- Build for Windows as an importable module
#XLIB_DIR = 
#LIBS = -lpython15 -lopengl32 -lglu32 -lglut32 -lpng -lz
#MODULE = -D_PYMOL_MODULE
#BUILD = -o modules/_pm.pyd
#---------------------------------------------------------------------
#
#- Choose One --------------------------------------------------------
#--- Workaround for XFree86/DRI linux dll problem for module build
BUGS = -D_DRI_WORKAROUND
#--- Running under windows (perhaps the biggest bug of all)
#BUGS = -D_PYMOL_WINDOWS -DWIN32 -D_WIN32
#---
#BUGS =
#---------------------------------------------------------------------
#
#- Choose One --------------------------------------------------------
#--- Gcc under Linux or Windows
CCOPT1 = -m486 -D__i686__ -ffast-math -Wall -ansi -Wmissing-prototypes
#--- SGI Irix 6.x
#CCOPT1 = -ansi -n32 -woff 1429,1204
#---------------------------------------------------------------------
#
#- Choose One --------------------------------------------------------
#--- GCC Optimized
#CCOPT2 = -O3 -funroll-loops -fomit-frame-pointer
#--- GCC Profiling
#CCOPT2 = -pg -O3 -funroll-loops
#--- Irix CC Optimized
#CCOPT2 = -O2
#--- Debugging
CCOPT2 = -g
#---------------------------------------------------------------------
#
#- Choose One Pair ---------------------------------------------------
#--- Libpng2 in system libraries (/usr/include,/usr/lib)
#PNG = -D_HAVE_LIBPNG 
#ZLIB = 
#--- Libpng2 in local user files (pymol/ext/...)
PNG = -D_HAVE_LIBPNG 
ZLIB = -lz
#--- Libpng2 not available
#PNG = 
#ZLIB = 
#---------------------------------------------------------------------
#
#---------------------------------------------------------------------
# No changes normally required below here
#---------------------------------------------------------------------

CFLAGS = $(CCOPT1) $(CCOPT2) -D_PYMOL_THREADS $(INC_DIRS) $(PNG) $(MODULE) $(BUGS)

CC = cc

LIB_DIRS = -L./ext/lib $(XLIB_DIR)

INC_DIRS = -I../layer0 -I../layer1 -I../layer2 \
	-I../layer3 -I../layer4 -I../layer5 \
	-I../ext/include -I../ext/include/python1.5













