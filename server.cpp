#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include<thread>
#include <fcntl.h>
#include<sys/time.h>
using namespace std;
struct timeval timestamp;
void error(char *msg)
{
    perror(msg);
    exit(1);
}
void *handleClient(void *arg)
{
    int newsockfd = *((int *)arg);
	gettimeofday(&timestamp,NULL);
	string fn_prefix = to_string(timestamp.tv_sec)+to_string(rand());	
    char buffer[2048]; // Increased buffer size
    int fp = open((fn_prefix+"RF.c").c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    ssize_t bytesRead = 0, bytesWritten = 0;
    ssize_t nwords;
    read(newsockfd, &nwords, sizeof(nwords));
    read(newsockfd, buffer, nwords);
    write(fp, buffer, nwords);
    printf("Now Running RF.c file\n");
    close(fp);
    // Compile and run code
    int compile_status = system(("gcc "+fn_prefix+"RF.c -o "+fn_prefix+"RF.out 2> "+fn_prefix+"compile_error_log").c_str());
    if (compile_status == 0)
    {
        printf("Compilation Successful\n");
        // Run the compiled program
        int run_status = system(("./"+fn_prefix+"RF.out > "+fn_prefix+"output_log 2> "+fn_prefix+"runtime_error_log").c_str());
        printf("%d\n", run_status);
        if ((run_status) == 0)
        {
            int diffstatus = system(("diff "+fn_prefix+"output_log original.txt > "+fn_prefix+"diffOutput.txt").c_str());
            if (diffstatus == 0)
            {
                ssize_t bytesRead = 0;
                ssize_t bytesSent = 0;
                int fp = open("correct.txt", O_RDONLY);
                bytesRead = read(fp, buffer, sizeof(buffer));
                write(newsockfd, &bytesRead, sizeof(bytesRead));
                bytesWritten = write(newsockfd, buffer, bytesRead);
                if (bytesWritten != bytesRead)
                {
                    printf("Error Sending the File. Exiting");
                    exit(1);
                }
            }
            else
            {
                FILE *output_error = fopen((fn_prefix+"diffOutput.txt").c_str(), "r+");
                if (output_error != NULL)
                {
                    ssize_t bytesRead = 0;
                    ssize_t bytesSent = 0;
                    char ce[] = "OUTPUT ERROR";
                    // fprintf(output_error,"%s\n",ce);
                    fclose(output_error);
                    int fp = open((fn_prefix+"diffOutput.txt").c_str(), O_RDONLY);
                    bytesRead = read(fp, buffer, sizeof(buffer));
                    write(newsockfd, &bytesRead, sizeof(bytesRead));
                    bytesWritten = write(newsockfd, buffer, bytesRead);
                    if (bytesWritten != bytesRead)
                    {
                        printf("Error Sending the File. Exiting");
                        exit(1);
                    }
                }
            }
        }
        else
        {
            FILE *runtime_error = fopen((fn_prefix+"runtime_error_log").c_str(), "r+");
            char ce[] = "RUNTIME ERROR";
            rewind(runtime_error);
            fprintf(runtime_error, "%s\n", ce);
            fclose(runtime_error);
            int fp = open((fn_prefix+"runtime_error_log").c_str(), O_RDONLY);
            bytesRead = read(fp, buffer, sizeof(buffer));
            write(newsockfd, &bytesRead, sizeof(bytesRead));
            bytesWritten = write(newsockfd, buffer, bytesRead);
            if (bytesWritten != bytesRead)
            {
                printf("Error Sending the Runtime Error. Exiting");
                exit(1);
            }
        }
    }
    else
    {
        FILE *compile_error = fopen((fn_prefix+"compile_error_log").c_str(), "r+");
        char ce[] = "COMPILER ERROR";
        fprintf(compile_error, "%s\n", ce);
        fclose(compile_error);
        int fp = open((fn_prefix+"compile_error_log").c_str(), O_RDONLY);
        bytesRead = read(fp, buffer, sizeof(buffer));
        write(newsockfd, &bytesRead, sizeof(bytesRead));
        bytesWritten = write(newsockfd, buffer, bytesRead);
        if (bytesWritten != bytesRead)
        {
            printf("Error Sending the Compiler Error. Exiting");
            exit(1);
        }
    }
    system(("rm -f "+fn_prefix+"RF.c").c_str());
    system(("rm -f "+fn_prefix+"RF.out").c_str());
    system(("rm -f "+fn_prefix+"output_log").c_str());
    system(("rm -f "+fn_prefix+"runtime_error_log").c_str());
    system(("rm -f "+fn_prefix+"diffOutput.txt").c_str());
    system(("rm -f "+fn_prefix+"compile_error_log").c_str());
    close(newsockfd);
    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, n;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);
    char buffer[2048]; // Increased buffer size
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cout << "Error opening socket";
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Error binding";
    }
    listen(sockfd, 20000);
    printf("Now server is listening\n");
    clilen = sizeof(cli_addr);
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            cout << "Error accepting the connection";
        }
        pthread_t thread;
        int *copysock = (int *)malloc(sizeof(int));
        *copysock=newsockfd;
        pthread_create(&thread,NULL,handleClient,copysock);
        //thread thread1(handleClient,newsockfd);
        //thread1.join();
    }
    close(sockfd);
    return 0;
}
