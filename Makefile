CC = gcc
# -Iinclude tells the compiler to look in the include directory for .h files
CFLAGS = -Wall -Wextra -fPIC -O2 -Iinclude `pkg-config --cflags poppler-glib`
LDFLAGS = -shared `pkg-config --libs poppler-glib`

# Folder definitions
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
LIB_DIR = lib
TEST_DIR = tests

# Logic to find all .c files in src/ and map them to build/*.o
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = $(LIB_DIR)/libengine.so

all: $(TARGET)

# The Test Suite: Links the test file with the engine objects
test: $(BUILD_DIR)/index_structure.o $(BUILD_DIR)/query_engine.o
	$(CC) -I$(INC_DIR) $(CFLAGS) -o test_suite $(TEST_DIR)/tests.c $^
	./test_suite

# Build the shared library in the lib/ folder
$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Rule to compile .c files from src/ into .o files in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) test_suite
