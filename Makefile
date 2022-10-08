OUTPUT = server
INPUT = timer_server.cpp
MODULES = ./source/server.cpp ./source/session.cpp 
FLAGS = -Wall -Wextra -std=c++17 
INCLUDE = -I /opt/homebrew/Cellar/boost/1.79.0_2/include

build_client:
	g++ client.cpp -o client

build_server:
	g++ $(INCLUDE) $(INPUT) $(MODULES) -o $(OUTPUT) $(FLAGS)

build_container: build_server
	docker build -t aumeinctutw1/boost_alarm_service .

build: build_client build_server

run_server: build_server
	./server 2000