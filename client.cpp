#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}
struct timeval Tsend;
struct timeval Trecv;
struct timeval Tavg;
struct timeval Tlstart;
struct timeval Tlend;
float Ttotal=0;
int sucResponse=0;
void error(const char *msg)
{
    perror(msg);
    exit(1); // Return an error code
}
int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    char buffer[2048];
    if (argc < 6)
    {
        printf("Usage: %s serverip portno sourcefile loopnum sleeptime\n", argv[0]);
        exit(1); // Return an error code
    }
    portno = atoi(argv[2]);
    char *filename = (char *)argv[3];
    int loopnum = atoi(argv[4]);
    int sleeptime = atoi(argv[5]);
    ssize_t bytesRead = 0;
    ssize_t bytesSent = 0;
    gettimeofday(&Tlstart,NULL);
    while (loopnum != 0)
    {
        struct sockaddr_in serv_addr;
        struct hostent *server;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("Unable to create socket");
        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            error("No such host.");
        }
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Connection Failed");
        bzero(buffer, 2048);
        int fp;
        // opening file to send
        fp = open(filename, O_RDONLY);
        // sending the characters to read from the stream to the server
        bytesRead = read(fp, &buffer, sizeof(buffer));
        write(sockfd, &bytesRead, sizeof(bytesRead));
        gettimeofday(&Tsend, NULL);
        bytesSent = write(sockfd, &buffer, bytesRead);
        if (bytesSent < 0)
        {
            perror("Error Sending File");
            close(fp);
            close(sockfd);
            exit(1); // Return an error code
        }
        bzero(buffer, sizeof(buffer));
        read(sockfd, &bytesRead, sizeof(bytesRead));
        gettimeofday(&Trecv, NULL);
        if(bytesRead != 0)
            sucResponse++;
        Ttotal += timedifference_msec(Tsend, Trecv);
        read(sockfd, buffer, bytesRead);
        printf("%s", buffer);
        sleep(sleeptime);
        bzero(buffer, 2048);
        close(fp);
        loopnum = loopnum-1;
    }
    gettimeofday(&Tlend,NULL);
    float Ttotal_loop = timedifference_msec(Tlstart,Tlend);
    printf("Successful Request Rate %f per second\n",(sucResponse/Ttotal_loop)*1000);
    printf("Average Response Time %f per second\n",(Ttotal/sucResponse)*1000);
    printf("The number of successful responses are %d\n",sucResponse);
    printf("Total Time spent in loop %f seconds\n",timedifference_msec(Tlstart,Tlend)*1000);
    printf("Throughput %f per seconds\n", (sucResponse/Ttotal)*1000);
    printf("Request Rate Sent %f per second\n",(atoi(argv[4])/Ttotal_loop)*1000);
    close(sockfd);
    return 0;
}
