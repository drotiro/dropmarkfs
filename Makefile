
# Variables
PKGS = fuse libapp rest-friend
FLAGS = $(shell pkg-config ${PKGS} --cflags)  ${CFLAGS}
LIBS = $(shell pkg-config ${PKGS} --libs)
OBJS = dmfs.o dmapi.o dmcollection.o
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DEPS = libapp rest-friend
STATIC_LIBS = -L./libapp/libapp -L./rest-friend

# Targets
dmfs: check_pkg $(OBJS) 
	@echo "Building  $@"
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

static: check_pkg deps $(OBJS)
	@echo "Building  dmfs"
	$(CC) $(FLAGS) $(LDFLAGS) -o dmfs $(OBJS) $(LIBS) $(STATIC_LIBS)
                

.c.o:
	@echo Compiling $<
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) *~ dmfs

install: dmfs
	install -s dmfs $(BINDIR)

deps:
	@$(foreach i,$(DEPS), (test -d `basename $i` || git clone https://github.com/drotiro/$i) && make -C `basename $i`; )

# Check required programs
check_pkg:
	$(shell pkg-config ${PKGS} --exists --print-errors) 

# Dependencies
# (gcc -MM *.c  -D_FILE_OFFSET_BITS=64)
dmapi.o: dmapi.c dmapi.h dmcollection.h
dmcollection.o: dmcollection.c dmcollection.h
dmfs.o: dmfs.c dmapi.h

