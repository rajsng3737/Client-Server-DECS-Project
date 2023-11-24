#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <fstream>
#include <sys/time.h>
#include "handleClient.hh"

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

using namespace std;
struct timeval timestamp;
void error(char *msg)
{
    perror(msg);
    exit(1);
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
int recv_file(int sockfd, const string &file_path)
{
    char buffer[BUFFER_SIZE];              // buffer into which we read the received file chars
    bzero(buffer, BUFFER_SIZE);            // initialize buffer to all NULLs
    ofstream file(file_path, ios::binary); // Get a file descriptor for writing received data into the file
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    // Buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];

    // First, receive the file size bytes
    if (read(sockfd, file_size_bytes, MAX_FILE_SIZE_BYTES) == -1)
    {
        perror("Error receiving file size");
        file.close();
        return -1;
    }

    int file_size;

    // Copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));

    // Some local printing for debugging, print file size
    // cout << "File size is: " << file_size << endl;

    // Now start receiving file data
    size_t bytes_read = 0, total_bytes_read = 0;

    while (true)
    {
        // Read a maximum of BUFFER_SIZE bytes of file data
        bytes_read = read(sockfd, buffer, BUFFER_SIZE);

        // Total number of bytes read so far
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            file.close();
            return -1;
        }

        // Write the buffer to the file
        file.write(buffer, bytes_read);

        // Reset the buffer
        bzero(buffer, BUFFER_SIZE);

        // Break out of the reading loop if we've read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }

    file.close();
    return 0;
}
void handleClient(int newsockfd)
{
    gettimeofday(&timestamp, NULL);
    string fn_prefix = to_string(timestamp.tv_sec) + to_string(rand());
    char buffer[2048]; // Increased buffer size
    string filename = (fn_prefix + "RF.c").c_str();
    if (recv_file(newsockfd, filename) != 0)
    {
        cout << "Error in recieving file" << endl;
    }

    int compile_status = system(("gcc " + fn_prefix + "RF.c -o " + fn_prefix + "RF.out 2> " + fn_prefix + "compile_error_log").c_str());
    if (compile_status == 0)
    {
        printf("Compilation Successful\n");
        int run_status = system(("./" + fn_prefix + "RF.out > " + fn_prefix + "output_log 2> " + fn_prefix + "runtime_error_log").c_str());
        printf("%d\n", run_status);
        if (run_status == 0)
        {
            int diffstatus = system(("diff " + fn_prefix + "output_log original.txt > " + fn_prefix + "diffOutput.txt").c_str());
            if (diffstatus == 0)
            {
                char ce[] = "PASS\n";
                write(newsockfd, &ce, sizeof(ce));
                string file1 = (fn_prefix + "output_log");

                if (send_file(newsockfd, file1.c_str()) < 0)
                {
                    cout << "Error in sending file" << endl;
                }
            }
            else
            {
                char ce[] = "OUTPUT ERROR";
                write(newsockfd, &ce, sizeof(ce));
                const char *file1 = (fn_prefix + "diffOutput.txt").c_str();
                if (send_file(newsockfd, file1) < 0)
                {
                    cout << "Error in sending file" << endl;
                }
            }
        }
        else
        {
            char ce[] = "RUNTIME ERROR";
            write(newsockfd, &ce, sizeof(ce));
            const char *file1 = (fn_prefix + "runtime_error_log").c_str();
            if (send_file(newsockfd, file1) < 0)
            {
                cout << "Error in sending file" << endl;
            }
        }
    }
    else
    {
        char ce[] = "COMPILER ERROR";
        write(newsockfd, &ce, sizeof(ce));
        const char *file1 = (fn_prefix + "runtime_error_log").c_str();
        if (send_file(newsockfd, file1) < 0)
        {
            cout << "Error in sending file" << endl;
        }
    }
    system(("rm -f " + fn_prefix + "RF.c").c_str());
    system(("rm -f " + fn_prefix + "RF.out").c_str());
    system(("rm -f " + fn_prefix + "output_log").c_str());
    system(("rm -f " + fn_prefix + "runtime_error_log").c_str());
    system(("rm -f " + fn_prefix + "diffOutput.=
    system(("rm -f " + fn_prefix + "compile_error_log").c_str());
    close(newsockfd);
}