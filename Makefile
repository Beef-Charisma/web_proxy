all : clean web_proxy

web_proxy: proxy.o main.o
	g++ -g -o web_proxy main.o proxy.o -lpcap -pthread

main.o:
	g++ -std=c++14 -g -c -o main.o main.cpp

proxy.o:
	g++ -std=c++14 -g -c -o proxy.o proxy.cpp

clean:
	rm -rf *.o

