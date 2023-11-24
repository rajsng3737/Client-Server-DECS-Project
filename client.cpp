#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <fstream>
#include <errno.h>
using namespace std;
const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}
struct timeval Tsend;
struct timeval Trecv;
struct timeval Tavg;
struct timeval Tlstart;
struct timeval Tlend;
struct timeval timeout;
float Ttotal_response = 0;
int sucResponse = 0;
void error(const char *msg)
{
    perror(msg);
    exit(1); // Return an error code
}
int send_file(int sockfd, const char *file_path)
{
    char buffer[BUFFER_SIZE];              // buffer to read from file
    bzero(buffer, BUFFER_SIZE);            // initialize buffer to all NULLs
    ifstream file(file_path, ios::binary); // open the file for reading, get file descriptor
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    file.seekg(0, ios::end); // for finding file size in bytes
    int file_size = file.tellg();
    // cout << "File size is: " << file_size << endl;

    file.seekg(0, ios::beg); // reset file descriptor to the beginning of the file

    char file_size_bytes[MAX_FILE_SIZE_BYTES]; // buffer to send file size to the server

    // copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));

    // send file size to the server, return -1 if error
    if (write(sockfd, file_size_bytes, MAX_FILE_SIZE_BYTES) == -1)
    {
        perror("Error sending file size");
        file.close();
        return -1;
    }

    // now send the source code file
    while (!file.eof())
    { // while not reached the end of the file
        // read buffer from the file
        size_t bytes_read = file.read(buffer, BUFFER_SIZE - 1).gcount();

        // send to the server
        if (send(sockfd, buffer, bytes_read + 1, 0) == -1)
        {
            perror("Error sending file data");
            file.close();
            return -1;
        }

        // clean out buffer before reading into it again
        bzero(buffer, BUFFER_SIZE);
    }

    // close file
    file.close();
    return 0;
}
int main(int argc, char *argv[])
{
    int sockfd, portno, n, refusedConnections = 0, timedoutConnections = 0, loopnum = atoi(argv[4]), sleeptime = atoi(argv[5]);
    char buffer[2048];
    ssize_t bytesRead = 0;
    ssize_t bytesSent = 0;
    if (argc < 6)
    {
        printf("Usage: %s serverip portno sourcefile loopnum sleeptime\n", argv[0]);
        exit(1); // Return an error code
    }
    portno = atoi(argv[2]);
    char *filename = (char *)argv[3];
    gettimeofday(&Tlstart, NULL);
    while (loopnum != 0)
    {
        struct sockaddr_in serv_addr;
        struct hostent *server;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("Unable to create socket");
        timeout.tv_sec = 1; // Set a 5-second timeout (adjust as needed)
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("Error setting receive timeout");
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("Error setting send timeout");
        }
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

        gettimeofday(&Tsend, NULL);
        if (send_file(sockfd, filename) < 0)
        {
            cout << "Error in sending file" << endl;
        }

        if (bytesSent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Send timeout occurred, but continuing the loop.\n");
                timedoutConnections++;
                // Handle the timeout condition here if needed
            }
            else if (errno == ECONNREFUSED)
            {
                printf("Connection refused by the server.\n");
                refusedConnections++; // Increment the refused connection counter
                // Handle the refused connection condition here if needed
            }
            else
            {
                perror("Error Sending File");
                close(sockfd);
                exit(1); // Return an error code
            }
        }
        bzero(buffer, sizeof(buffer));
        bool flag = false;
        int x = 0;
        while (true)
        {
           
            // read server response into buffer
            size_t bytes_read = read(sockfd, buffer, BUFFER_SIZE);
            if (!flag)
            {
                gettimeofday(&Trecv, NULL);
            }
            if (!flag && bytes_read)
            {
                sucResponse++;
            }
            if (bytes_read <= 0)
                break;
            // write(STDOUT_FILENO, "Server Response: Success", 17);
        
            flag = true;
            // cout << buffer << endl;
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        Ttotal_response += timedifference_msec(Tsend, Trecv);
        sleep(sleeptime);
        bzero(buffer, BUFFER_SIZE);

        loopnum = loopnum - 1;
    }
    gettimeofday(&Tlend, NULL);
    float Ttotal_loop = timedifference_msec(Tlstart, Tlend);
    printf("Successful Request Rate %f per second\n", (sucResponse / Ttotal_loop) * 1000);
    printf("Average Response Time %f per second\n", (Ttotal_response / sucResponse) * 1000);
    printf("The number of successful responses are %d\n", sucResponse);
    printf("Total Time spent in loop %f seconds\n", timedifference_msec(Tlstart, Tlend) * 1000);
    printf("Throughput %f per seconds\n", (sucResponse / Ttotal_response) * 1000);
    printf("Request Rate Sent %f per second\n", (atoi(argv[4]) / Ttotal_response) * 1000);
    printf("Total Refused Connections %f per second\n", (refusedConnections / Ttotal_response) * 1000);
    printf("Total Timed Out Connections %f per second\n", (timedoutConnections / Ttotal_response) * 1000);
    close(sockfd);
    return 0;
}