CXX = g++
CXXFLAGS = -O2 -Wall -Isrc/include -I/usr/include/mysql/ -L/usr/lib -std=c++11
SOURCES = src/main.cpp src/server.cpp src/channel.cpp src/session.cpp src/util.cpp
LIBS = -lmysqlpp -lscrypt -lcrypto -lpthread 
OUTPUT = nadlserver
OBJECTS = $(SOURCES:.cpp=.o)
RM = rm -f

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CXX) $(LIBS) -o $@ $^

$(SOURCES:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) -MM $< -o $@
	
clean: 
	-$(RM) $(OUTPUT) $(OBJECTS) $(SOURCES:.cpp=.d)

install:
	install $(OUTPUT) /usr/bin