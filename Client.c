/**
 * Client program used to send echo messages asynchronously to the server.
 * The received echos are used to calculate round trip statistics.
 *
 * Usage: client <Server Address> <Repeat times> [<Server Port>]
 *
 * Author: Kenneth Chik 2012.
 */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include "ErrorHandle.h"

static const int RECEIVE_TIMEOUT_SEC = 5; //Amount of time to wait for connection to time out.

//Passed into the receive_connection thread.
struct receive_param {
    int socket; //Socket to receive data from.
    int repeat; //Number of characters that were sent.
    int exit;   //Used to indicate whether the thread has exited.
    struct timeval *endTimes; //Stores the times the data messages were received.
};

/**
 * Calculated the time difference between endTime and startTime (endTime-startTime).
 * Return: Time difference in milliseconds.
 */
long time_diff(struct timeval startTime, struct timeval endTime) {
    long secs, usecs;
    secs  = endTime.tv_sec - startTime.tv_sec;
    usecs = endTime.tv_usec - startTime.tv_usec;
    return ((secs) * 1000 + usecs/1000.0) + 0.5;
}

/**
 * Prints the resulting statistics of the:
 * 1. Round trip time for each echo message.
 * 2. Average round trip time.
 * 3. Throughput rate for all messages.
 */
void print_results(struct timeval *startTimes, struct timeval *endTimes, int length) {
    printf("Round trip time for each message rounded to the nearest millisecond:\n");
    long total_time_used=0;    
    struct timeval lastArrive=endTimes[0]; //Stores the last echo time received.
    long mtime;
    int total_results=0;
    int i;
    for (i=0;i<length;++i) {        
        mtime = time_diff(startTimes[i],endTimes[i]);
        if (mtime >= 0) {       
            total_time_used += mtime;
            total_results++;
            printf("message %i: %ld milli seconds\n",i+1,mtime);
        }
        if (time_diff(lastArrive,endTimes[i])>0) {
            lastArrive = endTimes[i];
        }        
        //printf("start time sec: %ld usec: %ld\n",(long)startTimes[i].tv_sec,(long)startTimes[i].tv_usec);
        //printf("end time sec: %ld usec: %ld\n",(long)endTimes[i].tv_sec,(long)endTimes[i].tv_usec);
    }
    printf("\n%d out of %d messages received.\n",total_results,length);
    if (total_results == 0) return;
    double average_round_trip_time = (total_time_used*1.0) / total_results;
    printf("\nRunning average round trip time: %f milli seconds\n",average_round_trip_time);
    double total_time_elapsed = time_diff(startTimes[0],lastArrive)/1000.0;
    if (total_time_elapsed != 0) {
        double throughput = length*CHAR_BIT/total_time_elapsed;
        printf("\nRunning throughput rate for all messages: %fkbps\n",throughput);
    }
}   

/**
 * Method used to receive echos and stores their received times.
 * The method is run from a thread so that sending can occur at the same time.
 */
void * receive_connection(void * param) {
    struct receive_param *p = (struct receive_param *)param;
    int sock = p->socket;
    int repeat = p->repeat;
    struct timeval *endTimes = p->endTimes;
    printf("Received: ");
    while (repeat > 0) {
        char buffer[1]; // buffer to store received data.
        ssize_t numBytes = recv(sock, buffer, sizeof(char), 0);
        if (numBytes < 0) {
            systemErrorExit("recv() failed");
        }
        else if (numBytes == 0) {
            userErrorExit("connection to server closed.");
        }
        if (gettimeofday(endTimes+(int)buffer[0]-1,NULL) == -1) {
            systemErrorExit("Problem getting time of receive day ");
        }             
        printf("%d ",(int)buffer[0]); //prints received data.
        repeat--;
    }
    printf("\n");

    close(sock);
    p->exit=1; //Sets exit condition so that main thread knows receive is finished.
    return NULL;
}

int main(int argc, char *argv[]) {
    //printf("Program Start\n");
    //Check for correct usage.
    if (argc < 3 || argc > 4) {
        userErrorExit("Usage: client <Server Address> <Repeat times> [<Server Port>]\n<Server Port> defaults to 7");
    }

    char *server_ip = argv[1];     // First argument: server IP address in numeric dot form.
    int repeat = atoi(argv[2]); // Second argument: number of times to send data.
    if (repeat <= 0) {
        userErrorExit("repeat number invalid");
    } else if (repeat > 100) {
        userErrorMsg("repeat number too large setting to 100");
        repeat = 100;
    }

    // Third argument (optional): server port (numeric).
    // 7 is the default echo port.
    in_port_t server_port = (argc == 4) ? atoi(argv[3]) : 7;

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        systemErrorExit("socket() failed");
    }
    //printf("Socket created.\n");

    struct sockaddr_in server_address;            // Server address structure used in connect function.
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;          // IPv4

    int ret = inet_pton(AF_INET, server_ip, &server_address.sin_addr.s_addr);
    if (ret == 0) {
        userErrorExit("inet_pton() failed: invalid address string");
    }
    else if (ret < 0) {
        systemErrorExit("inet_pton() failed");
    }
    server_address.sin_port = htons(server_port);    // Server port

    // Establish the connection to server
    if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        systemErrorExit("connecting to server failed");
    }
    //printf("Connected\n");
    
    pthread_t t1;
    int s;
    struct receive_param p; //construct paramter to pass to receive thread.
    p.socket=sock;
    p.repeat=repeat;
    p.exit=0;
    p.endTimes=(struct timeval *)calloc(repeat,sizeof(struct timeval));
    s = pthread_create(&t1,NULL,receive_connection,(void *)(&p)); //start receive thread.
    if (s != 0) {
        systemErrorExit("receive connection thread creation failed.");
    }
    //printf("Thread created.\n");

    // Send a series of characters to the echo server.
    char msg = 0;
    ssize_t numBytes;
    int i;
    struct timeval *startTimes=(struct timeval *)calloc(repeat,sizeof(struct timeval));
    for (i=0;i<repeat;i++) {
        if (gettimeofday(startTimes+(int)msg,NULL) == -1) { //record the send time.
            systemErrorExit("Problem getting time of sent day ");
        }
        //printf("sec: %ld usec: %ld \n", startTimes[(int)msg].tv_sec,startTimes[(int)msg].tv_usec);    
        msg++;
        numBytes = send(sock, &msg, sizeof(char), 0); //send a character.
        if (numBytes < 0) {
            systemErrorExit("send() failed");
        }
        else if (numBytes != sizeof(char)) {
            userErrorExit("The number of bytes sent is not correct");
        }
    }
    //printf("sending complete");
  
    //do a timed thread join with max wait of RECEIVE_TIMEOUT_SEC
    double wait = 0;
    while (p.exit == 0 && wait < RECEIVE_TIMEOUT_SEC) {
        usleep(100000);
        wait += 0.1;
        //printf("waiting");
    }
    if (wait >= RECEIVE_TIMEOUT_SEC) {
        printf("\n");
    }
    
    print_results(startTimes,p.endTimes,repeat);
    free(startTimes);
    free(p.endTimes);
    return 0;
}
