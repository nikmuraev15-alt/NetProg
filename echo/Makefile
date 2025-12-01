CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
TARGET_CLIENT = echo_client
TARGET_SERVER = test_echo_server
SOURCE_CLIENT = echo_client.cpp
SOURCE_SERVER = test_echo_server.cpp

all: $(TARGET_CLIENT) $(TARGET_SERVER)

$(TARGET_CLIENT): $(SOURCE_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CLIENT) $(SOURCE_CLIENT)

$(TARGET_SERVER): $(SOURCE_SERVER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERVER) $(SOURCE_SERVER)

clean:
	rm -f $(TARGET_CLIENT) $(TARGET_SERVER)

.PHONY: all clean
