#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <process.h>

#define TREASURE_FILE "treasures.dat"
#define TREASURE_ID_SIZE 16
#define USERNAME_SIZE 32
#define CLUE_TEXT_SIZE 128

typedef struct {
    char id[TREASURE_ID_SIZE];
    char userName[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_TEXT_SIZE];
    int value;
} Treasure;

typedef struct UserScore {
    char user[USERNAME_SIZE];
    int score;
    struct UserScore *next;
} UserScore;

UserScore* add_score(UserScore *head, const char *user, int value) {
    UserScore *current = head;
    while (current) {
        if (strcmp(current->user, user) == 0) {
            current->score += value;
            return head;
        }
        current = current->next;
    }
    UserScore *new_node = malloc(sizeof(UserScore));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new user");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->user, user, USERNAME_SIZE - 1);
    new_node->user[USERNAME_SIZE - 1] = '\0';
    new_node->score = value;
    new_node->next = head;
    return new_node;
}

void free_scores(UserScore *head) {
    while (head) {
        UserScore *tmp = head;
        head = head->next;
        free(tmp);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]); fflush(stdout);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s\\%s", argv[1], TREASURE_FILE); // Use Windows-style path
    fflush(stdout);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open treasure file: %s\n", path);
        return 1;
    }

    Treasure t;
    UserScore *head = NULL;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        head = add_score(head, t.userName, t.value);
    }
    close(fd);

    int total = 0;
    printf("Hunt: %s\n", argv[1]); fflush(stdout);
    for (UserScore *cur = head; cur; cur = cur->next) {
        printf("%s: %d\n", cur->user, cur->score); fflush(stdout);
        total += cur->score;
    }
    printf("Total Treasure Value: %d\n\n", total); fflush(stdout);

    free_scores(head);
    return 0;
}