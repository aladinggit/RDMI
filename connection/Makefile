LDFLAGS = -libverbs -lpthread #-ldl
CFLAGS += -Wall -std=c++11 -I./  

kk_s:
	$(rm -f rdmatry_server)
kk_c:
	$(rm -f rdmatry_client)

rdmas: kk_s
	g++ rdmatry_server.cpp $(CFLAGS) $(LDFLAGS) -lrdmacm   -o rdmatry_server

rdmac: kk_c
	g++ rdmatry_client.cpp $(CFLAGS) $(LDFLAGS) -lrdmacm   -o rdmatry_client

debug: CFLAGS += -DDEBUG -g -O0
debug: ${APPS}

.DELETE_ON_ERROR:
.PHONY: all clean
