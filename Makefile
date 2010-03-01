CC=g++

CPPFLAGS = -Wall -O2 -g
LDLAGS = -lusb-1.0

target = yurex

$(target): $(target).o
	$(CC) $(LDLAGS) $(target).o -o $(target)

.cc.o:
	$(CC) $(CPPFLAGS) -c $<

clean:
	-rm *~ core $(target)

