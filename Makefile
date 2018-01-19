CFLAGS += -O2 -Wall #-ggdb3
LDFLAGS += -O2 -lnetpbm -lm #-ggdb3 #-lefence

all: convert-photo-to-c64

convert-photo-to-c64: convert-photo-to-c64.o generate.o evaluate.o

#.o.c:

#%.o: %.c
