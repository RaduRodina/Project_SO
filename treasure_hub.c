// treasure_hub_windows.c (Windows-compatible version)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define CMD_FILE "cmd.txt"

PROCESS_INFORMATION monitor_info;
int monitor_running = 0;

void write_command(const char *cmd) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("Failed to open cmd file");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

void check_monitor_exit() {
    DWORD exitCode;
    if (monitor_running && GetExitCodeProcess(monitor_info.hProcess, &exitCode)) {
        if (exitCode != STILL_ACTIVE) {
            CloseHandle(monitor_info.hProcess);
            CloseHandle(monitor_info.hThread);
            monitor_running = 0;
            printf("\n[Monitor process terminated]\n");
        }
    }
}

int main() {
    char input[128];

    while (1) {
        check_monitor_exit();
        printf("hub> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor is already running.\n");
                continue;
            }
            STARTUPINFO si = {0};
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&monitor_info, sizeof(monitor_info));

            if (!CreateProcess(NULL, "monitor.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &monitor_info)) {
                fprintf(stderr, "Failed to start monitor process.\n");
            } else {
                monitor_running = 1;
                printf("Monitor started with PID %lu\n", (unsigned long)monitor_info.dwProcessId);
            }
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command("list_hunts");
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, monitor_info.dwProcessId);
        } else if (strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command(input);
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, monitor_info.dwProcessId);
        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, monitor_info.dwProcessId);
        } else if (strcmp(input, "exit") == 0) {
            check_monitor_exit();
            if (monitor_running) {
                printf("Cannot exit while monitor is running. Use stop_monitor first.\n");
                continue;
            }
            break;
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}