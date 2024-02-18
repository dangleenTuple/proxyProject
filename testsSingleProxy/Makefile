CC=g++ -std=c++20
OBJ_FILES := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: singleProxyTest

cleanTests:
	rm $(OBJ_FILES)
	rm singleProxyTest

%.o: %.cpp
	$(CC) -c $<

singleProxyTest: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) -lboost_system -lboost_thread
