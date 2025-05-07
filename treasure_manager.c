#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


#define TREASURE_ID_SIZE 16
#define USERNAME_SIZE 32
#define CLUE_TEXT_SIZE 128
#define MAX_FILE_PATH 256
#define N 10000
#define D 200
#define TREASURE_FILE "treasures.dat"



typedef struct {
    char id[TREASURE_ID_SIZE];
    char userName[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_TEXT_SIZE];
    int value;
}Treasure;

void log_operation(const char *hunt_id, const char *message) {
    char log_path[MAX_FILE_PATH];
    snprintf(log_path, MAX_FILE_PATH, "%s/log.txt", hunt_id);

    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    char time_str[64];
    snprintf(time_str, sizeof(time_str), "[%s] ", ctime(&now));

    write(log_fd, time_str, strlen(time_str)); //Write timestamp
    write(log_fd, message, strlen(message)); //Write message
    write(log_fd, "\n", 1); //Newline for separation
    close(log_fd);
}


//DE CITIT
void add_treasure(const char *hunt_id) {

    mkdir(hunt_id);
    char file_path[MAX_FILE_PATH];
    snprintf(file_path, MAX_FILE_PATH, "%s/%s", hunt_id, TREASURE_FILE);
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening file for writing");
        return;
    }

    Treasure treasure;
    printf("Enter Treasure ID: ");
    scanf("%s", treasure.id);
    printf("Enter Username: ");
    scanf("%s", treasure.userName);
    printf("Enter latitude: ");
    scanf("%f", &treasure.latitude);
    printf("Enter longitude: ");
    scanf("%f", &treasure.longitude);
    printf("Enter clue: ");
    getchar();

    fgets(treasure.clue, CLUE_TEXT_SIZE, stdin);
    treasure.clue[strcspn(treasure.clue, "\n")] = 0;

    printf("Enter Treasure Value: ");
    scanf("%d", &treasure.value);

    write(fd, &treasure, sizeof(Treasure));
    close(fd);

    log_operation(hunt_id, "Added new Treasure");
}

//DE CITIT
void list_treasures(const char *hunt_id) {
    char file_path[MAX_FILE_PATH];
    snprintf(file_path, MAX_FILE_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("Error opening file for reading");
        return;
    }

    printf("Hunt ID: %s\n", hunt_id);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Last Modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file for reading");
        return;
    }

    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("Treasure ID: %s\n | User: %s | GPS: %.2f, %.2f | Value: %d | Clue: %s\n",
            treasure.id, treasure.userName, treasure.latitude, treasure.longitude, treasure.value, treasure.clue);
    }
    close(fd);

    log_operation(hunt_id, "Listed treasure");
}

void view_treasures(const char *hunt_id, const char *treasure_id) {
    char file_path[MAX_FILE_PATH];
    snprintf(file_path, MAX_FILE_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file for reading");
        return;
    }
    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(treasure.id, treasure_id) == 0) {
            printf("ID: %s\nUser: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n",
                   treasure.id, treasure.userName, treasure.latitude, treasure.longitude, treasure.clue, treasure.value);
            close(fd);
            return;
        }
    }
    printf("Treasure not found\n");
    close(fd);
}

void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char file_path[MAX_FILE_PATH];

    snprintf(file_path, MAX_FILE_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file for reading");
        return;
    }

    int tmp_fd = open("tmp.dat", O_WRONLY | O_CREAT | O_TRUNC);
    if (tmp_fd < 0) {
        perror("Failed to open file for writing");
        close(fd);
        return;
    }

    Treasure treasure;
    int found = 0;

    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(treasure.id, treasure_id) != 0) {
            write(tmp_fd, &treasure, sizeof(Treasure));
        }else {
            found = 1;
        }
    }
    close(fd);
    close(tmp_fd);

    if (found == 1) {
        remove(file_path);
        rename("tmp.dat", file_path);
        log_operation(hunt_id, "Removed a treasure.");
    }
    else {
        printf("Treasure not found\n");
        remove("tmp.dat");
    }
}


void remove_hunt(const char *hunt_id) {
    char path[MAX_FILE_PATH];
    snprintf(path, MAX_FILE_PATH, "%s/%s", hunt_id, TREASURE_FILE);
    remove(path);

    rmdir(hunt_id);

    char link_name[MAX_FILE_PATH];
    snprintf(link_name, MAX_FILE_PATH, "logged_hunt-%s", hunt_id);
    unlink(link_name);

    printf("%s removed.\n", hunt_id);
}



int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --<command> <hunt_id> [<treasure_id>]\n", argv[0]);
        return 1;
    }

    const char *op = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(op, "--add") == 0) {
        add_treasure(hunt_id);
    }else if (strcmp(op, "--list") == 0) {
        list_treasures(hunt_id);
    }else if (strcmp(op, "--view") == 0 && argc == 4) {
        view_treasures(hunt_id, argv[3]);
    }else if (strcmp(op, "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(hunt_id, argv[3]);
    }else if (strcmp(op, "--remove") == 0) {
        remove_hunt(hunt_id);
    }else {
        fprintf(stderr, "Invalid command.\n");
        return 1;
    }
    return 0;
}