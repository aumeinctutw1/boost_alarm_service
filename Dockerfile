FROM alpine:3.14
WORKDIR /app

COPY source ./source
COPY timer_server.cpp ./

# install libs and compiler
RUN apk add boost-dev g++

# compile
RUN g++ timer_server.cpp ./source/server.cpp ./source/session.cpp -o server -Wall -Wextra -std=c++17 

# delete all unessesary dependencies
RUN apk del g++
RUN rm -rf ./source && rm -rf timer_server.cpp

EXPOSE 2000

ENTRYPOINT ["./server", "2000"]