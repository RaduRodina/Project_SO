#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <process.h>

#define CMD_FILE "cmd.txt"

HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
PROCESS_INFORMATION monitor_info;
int monitor_running = 0;

void write_command(const char *cmd) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("Failed to open command file");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

void CreateMonitorWithPipe() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0);
    SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&monitor_info, sizeof(PROCESS_INFORMATION));

    CreateProcess(
        NULL,
        "monitor.exe",
        NULL,
        NULL,
        TRUE,
        CREATE_NEW_PROCESS_GROUP,
        NULL,
        NULL,
        &siStartInfo,
        &monitor_info
    );

    monitor_running = 1;
    CloseHandle(g_hChildStd_OUT_Wr);
}

void ReadFromPipe() {
    CHAR buffer[4096];
    DWORD bytesRead;
    DWORD available;
    BOOL success;
    int quiet_cycles = 0;

    while (quiet_cycles < 10) {
        if (!PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &available, NULL)) break;

        if (available == 0) {
            Sleep(100);
            quiet_cycles++;
            continue;
        }

        success = ReadFile(g_hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (!success || bytesRead == 0) break;

        buffer[bytesRead] = '\0';
        printf("%s", buffer);
        fflush(stdout);
        quiet_cycles = 0;
    }
}

void run_score_calculator(const char *hunt_id) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "score_calculator.exe %s", hunt_id);
    FILE *fp = _popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Failed to run score calculator for %s\n", hunt_id);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
        fflush(stdout);
    }
    _pclose(fp);
}

void calculate_scores() {
    struct _finddata_t c_file;
    intptr_t hFile;

    if ((hFile = _findfirst("*", &c_file)) == -1L) {
        printf("No hunts found.\n");
        return;
    } else {
        do {
            if ((c_file.attrib & _A_SUBDIR) &&
                strcmp(c_file.name, ".") != 0 &&
                strcmp(c_file.name, "..") != 0) {
                run_score_calculator(c_file.name);
            }
        } while (_findnext(hFile, &c_file) == 0);
        _findclose(hFile);
    }
}

int count_tokens(char *str) {
    int count = 0;
    char *tok = strtok(str, " ");
    while (tok) {
        count++;
        tok = strtok(NULL, " ");
    }
    return count;
}

int main() {
    char input[128];

    while (1) {
        printf("hub> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor is already running.\n");
            } else {
                CreateMonitorWithPipe();
                printf("Monitor started with PID %lu\n", (unsigned long)monitor_info.dwProcessId);
            }
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command("list_hunts");
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, monitor_info.dwProcessId);
            ReadFromPipe();
        } else if (strncmp(input, "list_treasures", 14) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            char temp[128];
            strcpy(temp, input);
            if (count_tokens(temp) != 2) {
                printf("Usage: list_treasures <hunt_id>\n");
                continue;
            }
            write_command(input);
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, monitor_info.dwProcessId);
            ReadFromPipe();
        } else if (strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            char temp[128];
            strcpy(temp, input);
            if (count_tokens(temp) != 3) {
                printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
                continue;
            }
            write_command(input);
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, monitor_info.dwProcessId);
            ReadFromPipe();
        } else if (strcmp(input, "calculate_score") == 0) {
            calculate_scores();
        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, monitor_info.dwProcessId);
            printf("Waiting for monitor to exit...\n");

            DWORD result = WaitForSingleObject(monitor_info.hProcess, 5000); // wait 5 seconds
            if (result == WAIT_OBJECT_0) {
                printf("Monitor terminated successfully.\n");
            } else if (result == WAIT_TIMEOUT) {
                printf("Monitor did not terminate in time. Forcing shutdown.\n");
                TerminateProcess(monitor_info.hProcess, 1);
            }

            CloseHandle(monitor_info.hProcess);
            CloseHandle(monitor_info.hThread);
            monitor_running = 0;
        } else if (strcmp(input, "exit") == 0) {
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