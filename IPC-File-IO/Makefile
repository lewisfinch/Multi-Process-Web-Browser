CC=gcc
CFLAGS= -g -Wall `pkg-config --cflags gtk+-3.0 webkit2gtk-4.0`
LDFLAGS+=`pkg-config --libs gtk+-3.0 webkit2gtk-4.0`
DEPS = -Wno-deprecated -Wno-deprecated-declarations 

LIB=/usr/lib/
BINS=wrapper.o util.o
EXS=browser 

all:  $(EXS) $(BINS)

browser: $(BINS) browser.c
	$(CC) $(BINS) browser.c $(DEPS) -L ${LIB} $(CFLAGS) $(LDFLAGS) -o browser

clean:
	rm -rf $(EXS)


