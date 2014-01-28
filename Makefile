CC = gcc
CFLAGS = -lm
DEBUGFLAGS = -g -ggdb
LDFLAGS = -lm

DEPEND = makedepend
DEPEND_FLAGS = -Y   # suppresses shared includes
DEPEND_DEFINES = 

srcdir = 
INCLUDES = -I$(srcdir)

SRCS = iota-bootstrap.c
OBJS = iota-bootstrap.o
EXE = iota

HDRS = 

all: debug

debug: CFLAGS += ${DEBUGFLAGS}
debug: $(EXE)

clean:
	rm -f *.o a.out core ${EXE}

depend:
	${DEPEND} -s '# DO NOT DELETE: updated by make depend'		   \
	$(DEPEND_FLAGS) -- $(INCLUDES) $(DEFS) $(DEPEND_DEFINES) $(CFLAGS) \
	-- ${SRCS}

.PHONY: TAGS
.PHONY: tags
TAGS: tags
tags:
	rm -f TAGS
	find $(srcdir) -name '*.[chly]' -print | xargs etags -a

.c.o:
	$(CC) -c $(INCLUDES) $(DEFS) $(CFLAGS) $<

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# DO NOT DELETE: updated by make depend
