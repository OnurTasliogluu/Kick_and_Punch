LD_FLAGS:=-ldl	-lpthread
CUSTOM_INC:=-I./inc
OBJECTS:=kick_and_punch.o
DEBUG:=-DDEBUG
CC:=gcc

main:$(OBJECTS)
	$(CC)	-g	$(DEBUG)	$(CUSTOM_INC)	$(OBJECTS)	main.c	-o	main	$(LD_FLAGS)

kick_and_punch.o:
	$(CC)	-g	$(DEBUG)	$(CUSTOM_INC)	-c	src/kick_and_punch.c	$(LD_FLAGS)

clean:
rm *.o main