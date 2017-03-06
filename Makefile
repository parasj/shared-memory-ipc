CC = gcc
LIBS = .
SRC = src/client.c
OBJ = $(SRC:.c=.o)
INC1 = snappy-c/
INCDIRS = -I ${INC1}
CFLAGS = ${INCDIRS} -ggdb -Wall -std=gnu99
OUT = libtinyfile.a
LDFLAGS = -L snappy-c snappy-c/libsnappyc.so.1 -L bin -l:${OUT}

all: snappy ${OUT} tinyd test-client tiny

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs ./bin/$(OUT) $(OBJ)

tinyd: src/server.c
	$(CC) $(CFLAGS) src/server.c -o bin/tinyd ${LDFLAGS}

tiny: src/app.c ${OBJ} ${OUT}
	$(CC) $(CFLAGS) src/app.c -o bin/tiny ${LDFLAGS}

test-client: src/test-client.c ${OBJ} ${OUT}
	$(CC) $(CFLAGS) src/test-client.c -o bin/test-client ${LDFLAGS}

snappy:
	$(MAKE) -C snappy-c

clean:
	-@rm -rf /tmp/.tiny_msgqfile*;
	-@rm src/*.o bin/*.a bin/tinyd;
	-@ ipcs -q | awk ' { print $2 } ' | xargs ipcrm msg 2>/dev/null;
	-@ ipcs -s | awk ' { print $2 } ' | xargs ipcrm sem 2>/dev/null;
	-@ ipcs -m | awk ' { print $2 } ' | xargs ipcrm shm 2>/dev/null;
	@echo Cleaned!
