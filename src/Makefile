OBJ_FILES := $(patsubst %.c,%.o,$(wildcard *.c))
HEADER_FILES := $(wildcard *.h)

all: multiProxy

clean:
	rm $(OBJ_FILES)
	rm multiProxy

%.o: %.c $(HEADER_FILES)
	$(CC) -c -Wall $<

multiProxy: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES)
