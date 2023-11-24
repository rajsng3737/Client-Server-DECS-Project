#include <iostream>
#include <string>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include "handleClient.hh"

using namespace std;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
void *threadFunction(void *arg) {
    queue<int> *clientQueue = (queue<int> *)arg;
    while (true) {
        int newsockfd;

        pthread_mutex_lock(&mutex);
        while (clientQueue->empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        newsockfd = clientQueue->front();
        clientQueue->pop();
        pthread_mutex_unlock(&mutex);
        handleClient(newsockfd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <thread_pool_size>\n", argv[0]);
        exit(1);
    }
    int sockfd, newsockfd, portno, n, pool_size;
    portno = atoi(argv[1]);
    pool_size = atoi(argv[2]);

    pthread_t threads[pool_size];
    queue<int> clientQueue;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout << "Error opening socket";
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Error binding";
    }
    listen(sockfd, 2000);
    printf("Now server is listening\n");
    clilen = sizeof(cli_addr);
    // Create and start thread pool
    for (int i = 0; i < pool_size; i++) {
        pthread_create(&threads[i], NULL, threadFunction, &clientQueue);
    }
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            cout << "Error accepting the connection";
        }

        pthread_mutex_lock(&mutex);

        // Add the client socket to the queue
        clientQueue.push(newsockfd);

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    close(sockfd);
    return 0;
}
