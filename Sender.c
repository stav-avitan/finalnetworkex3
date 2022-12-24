#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#define SERVER_PORT 6000
#define SERVER_IP_ADDRESS "127.0.0.1"
#define FILE_SIZE 2136287

void smts(char *half_file, int socket_fd);

int main()
{
    //Calculating the XOR
    int id1 = 8859;
    int id2 = 5872;
    int XOR = id1 ^ id2;
    char file_again;

    //Creating socket and using TCP protocol
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("Could not create socket");
    }

    //Creating sockaddr_in struct, reset it, and enter important values.
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    //Convert the ip to binary
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &server_address.sin_addr);
    if (rval <= 0)
    {
        perror("inet pton() failed");
        return -1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    //Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("connect() failed");
        return -1;
    }

    printf("connected to server\n");

    //Open the file
    FILE *myFile;
    myFile = fopen("message.txt", "r");

    //The size of the parts
    long partonesize = FILE_SIZE / 2;
    long parttwosize = FILE_SIZE - (FILE_SIZE / 2);

    // Read the parts into arrays
    char first_half_message[partonesize];
    for (int i = 0; i < FILE_SIZE; i++)
    {
        fscanf(myFile, "%c", &first_half_message[i]);
    }
    char second_half_message[parttwosize];
    for (int i = parttwosize; i < FILE_SIZE; i++)
    {
        fscanf(myFile, "%c", &second_half_message[i]);
    }

    //Endless loop that we can send the file how many times we want.
    while (1)
    {
        //Setting the cc algorithm to cubic
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, "cubic", 5) == -1)
        {
            perror("setsockopt() failed");
            return -1;
        }

        //We send the first part of the file.
        printf("First part :");
        smts(first_half_message, socket_fd);

        //Receiving the authentication.
        int result = 0;
        int bytes_received = recv(socket_fd, &result, sizeof(result), 0);
        if (bytes_received == -1)
        {
            perror("recv() failed");
        }
        else if (bytes_received == 0)
        {
            perror("Peer has closed the TCP connection prior to recv().\n");
        }

        //Checking for authentication.
        if (XOR != result)
        {
            perror("Authentication failed\n");
            close(socket_fd);
            return -1;
        }
        else{
            printf("Authentication succeed\n");
        }

        //Setting the CC algorithm to reno.
        if (setsockopt(socket_fd, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) == -1)
        {
            perror("setsockopt() failed");
            return -1;
        }

        //Sending the second part of the file.
        printf("Second part :");
        smts(second_half_message, socket_fd);

        //We give the server time to process the data.
        sleep(3);

        //Asking the client whether to continue or not.
        free_will:
        printf("Send the file again? (y/n): ");
        scanf(" %c", &file_again);
        if (file_again == 'n')
        {
            //Exit from the program
            int exit_send = send(socket_fd, "N", 1, 0);
            if(exit_send == -1){
                printf("exit message has failed to send\n");
            }
            close(socket_fd);
            file_again=0;
            exit(1);

        }
        else if(file_again == 'y')
        {
            //Continue
            send(socket_fd, "Y", 1, 0);
        }
        else
        {
            //Another char other than y or n.
            printf("Char error\n");
            goto free_will;
        }
    }
}

//This function send half of the message to the server.
void smts(char *half_file, int socket_fd)
{
    int bytes_sent = send(socket_fd, half_file, FILE_SIZE / 2, 0);
    if (bytes_sent == -1)
    {
        printf("send() failed with error code : %d", errno);
        close(socket_fd);
        exit(1);
    }
    else if (bytes_sent == 0)
    {
        printf("peer has closed the TCP connection prior to send().\n");
    }
    else if (bytes_sent < FILE_SIZE / 2)
    {
        printf("sent only %d bytes from the required %d.\n", FILE_SIZE / 2, bytes_sent);
    }
    else
    {
        printf("message was successfully sent.\n");
    }
}