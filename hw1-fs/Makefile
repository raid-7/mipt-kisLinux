CC=gcc
CFLAGS=-I. -g
DEPS = fs.h manage.h myutil.h
OBJ = fs.o main.o manage.o myutil.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rfs: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
