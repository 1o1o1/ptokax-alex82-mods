# ---------------------------------------------------------------------------
#	makefile for SHA-2 lua library
# ---------------------------------------------------------------------------

CC=gcc
LUAPKG=lua5.1
OUTFILE=sha2.so
CFLAGS=`pkg-config $(LUAPKG) --cflags` -O2 -Wall -fno-strict-aliasing -fPIC
LFLAGS=-shared `pkg-config $(LUAPKG) --libs` -fPIC
INSTALL_PATH=`pkg-config $(LUAPKG) --variable=INSTALL_CMOD`
OBJS = $(CURDIR)/sha2.o $(CURDIR)/sha2lib.o ../base64/base64.o

# ---------------------------------------------------------------------------

all: $(OUTFILE)

$(OUTFILE): $(OBJS)
	$(CC) $(OBJS) -o $(OUTFILE) $(CFLAGS) $(LFLAGS)

install: $(OUTFILE)
	mkdir -p $(INSTALL_PATH)
	install -s $(OUTFILE) $(INSTALL_PATH)

clean:
	rm -f $(OUTFILE) $(OBJS)
