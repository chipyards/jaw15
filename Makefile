# directories
# GTKBASE= F:/Appli/msys64/mingw32
# on ne depend plus d'un F: absolu, mais on doit compiler avec le shell mingw32
GTKBASE= /mingw32

# choix de port audio I/O (laisser en blanc pour desactiver)
AUDIOPORT = PORTAUDIO
# AUDIOPORT = 


ifeq ($(AUDIOPORT), PORTAUDIO)
     ADEF = -DUSE_PORTAUDIO -DPA_USE_ASIO
     # pa_devs est un utilitaire local, une couche sur portaudio
     ADEV = pa_devs.c
     # winmm et ole32 requises par portaudio
     ALIB  = -lportaudio_2011 -lwinmm -lole32
     EXE = kawa.exe
else
     ADEF =
     ADEV =
     ALIB =
     EXE = kawnoa.exe
endif

# listes
SOURCESC = modpop3.c $(ADEV)
SOURCESCPP = JLUP/gluplot.cpp JLUP/jluplot.cpp JLUP/layer_rgb.cpp \
    spectro.cpp process.cpp gui.cpp param.cpp wavio.cpp mp3in.cpp MIDI/fluid.cpp MIDI/song.cpp
HEADERS = JLUP/gluplot.h JLUP/jluplot.h \
    JLUP/layer_rgb.h JLUP/layer_lod.h JLUP/layer_u.h JLUP/strip_x_midi.h \
    modpop3.h pa_devs.h process.h gui.h cli_parse.h spectro.h autobuf.h param.h audiofile.h wavio.h mp3in.h \
    MIDI/fluid.h MIDI/midirender.h MIDI/midi_event.h MIDI/song.h

OBJS= $(SOURCESC:.c=.o) gluplot.o jluplot.o \
    layer_rgb.o \
    spectro.o process.o gui.o param.o wavio.o mp3in.o fluid.o song.o

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
-lmpg123 \
-lfluidsynth \
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
-I$(GTKBASE)/include/harfbuzz \
-I$(GTKBASE)/include/pango-1.0 \
-I$(GTKBASE)/lib/glib-2.0/include \
-I$(GTKBASE)/lib/gtk-2.0/include \
# cibles

ALL : $(OBJS)
	g++ -o $(EXE) -s $(OBJS) $(LIBS)

clean :
	rm *.o

.c.o:
	gcc $(INCS) -c $<

gluplot.o : JLUP/gluplot.cpp ${HEADERS}
	gcc $(INCS) -c JLUP/gluplot.cpp
jluplot.o : JLUP/jluplot.cpp ${HEADERS}
	gcc $(INCS) -c JLUP/jluplot.cpp
layer_rgb.o : JLUP/layer_rgb.cpp ${HEADERS}
	gcc $(INCS) -c JLUP/layer_rgb.cpp
spectro.o : spectro.cpp ${HEADERS}
	gcc $(INCS) -c spectro.cpp
process.o : process.cpp ${HEADERS}
	gcc $(INCS) -c process.cpp
gui.o : gui.cpp ${HEADERS}
	gcc $(INCS) -c gui.cpp
param.o : param.cpp ${HEADERS}
	gcc $(INCS) -c param.cpp
mp3in.o : mp3in.cpp ${HEADERS}
	gcc $(INCS) -c mp3in.cpp
fluid.o : MIDI/fluid.cpp ${HEADERS}
	gcc $(INCS) -c MIDI/fluid.cpp
song.o : MIDI/fluid.cpp ${HEADERS}
	gcc $(INCS) -c MIDI/song.cpp
# dependances
modpop3.o : ${HEADERS}
