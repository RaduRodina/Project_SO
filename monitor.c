// monitor.c (Windows-compatible, full implementation)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <io.h>

#define CMD_FILE "cmd.txt"
#define TREASURE_FILE "treasures.dat"

BOOL running = TRUE;

BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        printf("\n[Monitor] Received stop signal. Exiting after delay...\n");
        Sleep(2000);  // Delay for demonstration
        running = FALSE;
        return TRUE;
    } else if (signal == CTRL_BREAK_EVENT) {
        FILE *fp = fopen(CMD_FILE, "r");
        if (!fp) {
            perror("[Monitor] Failed to open command file");
            return TRUE;
        }

        char cmd[128];
        fgets(cmd, sizeof(cmd), fp);
        fclose(fp);
        cmd[strcspn(cmd, "\n")] = 0;

        char command[256];

        if (strcmp(cmd, "list_hunts") == 0) {
            struct _finddata_t c_file;
            intptr_t hFile;
            int count = 0;

            if ((hFile = _findfirst("*", &c_file)) == -1L) {
                printf("[Monitor] No files or directories found.\n");
            } else {
                do {
                    if ((c_file.attrib & _A_SUBDIR) && strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0) {
                        char path[256];
                        snprintf(path, sizeof(path), "%s/%s", c_file.name, TREASURE_FILE);
                        FILE *hunt_fp = fopen(path, "rb");
                        if (hunt_fp) {
                            int treasure_count = 0;
                            fseek(hunt_fp, 0, SEEK_END);
                            long size = ftell(hunt_fp);
                            fclose(hunt_fp);
                            treasure_count = size / sizeof(struct {char id[16];char user[32];float lat;float lon;char clue[128];int value;});
                            printf("Hunt: %s | Treasures: %d\n", c_file.name, treasure_count);
                            count++;
                        }
                    }
                } while (_findnext(hFile, &c_file) == 0);
                _findclose(hFile);

                if (count == 0) printf("[Monitor] No valid hunts found.\n");
            }
        } else if (strncmp(cmd, "list_treasures", 14) == 0) {
            char hunt[64];
            sscanf(cmd, "list_treasures %63s", hunt);
            snprintf(command, sizeof(command), "treasure_manager.exe --list %s", hunt);
            system(command);
        } else if (strncmp(cmd, "view_treasure", 13) == 0) {
            char hunt[64], tid[64];
            sscanf(cmd, "view_treasure %63s %63s", hunt, tid);
            snprintf(command, sizeof(command), "treasure_manager.exe --view %s %s", hunt, tid);
            system(command);
        } else {
            printf("[Monitor] Unknown command: %s\n", cmd);
        }
        return TRUE;
    }
    return FALSE;
}

int main() {
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        fprintf(stderr, "[Monitor] Error setting control handler\n");
        return 1;
    }

    printf("[Monitor] Ready. Waiting for commands from hub...\n");
    while (running) {
        Sleep(100);
    }
    printf("[Monitor] Exiting.\n");
    return 0;
}