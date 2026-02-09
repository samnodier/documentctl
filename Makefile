CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 `pkg-config --cflags poppler-glib`
LDFLAGS = -shared `pkg-config --libs poppler-glib`

# List all your source files here
SRCS = engine.c trie.c crawler.c search.c pdf.c
OBJS = $(SRCS:.c=.o)
TARGET = libengine.so

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
