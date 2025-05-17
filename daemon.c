#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

void daemonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);

    chdir("/home/andywu2/mud_project");

    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }

    int fd0 = open("/dev/null", O_RDWR);
    dup2(fd0, STDIN_FILENO);
    dup2(fd0, STDOUT_FILENO);
    dup2(fd0, STDERR_FILENO);
}

int run_command(const char *cmd) {
    int status = system(cmd);
    if (status == -1) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

int main() {
    daemonize();

    while (1) {
        if (run_command("./make_map.sh mud-andy nicholas AaronMap GabrielMap jenson") != 0) {
            break;
        }

        if (run_command("./mud") != 0) {
            break;
        }

        if (run_command("rm -r new_map") != 0) {
            break;
        }

        sleep(5);
        break;

    }

    printf("Exiting Daemon\n");
    exit(EXIT_SUCCESS);
}
