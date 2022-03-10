all: proxy test
test: test.cpp
	g++ -g -std=c++11 -o test test.cpp
proxy: functions.cpp proxy.cpp
	g++ -g -std=c++11 -o proxy functions.cpp proxy.cpp -pthread

.PHONY:
	clean
clean:
	rm -rf *.o main proxy test