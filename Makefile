CC=g++

CPPFLAGS = -Wall -O2 -g
CPPFLAGS += -lusb-1.0

target = yurex

$(target):
	$(CC) $(CPPFLAGS) yurex.cc -o $(target)

clean:
	-rm *~ core $(target)

