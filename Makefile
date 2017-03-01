CC = gcc
LDFLAGS = 
LIBS = .
SRC = src/server.c src/client.c
OBJ = $(SRC:.c=.o)
INC1 = snappy-c/
INCDIRS = -I${INC1}
CFLAGS = -ggdb -Wall -std=gnu99 ${INCDIRS}

OUT = bin/libtinyfile.a

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

tinyd:
	$(CC) $(CFLAGS) src/server.c $(OUT) -o bin/tinyd

clean :
	@rm src/*.o bin/*.a bin/tinyd
	@echo Cleaned!
