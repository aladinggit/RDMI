APPS = send

LDFLAGS = -libverbs -lpthread #-ldl
CFLAGS += -Wall -std=c++11 -I./  

all: CFLAGS += -O2
all: ${APPS}

clean:
	$(foreach fname,${APPS}, rm -f ${fname})

send: clean
	g++ send.cpp --std=c++11 -lz -o send
