cc=g++
ccflags=-Wall -std=c++14
ldflags=-lcurlpp -lcurl -lpthread
prefix=/usr/local

all: pd

pd: pd.cc
	$(cc) -o $@ $< $(ccflags) $(ldflags)

install: pd
	install ./pd $(prefix)/bin

clean:
	rm -f ./pd
