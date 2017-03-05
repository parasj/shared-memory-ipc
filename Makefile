CC = gcc
LDFLAGS = -L snappy-c snappy-c/libsnappyc.so.1 
LIBS = .
SRC = src/client.c
OBJ = $(SRC:.c=.o)
INC1 = snappy-c/
INCDIRS = -I ${INC1}
CFLAGS = ${INCDIRS} -ggdb -Wall -std=gnu99

OUT = bin/libtinyfile.a

all: ${OUT} tinyd test-client tiny

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

tinyd: src/server.c
	$(CC) $(CFLAGS) src/server.c -o bin/tinyd ${LDFLAGS}

tiny: src/app.c ${OBJ} ${OUT}
	$(CC) $(CFLAGS) src/app.c $(OUT) -o bin/tiny

test-client: src/test-client.c ${OBJ} ${OUT}
	$(CC) $(CFLAGS) src/test-client.c $(OUT) -o bin/test-client

clean:
	@rm -rf /tmp/.tiny_msgqfile*
	@rm src/*.o bin/*.a bin/tinyd
	@echo Cleaned!
