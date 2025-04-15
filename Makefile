CC = gcc
CFLAGS = -Wall -Werror
TARGET = shell

all: $(TARGET)

$(TARGET): shell.c
	$(CC) $(CFLAGS) -o $(TARGET) shell.c

clean:
	rm -f $(TARGET)
	
.PHONY: all clean