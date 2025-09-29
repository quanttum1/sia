#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>

int getch_nonblock(void) {
    if (_kbhit()) {
        return _getch();
    }
    return -1;
}

#else // POSIX
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

static struct termios oldt, newt;

void initTermios(void) {
    tcgetattr(STDIN_FILENO, &oldt); // Save old settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable canonical & echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Set non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void resetTermios(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore old settings
}

int getch_nonblock(void) {
    int c = getchar();
    if (c == EOF) return -1;
    return c;
}

#endif

// TODO: I feel I could make it simpler, but if I do it without getch_nonblock, it
// won't work because of that initTermios(). I may revisit this in the future
int get_int(const char *prompt) {
    int result = 0;
    int digit_count = 0;

    while (1) {
        printf("%s", prompt);
        while (1) {
            int c = getch_nonblock();

            if (c != -1) {
                if (c == 127) { // Backspace pressed
                    if (digit_count == 0) continue;
                    printf("\b \b");
                    result /= 10;
                    digit_count--;
                }

                if (c == '\n') break;
                if (c < 48 || c > 57) {
                    continue;
                }

                if (digit_count == 9) continue;
                printf("%c", c);
                result *= 10;
                result += c - 48;
                digit_count++;
            }
            usleep(50000);
        }
        printf("\n");

        if (digit_count == 0) {
            printf("Enter a valid number ^-^\n");
            continue;
        }
        break;
    }

    return result;
}

// Function to get UTF-8 character length from the first byte
int utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;         // 0xxxxxxx -> ASCII
    else if ((c & 0xE0) == 0xC0) return 2; // 110xxxxx
    else if ((c & 0xF0) == 0xE0) return 3; // 1110xxxx
    else if ((c & 0xF8) == 0xF0) return 4; // 11110xxx
    return 1; // Fallback for invalid UTF-8
}

char *read_entire_file(char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Sowwy, can't read file '%s': %s\n", file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    char *buffer = malloc(size + 1);
    rewind(file);

    fread(buffer, size, 1, file);

    fclose(file);
    buffer[size] = '\0';
    return buffer;
}

int main(int argc, char **argv) {
#ifndef _WIN32
    initTermios();
    atexit(resetTermios);
#endif

    if (argc != 2) {
        // TODO: Maybe ask the path to a file if not provided?
        printf("Usage: %s <file to read>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *text = read_entire_file(argv[1]);

    printf("Controls:\n");
    printf("Space  → Pause/read\n");
    printf("q      → Quit\n");
    printf("-      → Slower (-200 cpm)\n");
    printf("+ or = → Faster (+200 cpm)\n");
    printf("\n");
    printf("Be aware that the file '%s' will be modified when you quit the app, removing everything you've already read so you can continue where you left off next time.", argv[1]);
    printf("You may want to copy it. You can press Ctrl-C to quit the program before you start reading if you want to do that. You will start reading immediately after you enter the speed\n");
    printf("Happy reading! :3\n");
    printf("\n");

    int speed = get_int("Enter speed (characters per minute, ~1000 recommended): ");

    int time = 0;
    char *current_word;
    size_t word_len_bytes;
    size_t word_len_symbols; // Number of symbols to print, because of UTF-8
    bool paused = false;
    for (size_t i = 0; i < strlen(text);) {
        if (time == 0) {
            for (; i < strlen(text) && (text[i] == ' ' || text[i] == '\n'); i++) {
                printf("%c", text[i]);
            }

            current_word = text + i;
            word_len_bytes = 0;
            word_len_symbols = 0;

            for (; i < strlen(text) && !(text[i] == ' ' || text[i] == '\n'); i++) {
                printf(" ");
                word_len_symbols++;
                for (size_t j = 0; j < utf8_char_len(text[i]); j++) {
                    word_len_bytes++;
                }
                i += utf8_char_len(text[i]) - 1;
            }
            // By writing spaces for each letter of the word and then going back,
            // we make sure that the word is printed on the next line if it doesn't fit here
            for (size_t j = 0; j < word_len_symbols; j++) {
                printf("\b");
            }

            // TODO: Without this, at the end of lines 2 words sometimes concatenate
            // But with it, sometimes the last letter of a word is removed 👍🥲
            // For now I leave it as is, but in the future, I will use
            // terminal size for line break to ensure words don't get broken
            // at the end of lines
            printf("\b ");

            // TODO: Respect $NO_COLOR
            printf("\033[32m"); // Starts printing green
            for (size_t j = 0; j < word_len_bytes; j++) {
                printf("%c", current_word[j]);
            }
            printf("\033[0m"); // Resets colours

            fflush(stdout);
        }

        // TODO: Don't use usleep
        usleep(1000);
        if (!paused) time += 1000;

        int c = getch_nonblock();
        if (c == 'q') {
            // TODO: Right now, I save the progress by simply removing everything
            // that is read. In the future, I should insert a "bookmark" instead
            // Additionally, it'll save the speed user had last time
            FILE *file = fopen(argv[1], "w");

            if (file == NULL) {
                printf("Sowwy, couldn't save your reading progress 😭: %s\n", strerror(errno));
                break;
            }

            fprintf(file, "%s", text + i);
            fclose(file);
            printf("\n");
            break;
        }

        if (c == ' ') {
            paused = !paused;
        }

        if (c == '-') {
            speed -= 200;
        }
        if (c == '+' || c == '=') {
            speed += 200;
        }

        if (time >= 60.0 * 1000 * 1000 * word_len_symbols / speed) {
            // Rewriting current word in white
            for (size_t j = 0; j < word_len_symbols; j++) {
                printf("\b");
            }
            for (size_t j = 0; j < word_len_bytes; j++) {
                printf("%c", current_word[j]);
            }

            fflush(stdout);
            time = 0;
        }
    }

    printf("Bye-bye! :3\n");
    return EXIT_SUCCESS;
}
