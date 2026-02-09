# Variables
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 `pkg-config --cflags poppler-glib`
LDFLAGS = -shared `pkg-config --libs poppler-glib`

# Targets
CRAWLER_LIB = crawler.so
PDF_LIB = pdf.so

# The 'all' target builds everything
all: $(CRAWLER_LIB) $(PDF_LIB)

# Rule to build the crawler library
$(CRAWLER_LIB): crawler.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Rule to build the pdf library
$(PDF_LIB): pdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Clean up binaries
clean:
	rm -f *.so *.o
