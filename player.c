#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#define MAX_HISTORY 100
#define START_DIR "new_map/east" // This should be the directory of the first room
#define DESCRIPTION_FILE "des.txt" // name for the room description file
#define ITEM_FILE "item.txt" // name for the item file
//#define PATH_MAX 4096

#ifdef _WIN32
#define OS_TYPE "windows"
#else
#define OS_TYPE "unix"
#endif


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
    while (fgets(line, sizeof(line), file)) { // PUBLISH TO MQTT TOPIC HERE THE BUFFER IS LINE
        printf("%s", line);
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

    // Change to the starting room directory (e.g., "start2")
    if (chdir(START_DIR) != 0) {
        perror("chdir to start");
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
        clear_screen(OS_TYPE);

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
        printf("\nEnter direction (or q to quit): ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0'; // Remove newline from input

        // Exit game if player enters 'q'
        if (strcmp(input, "q") == 0) break;

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
            printf("You bump into a wall. That direction doesn't go anywhere.\n"); // YOU CAN ALSO PUBLISH THIS TO MQTT
        }
    }
    // Free up all memory used by the history stack
    cleanup_history();
    return 0;
}
