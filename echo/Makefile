CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = echo_client
SOURCES = echo_client.cpp

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.o

.PHONY: clean
