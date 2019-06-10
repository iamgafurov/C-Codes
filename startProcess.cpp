//
// Start a process and kill it after timeout,
// if the process hangs.
// The process command line and timeout in milliseconds
// are given in a command line:
//    startproc -t 2000 "myproc.exe 123456"
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const unsigned int DEFAULT_TIMEOUT = 5000;  // 5 sec. in ms

static void printHelp();

int main(int argc, char *argv[]) {
    unsigned int timeout = DEFAULT_TIMEOUT;
    char processPath[1024];
    char processCommandLine[1024];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (argc <= 1) {
        printHelp();
        return 1;
    }

    // Parse a command line
    int argIdx = 1;
    if (strncmp(argv[argIdx], "-t", 2) == 0) {
        const char *p = argv[argIdx] + 2;
        if (*p == 0) {
            // Go to the next argument
            if (argIdx >= argc - 1) {
                fprintf(stderr, "Incorrect command line.\n");
                printHelp();
                return (-1);
            }
            ++argIdx;
            p = argv[argIdx];
        }
        timeout = atoi(p);
        ++argIdx;
    }

    if (argIdx >= argc) {
        fprintf(stderr, "Incorrect command line.\n\n");
        printHelp();
        return (-1);
    }

    strncpy(processCommandLine, argv[argIdx], 1022);
    processCommandLine[1022] = 0; // for safety

    // Extract the process path from the command line
    const char *p = processCommandLine;
    char *q = processPath;
    if (*p == '\"') {
        ++p;
        while (*p != 0 && *p != '\"') {
            *q = *p; ++q; ++p;
        }
        if (*p == 0) {
            fprintf(stderr, "Incorrect command line.\n\n");
            return (-1);
        }
        *q = 0;
    } else {
        while (*p != 0 && !isspace(*p)) {
            *q = *p; ++q; ++p;
        }
        *q = 0;
    }

    // Start a process
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcess(
        processPath,   // Path to exe-file
        processCommandLine, // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Do not inherit handles
        0, // CREATE_NEW_CONSOLE, // Creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure
    ))
    {
        fprintf(
            stderr, "CreateProcess failed: error %d\n",
            GetLastError()
        );
        return (-1);
    }

    // Wait until the process terminates
    DWORD res = WaitForSingleObject(pi.hProcess, timeout);

    int finalRes = 0;
    if (res == WAIT_OBJECT_0) {
        printf("Process finished successfully.\n");
    } else if (res == WAIT_TIMEOUT) {
        // Terminate the process after timeout
        BOOL terminated = TerminateProcess(
            pi.hProcess,
            UINT(-1)
        );
        if (!terminated) {
            fprintf(
                stderr, "Cannot terminate a process: error %d\n",
                GetLastError()
            );
            return (-1);
        }
        // Wait until the terminate command is completed
        // (maximum 10 s)
        WaitForSingleObject(pi.hProcess, 10000);
        fprintf(stderr, "Process is terminated after timeout.\n");
        finalRes = 1;
    } else {
        fprintf(
            stderr, "WaitForSingleObject error: %d\n",
            GetLastError()
        );
        return (-1);
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return finalRes;
}

static void printHelp() {
    printf(
        "Start an external process and kill it after timeout,\n"
        "if the process hangs.\n"
        "Usage:\n"
        "    startproc [-t timeout_ms] process_command_line\n"
        "Defaut timeout is 5000 ms (5 seconds).\n"
        "The path to exe-file is extracted from the command line.\n"
        "Example:\n"
        "    startproc -t 2000 \"myproc.exe 123456\"\n"
    );
}
