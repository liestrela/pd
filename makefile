cc=g++
ccflags=-g -Wall -std=c++14
ldflags=-lcurlpp -lcurl -lpthread

all: pd

pd: pd.cc
	$(cc) -o $@ $< $(ccflags) $(ldflags)

clean:
	rm -f ./pd
