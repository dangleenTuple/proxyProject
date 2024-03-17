CC=g++ -std=c++20
OBJ_FILES := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: multiProxyTest

cleanTests:
	rm $(OBJ_FILES)
	rm multiProxyTest

%.o: %.cpp
	$(CC) -c $<

multiProxyTest: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) -lboost_system -lboost_thread
