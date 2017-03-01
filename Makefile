CC = gcc
LDFLAGS = 
LIBS = .
SRC = src/client.c
OBJ = $(SRC:.c=.o)
INC1 = snappy-c/
INCDIRS = -I${INC1}
CFLAGS = -ggdb -Wall -std=gnu99 ${INCDIRS}

OUT = bin/libtinyfile.a

all: tinyd test-client ${OUT}

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

tinyd: src/server.c
	$(CC) $(CFLAGS) src/server.c -o bin/tinyd

test-client: src/test-client.c ${OBJ}
	$(CC) $(CFLAGS) src/test-client.c $(OUT) -o bin/test-client

clean :
	@rm src/*.o bin/*.a bin/tinyd
	@echo Cleaned!
