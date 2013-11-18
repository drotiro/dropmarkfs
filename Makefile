
# Variables
PKGS = fuse libapp rest-friend
FLAGS = $(shell pkg-config ${PKGS} --cflags)  ${CFLAGS}
LIBS = $(shell pkg-config ${PKGS} --libs)
OBJS = dmfs.o dmapi.o dmcollection.o
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DEPS = drotiro/libapp drotiro/rest-friend

.PHONY: clean install check_pkg deps

# Targets
dmfs:  check_pkg $(OBJS) 
	@echo "Building  $@"
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	@echo Compiling $<
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) *~ dmfs

install: dmfs
	install -s dmfs $(BINDIR)

deps:
	@$(foreach i,$(DEPS), git clone https://github.com/$i && make -C `basename $i` install; )

# Check required programs
PKG_CONFIG_VER := $(shell pkg-config --version 2>/dev/null)
check_pkg:
ifndef PKG_CONFIG_VER
	@echo " *** Please install pkg-config"
	@exit 1
endif

# Dependencies
# (gcc -MM *.c  -D_FILE_OFFSET_BITS=64)
dmapi.o: dmapi.c dmapi.h dmcollection.h
dmcollection.o: dmcollection.c dmcollection.h
dmfs.o: dmfs.c dmapi.h

