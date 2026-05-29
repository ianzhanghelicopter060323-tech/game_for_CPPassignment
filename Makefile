CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -O2 -Isrc
LDFLAGS ?=
LDLIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || echo -lncurses)

BUILD_DIR := build
TARGET := $(BUILD_DIR)/ascii_dungeon_duel
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
