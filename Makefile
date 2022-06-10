# Musuroi Daniel-Nicusor, 323CB
# Makefile

CFLAGS = -Wall -g

all: server subscriber

# Compileaza server.cpp
server: server.cpp

# Compileaza client.cpp
subscriber: subscriber.cpp

clean:
	rm -f server subscriber
