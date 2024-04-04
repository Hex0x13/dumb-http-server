#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <stdlib.h>


#define PORT 8080

#define DIE(msg) \
    fprintf(stderr, "Error in "__FILE__" line %d: %s\n", __LINE__ - 1, msg); \
    goto cleanup;

typedef struct {
    int socket;
    struct sockaddr_in addr;
    socklen_t len;
} client_t;

void print_addr();
void *accept_client(void *args);


int main(int argc, char *argv[]) {
    int server_socket;
    struct sockaddr_in server;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        DIE(strerror(errno));
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == -1) {
        DIE(strerror(errno));
    }

    if (listen(server_socket, 5) == -1) {
        DIE(strerror(errno));
    }

    pthread_t thread;
    print_addr();

    while (1) {
        struct sockaddr_in client;
        socklen_t client_len;
        int client_socket = accept(server_socket, (struct sockaddr *)&client, &client_len);
        client_t args = {
            .socket = client_socket,
            .addr = client,
            .len = client_len
        };
        if (client_socket > -1) {
            pthread_create(&thread, NULL, accept_client, &args);
        } else {
            DIE(strerror(errno));
        }
    }

    return 0;

cleanup:
    close(server_socket);
    return 1;
}

void print_addr() {
    struct ifaddrs *addrs = NULL;

    if (getifaddrs(&addrs) == -1) {
        fprintf(stderr,"Error: getting address error\n");
        exit(1);
    }

    for (struct ifaddrs *tmp=addrs; tmp; tmp=tmp->ifa_next) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)tmp->ifa_addr;
            printf("%s: %s\n", tmp->ifa_name, inet_ntoa(addr->sin_addr));
        }
    }
}


void *accept_client(void *args) {
    client_t client = *((client_t *)args);
    const char *path = "./dest/index.html";
    FILE *page = fopen(path, "rb");

    if (page == NULL) {
        perror("Opening File Error");
        return NULL;
    }

    fseek(page, 0, SEEK_END);
    long size = ftell(page);
    fseek(page, 0, SEEK_SET);

    char header[1024];
    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n\r\n",
    size);

    send(client.socket, header, strlen(header)+1, 0);
    
    char buffer[1024];
    
    size_t bytes_read;

    do {
        bytes_read = fread(buffer, sizeof(char), sizeof(buffer), page);
        send(client.socket, buffer, bytes_read, 0);
    } while(bytes_read > 0);

    printf("Sent %ld bytes of message\n", size);
    fclose(page);

    return NULL;
}
