server:
    gcc -o server server.c -lsqlite3 -lpthread
    ./server

client:
    gcc -o client client.c
    ./client 192.168.221.128 2728
