CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra $(shell root-config --cflags)
LDFLAGS ?= $(shell root-config --libs) -lsqlite3
TARGET = nucondenser
SRC = nucondenser.cxx

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
