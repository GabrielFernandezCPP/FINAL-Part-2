#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>

#define MAX_HISTORY 100
#define START_DIR "/home/andywu2/mud_project/new_map/east" 
#define DESCRIPTION_FILE "des.txt"
#define ITEM_FILE "item.txt"
#define PORT 8888
#define MAX_LEN 1024
#define BROKER "34.94.91.89"
#define topic "MUD/playerMove"

#ifdef _WIN32
#define OS_TYPE "windows"
#else
#define OS_TYPE "unix"
#endif

// TCP SERVER CODE

int connfd = -1, server_sock = -1;

void handle_sigint(int sig) {
    if (connfd != -1) close(connfd);
    if (server_sock != -1) close(server_sock);
    printf("\nSockets closed. Exiting cleanly.\n");
    exit(0);
}

int setup_tcp_server() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 1) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for ESP32 to connect on port %d...\n", PORT);
    return sockfd;
}

int accept_client(int sockfd) {
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    int connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
    if (connfd < 0) {
        perror("Accept failed");
        return -1;
    }

    printf("ESP32 connected");
    return connfd;
}

// TCP SERVER CODE


// Clear the screen based on OS type
void clear_screen(const char *os) {
    if (strcmp(os, "windows") == 0) {
        system("cls");
    } else {
        // Assume Unix-like: Linux, macOS, etc.
        printf("\033[2J\033[H");
        fflush(stdout);
    }
}

typedef struct {
    char *path;
    char back_direction[16]; // Direction that gets you BACK here
} RoomState;

RoomState *history[MAX_HISTORY];
int history_top = -1;

char command[512];

// List all directories in the current room
// If debug is 1, show all entries; if 0, show only directories
void list_directory(const char *path, int debug) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat st;
    char fullpath[PATH_MAX];

    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construct the full path of the entry
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // Retrieve file status
        if (stat(fullpath, &st) == 0) {
            if (debug) {
                // In debug mode, display all entries
                printf("  %s%s\n", entry->d_name, S_ISDIR(st.st_mode) ? "/" : "");
            } else {
                // In non-debug mode, display only directories without trailing '/'
                if (S_ISDIR(st.st_mode)) {
                    printf("  %s\n", entry->d_name);
                }
            }
        }
    }

    closedir(dir);
}


// Show room description from des.txt
int print_file_contents(const char *filename) {
    FILE *file = fopen(filename, "r");
    char line[256];
    if (!file) return 0;

    printf("\n");
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);

        line[strcspn(line, "\n")] = '\0';

        snprintf(command, sizeof(command),
                 "mosquitto_pub -h %s -t %s -m \"%s\"",
                 BROKER, topic, line);

        system(command);
    }
    printf("\n");

    fclose(file);
    return 1;
}

// Map direction -> opposite
// Returns the opposite direction for backtracking
const char* opposite_direction(const char *dir) {
    if (strcmp(dir, "north") == 0) return "south";
    if (strcmp(dir, "south") == 0) return "north";
    if (strcmp(dir, "east") == 0)  return "west";
    if (strcmp(dir, "west") == 0)  return "east";
    return NULL;
}

// Push room onto history stack
void push_history(const char *path, const char *back_direction) {
    if (history_top >= MAX_HISTORY - 1) return;
    RoomState *state = malloc(sizeof(RoomState));
    state->path = strdup(path);
    strncpy(state->back_direction, back_direction, sizeof(state->back_direction));
    history[++history_top] = state;
}

// Pop room from history stack
RoomState* pop_history() {
    if (history_top < 0) return NULL;
    return history[history_top--];
}

// Free memory
void cleanup_history() {
    while (history_top >= 0) {
        free(history[history_top]->path);
        free(history[history_top--]);
    }
}

int main() {
    char cwd[PATH_MAX], input[PATH_MAX];
    int bytes_read;

    // Change to the starting room directory (e.g., "start2")
    if (chdir(START_DIR) != 0) {
        perror("chdir to start");
        return 1;
    }

    server_sock = setup_tcp_server();

    connfd = accept_client(server_sock);
    if (connfd < 0) {
        close(server_sock);
        return 1;
    }

    // Main game loop
    while (1) {
        // Get the current working directory (room)
        if (!getcwd(cwd, sizeof(cwd))) {
            perror("getcwd");
            break;
        }

        // Clear the screen for a clean view (depends on OS)
        printf("\n");

        // If item.txt exists in the room, display its contents and end the game (e.g., player found the item)
        if (print_file_contents(ITEM_FILE) == 1) {
            break;
        }

        // Print room description from des.txt
        print_file_contents(DESCRIPTION_FILE);

        // List available directions (subdirectories)
        printf("\n");
        list_directory(cwd, 0);

        // Prints back direction if available
        if (history_top >= 0 && strlen(history[history_top]->back_direction) > 0) {
            printf("  %s \n", history[history_top]->back_direction);
        }

        // Prompt the player for input
        printf("\nEnter direction: ");
        fflush(stdout);
        
        // Read move from ESP32 via TCP (blocking)
        memset(input, 0, sizeof(input));
        bytes_read = read(connfd, input, sizeof(input) - 1);
        if (bytes_read <= 0) {
            printf("ESP32 disconnected or error reading\n");
            break;
        }
        printf("Received direction: '%s'\n", input);

        input[bytes_read] = '\0';  // Null-terminate the string

        // Strip newline or carriage return if present
        input[strcspn(input, "\r\n")] = '\0';

        // Get the opposite direction to enable backtracking
        const char *back = opposite_direction(input);

        // Check if input is a valid subdirectory (a valid move)
        struct stat st;
        if (stat(input, &st) == 0 && S_ISDIR(st.st_mode)) {
            // Save current room to history stack before moving
            getcwd(cwd, sizeof(cwd));
            push_history(cwd, back ? back : "");

            // Change into the new room directory
            if (chdir(input) != 0) {
                perror("chdir");
            }
        } 
        // Check if player wants to go back using the reverse direction
        else if (history_top >= 0 &&
                strcmp(input, history[history_top]->back_direction) == 0) {
            // Pop previous room from history stack and go back
            RoomState *prev = pop_history();
            if (chdir(prev->path) != 0) {
                perror("chdir");
            }
            free(prev->path);
            free(prev);
        }   
        // Invalid direction input
        else {
            char invalid[128] = "You bump into a wall. That direction does not go anywhere.\n"; // YOU CAN ALSO PUBLISH THIS TO MQTT
            snprintf(command, sizeof(command),
                 "mosquitto_pub -h %s -t %s -m '%s'",
                 BROKER, topic, invalid);
            system(command);
        }
    }
    // Free up all memory used by the history stack
    cleanup_history();
    close(connfd);
    close(server_sock);
    return 0;
}
