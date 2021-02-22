# directories
# GTKBASE= F:/Appli/msys64/mingw32
# on ne depend plus d'un F: absolu, mais on doit compiler avec le shell mingw32
GTKBASE= /mingw32

# choix de port audio I/O (laisser en blanc pour desactiver)
#AUDIOPORT = PORTAUDIO
AUDIOPORT = 


ifeq ($(AUDIOPORT), PORTAUDIO)
     ADEF = -DUSE_PORTAUDIO -DPA_USE_ASIO
     ADEV = pa_devs.c
     # winmm et ole32 requises par portaudio
     ALIB  = -lportaudio -lwinmm -lole32
     EXE = kawa.exe
else
     ADEF =
     ADEV =
     ALIB =
     EXE = kawnoa.exe
endif

# listes
SOURCESC = wav_head.c modpop2.c $(ADEV)
SOURCESCPP = layers.cpp gluplot.cpp jluplot.cpp spectro.cpp process.cpp gui.cpp param.cpp
HEADERS = glostru.h gluplot.h jluplot.h layers.h modpop2.h pa_devs.h process.h spectro.h wav_head.h param.h

OBJS= $(SOURCESC:.c=.o) $(SOURCESCPP:.cpp=.o)

# maintenir les libs et includes dans l'ordre alphabetique SVP

# LIBS= `pkg-config --libs gtk+-2.0`
LIBS= -L$(GTKBASE)/lib \
-latk-1.0 \
-lcairo \
-lgdk-win32-2.0 \
-lgdk_pixbuf-2.0 \
-lglib-2.0 \
-lgmodule-2.0 \
-lgobject-2.0 \
-lgtk-win32-2.0 \
-lpango-1.0 \
-lpangocairo-1.0 \
-lpangowin32-1.0 \
-L. -lfftw3f-3 \
$(ALIB)

# -mwindows
# enlever -mwindows pour avoir la console stdout


# options
# INCS= `pkg-config --cflags gtk+-2.0`
INCS= -Wall -Wno-parentheses -Wno-deprecated-declarations -O2 -mms-bitfields $(ADEF) \
-I$(GTKBASE)/include \
-I$(GTKBASE)/include/atk-1.0 \
-I$(GTKBASE)/include/cairo \
-I$(GTKBASE)/include/gdk-pixbuf-2.0 \
-I$(GTKBASE)/include/glib-2.0 \
-I$(GTKBASE)/include/gtk-2.0 \
-I$(GTKBASE)/include/pango-1.0 \
-I$(GTKBASE)/include/harfbuzz \
-I$(GTKBASE)/lib/glib-2.0/include \
-I$(GTKBASE)/lib/gtk-2.0/include \
# cibles

ALL : $(OBJS)
	g++ -o $(EXE) -s $(OBJS) $(LIBS)

clean : 
	rm *.o

.cpp.o: 
	g++ $(INCS) -c $<

.c.o: 
	gcc $(INCS) -c $<

# dependances
wav_head.o : ${HEADERS}
modpop2.o : ${HEADERS}
layers.o : ${HEADERS}
gluplot.o : ${HEADERS}
jluplot.o : ${HEADERS}
spectro.o : ${HEADERS}
process.o : ${HEADERS}
gui.o : ${HEADERS}
