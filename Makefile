BIN_NAME := balatro_server

ifeq ($(OS),Windows_NT)
CXXFLAGS := -std=gnu++11
else
CXXFLAGS := -std=c++11
endif
INCLUDES := -I include/
LIBS := -pthread -lcrypto -lssl

SRC_PATH := src
SRC_FOLDERS := $(shell find $(SRC_PATH) -maxdepth 1 -type d | grep -o '$(SRC_PATH)/.*')
BUILD_PATH := build
BIN_PATH := $(BUILD_PATH)/bin

SOURCES := $(shell find $(SRC_PATH) -name '*.cpp' | sort -k 1nr | cut -f2-)
OBJECTS := $(SOURCES:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)
OBJECT_FOLDERS := $(SRC_FOLDERS:$(SRC_PATH)/%=$(BUILD_PATH)/%)
DEPENDENCIES := $(OBJECTS:.o=.d)

all: dirs $(BIN_PATH)/$(BIN_NAME)

$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LIBS) -o $@

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@

-include $(DEPENDENCIES)

dirs:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(OBJECT_FOLDERS)
	@mkdir -p $(BIN_PATH)

clean:
	rm -rf $(BIN_PATH)
	rm -rf $(BUILD_PATH)