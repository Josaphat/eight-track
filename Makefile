BIN  := et
OBJS := et.o et.l.o et.y.o et_compiler.o symbol_memory.o

CPPFLAGS := -D_POSIX_SOURCE
CFLAGS   := -std=c99 -Og -g3 -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-unused-parameter
YFLAGS    = --yacc --defines="$(@:.c=.h)"

$(BIN): $(OBJS)

et.l.c: et.y.h

%.y.h: %.y.c
	-

.PHONY: clean
clean:
	$(RM) $(OBJS) *.l.c *.y.h *.y.c

.PHONY: distclean
distclean: clean
	$(RM) $(BIN)
