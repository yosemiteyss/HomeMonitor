#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <pigpio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dht11.h"

/* Server configuration */
#define SERVER_PORT             12345
#define LISTEN_NQ               10
#define MAX_LINE                100

/* Milliseconds between each sensor reading (default: 0.1s) */
#define DHT11_READ_INTERVAL     100

/* Seconds between each output to client (default: 1s) */
#define OUTPUT_INTERVAL         1

/* Threads */
#define MAX_THREADS             128

pthread_t threads[MAX_THREADS];
pthread_rwlock_t thread_lock;
int threads_count = 0;

/**
 * Callback function for reseting GPIO pins
 */
static void gpio_reset(void)
{
    gpioTerminate();
}

/**
 * Dedicated thread for reading sensor data from DHT11
 */ 
void* dht11_thread(void *args)
{
    dht11_start();

    /* Each read has an interval between 0.1s */
    while (1) {
        dht11_read();
        gpioDelay(DHT11_READ_INTERVAL * 1000);
    }
}

/**
 * Dedicated thread for handling client request.
 * 
 */
void* client_request(void *args)
{
    int connfd = (int) args;
    int write_bytes, total_bytes;
    int retval;
    char buffer[MAX_LINE];
    pthread_rwlock_t rwlock;

    while (1) {
        /* Parse sensor data */
        pthread_rwlock_rdlock(&rwlock);

        snprintf(buffer, MAX_LINE, "%d,%d,%d,%d\n", 
            dht11_output.temp_high, dht11_output.temp_low, 
            dht11_output.hum_high, dht11_output.hum_low
        );

        pthread_rwlock_unlock(&rwlock);

        write_bytes = 0;
        total_bytes = strlen(buffer);

        while (write_bytes < total_bytes) {
            int res = write(connfd, buffer + write_bytes, total_bytes - write_bytes);
            
            if (res == -1) {
                pthread_rwlock_wrlock(&thread_lock);
                --threads_count;
                pthread_rwlock_unlock(&thread_lock);

                fprintf(stderr, "Connection loss with fd: %d, thread_count: %d\n", connfd, threads_count);
                pthread_exit(&retval);
            }

            write_bytes += res;
        }

        sleep(OUTPUT_INTERVAL);
    }
}

int main(void)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(struct sockaddr_in);
    char ip_str[INET_ADDRSTRLEN] = {0};

    /* Reset GPIO when exit */
    atexit(gpio_reset);

    /* Initialize GPIO */
    if (gpioInitialise() == PI_INIT_FAILED) {
	    fprintf(stderr, "Failed to initialize GPIO\n");
	    exit(EXIT_FAILURE);
    }

    /* Create thread for DHT11 */
    if (pthread_create(&threads[threads_count++], NULL, dht11_thread, NULL) != 0) {
        fprintf(stderr, "Error creating DHT11 thread\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize server socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Failed to initialize server socket\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize server address */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);
    
    /* Bind the socket to the server address */
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Failed to bind server address: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, LISTEN_NQ) < 0) {
        fprintf(stderr, "Failed to listen server address: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Main Thread */
    while (1) {
        /* Accept incoming connections */
        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
            fprintf(stderr, "Failed to accept new connection: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
        
        /* Create dedicated thread to process the connection request */
        if (pthread_create(&threads[threads_count++], NULL, client_request, (void *) connfd) != 0) {
			fprintf(stderr, "Error creating thread for fd: %d\n", connfd);
            exit(EXIT_FAILURE);
		}

        printf("Incoming connection from %s : %hu with fd: %d, thread_count: %d\n", ip_str, ntohs(cliaddr.sin_port), connfd, threads_count);
    }

    return 0;
}
