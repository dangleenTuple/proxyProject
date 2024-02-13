OBJ_FILES := $(patsubst %.c,%.o,$(wildcard *.c))

all: singleProxy

install: singleProxy
	cp singleProxy ..

clean:
	rm $(OBJ_FILES)
	rm singleProxy

%.o: %.c
	$(CC) -c $<

singleProxy: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES)
