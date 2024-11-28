#include <unistd.h>
#include <string.h>

int main() {

    const char *welcome_message = "Welcome to ENSEA (Alamo and Sabrina) Tiny Shell.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));

    const char *exit_message = "Type 'exit' to quit.\n";
    write(STDOUT_FILENO, exit_message, strlen(exit_message));

    const char *prompt = "enseash %% ";
    write(STDOUT_FILENO, prompt, strlen(prompt));

    return 0;
}
