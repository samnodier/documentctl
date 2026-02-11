CC = gcc
# -I$(INC_DIR) tells the compiler to look in your include/ folder for .h files
CFLAGS = -Wall -Wextra -fPIC -O2 -g -I$(INC_DIR) `pkg-config --cflags poppler-glib`
LDFLAGS = -shared `pkg-config --libs poppler-glib`

# Folder definitions
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
LIB_DIR = lib
TEST_DIR = tests

# Find all .c files in src/ and map them to build/*.o
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = $(LIB_DIR)/libengine.so

all: $(TARGET)

# The Test Suite: Now includes ALL objects and correct Poppler linking
test: $(OBJS)
	@mkdir -p $(TEST_DIR)/test_data
	$(CC) $(CFLAGS) -o test_roundtrip $(TEST_DIR)/test_roundtrip.c $(OBJS) `pkg-config --libs poppler-glib`
	./test_roundtrip

# Build the shared library
$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Rule to compile .c files from src/ into .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) test_roundtrip $(TEST_DIR)/test_data/*.db
