//compile with gcc tcpMqttServer.c -o tcpMqttServer.exe -lmosquitto
//if mosquitto library doesn't work do sudo apt install libmosquitto-dev

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mosquitto.h>
#include <unistd.h>
#define TCP_PORT 8888
#define MQTT_PORT 1883
#define MAX_LEN 1024
#define IP "34.168.132.202"

struct mosquitto *mosq = NULL;
const char *test_des = "This is just a test description to see if mqtt messaging works after recieving the TCP message.";
const char *topic = "MUD/playerMove";

void publish_msg(const char *msg) {
	if (!mosq) {
		fprintf(stderr, "Mosquitto client is not initialized.\n");
		return;
	}

	int rc = mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, false);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Failed to publish: %s\n", mosquitto_strerror(rc));
	}
}


int main() {
	mosquitto_lib_init();
	mosq = mosquitto_new("MUD", true, NULL);
	if (!mosq) {
		fprintf(stderr, "Failed to create mosquitto client.\n");
		return 1;
	}
	if (mosquitto_connect(mosq, IP, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Failed to connect to broker.\n");
		return 1;
	}
	mosquitto_loop_start(mosq);
	int sockfd, connfd;
	struct sockaddr_in servaddr, cliaddr;
	// Creating TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(TCP_PORT);
	// Binding socket with address and port
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		perror("Binding failed");
		exit(EXIT_FAILURE);
	}
	// Listening for incoming connections
	if (listen(sockfd, 5) != 0) {
		perror("Listening failed");
		exit(EXIT_FAILURE);
	}
	printf("Waiting for client to connect...\n");
	socklen_t len = sizeof(cliaddr);
	connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
	if (connfd < 0) {
		perror("Accept failed");
		exit(EXIT_FAILURE);
	}
	printf("Client connected.\n");
	char message[MAX_LEN];
	while (1) {
		// Receive message from peer 1
		read(connfd, message, sizeof(message));
		message[strcspn(message, "\r\n")] = 0;
		//printf("Peer 1: %s", message);
		//printf("You: ");
		//fgets(message, MAX_LEN, stdin);
		// Sending message to peer 1
		//write(connfd, message, strlen(message));

		if (strcmp(message, "N") == 0 || strcmp(message, "S") == 0 || strcmp(message, "W") == 0 || strcmp(message, "E") == 0) {
			publish_msg(test_des);
		}
	}
	mosquitto_loop_stop(mosq, true);
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	close(sockfd);
	return 0;
}