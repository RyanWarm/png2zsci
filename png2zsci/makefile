ZLIBINC = zlib
ZLIBLIB = zlib
PNGINC  = libpng
PNGLIB  = libpng
JPEGINC = libjpeg
JPEGLIB = libjpeg/.libs

CC = g++
LD = $(CC)

LIBS = -lpng -lz -ljpeg

CRELEASE = -O3
LDRELEASE = -s
CFLAGS = -static -W -Wall $(CRELEASE)
LDFLAGS = $(LDRELEASE)

RM_F = rm -f

O=.o
A=.a

OBJS = png2zsci$(O) \
       alphacompress$(O)

.cpp$(O):
	$(CC) -c $(CFLAGS) -I$(ZLIBINC) -I$(PNGINC) -I$(JPEGINC) $<

png2zsci: png2zsci$(O) alphacompress$(O)
	$(LD) $(LDFLAGS) -L$(ZLIBLIB) -L$(PNGLIB) -L$(JPEGLIB) -o $@ png2zsci$(O) alphacompress$(O) $(LIBS)


clean:
	$(RM_F) *$(O) png2zsci




