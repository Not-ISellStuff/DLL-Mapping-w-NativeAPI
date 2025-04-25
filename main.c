#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tlhelp32.h>

#include "loader.h"

//////////////////////////////////////////////////////////////////////////////////

char* PARG(char** args, const char* flag, int argsa)
{
    if (!args || !flag || argsa <= 0)
        return NULL;

    for (int i = 0; i < argsa - 1; i++)  
    {
        if (strcmp(args[i], flag) == 0)
        {
            return args[i + 1]; 
        }
    }

    return NULL; 
}

DWORD PID(const char* pname) {
    PROCESSENTRY32 proce;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hsnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    proce.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hsnap, &proce)) {
        do {
            if (strcmp(pname, proce.szExeFile) == 0) {
                CloseHandle(hsnap);
                return proce.th32ProcessID;
            }
        } while (Process32Next(hsnap, &proce));
    }

    CloseHandle(hsnap);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    const char* dll = PARG(argv, "-D", argc);
    const char* pname = PARG(argv, "-P", argc);

    if (dll == NULL || pname == NULL) {
        printf("[!] Missing required arguments.\n");
        return EXIT_FAILURE;
    }

    const DWORD pid = PID(pname);
    if (pid == 0) {
        printf("[!] Failed To Find The Process --> %s\n", pname);
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////

    if (!Load(pid, dll)) {
        printf("\n\n[!] Failed To Inject DLL.");
        exit(EXIT_FAILURE);
    }

    /////////////////////////////////////////

    printf("\n\n[!] Successfully Injected DLL | Process ID: -> %d | DLL Path -> %s", pid, &dll);
    exit(EXIT_SUCCESS);
}
