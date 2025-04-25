#include <windows.h>
#include <stdio.h>

/////////////////////////////////////////////////

typedef unsigned long ULONG;
typedef void* HANDLE;
typedef void* PVOID;
typedef LONG NTSTATUS;
typedef ULONG ACCESS_MASK;
typedef HANDLE* PHANDLE;

#define NTAPI __stdcall
#define STATUS_SUCESS ((NTSTATUS) 0x00000000L)

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef NTSTATUS (NTAPI* NtOpenProcess)( 
    PHANDLE ProcessHandle, 
    ACCESS_MASK DesiredAccess, 
    POBJECT_ATTRIBUTES ObjectAttributes, 
    PCLIENT_ID ClientId 
);

typedef NTSTATUS(NTAPI* NtAllocateVirtualMemory)( 
    HANDLE ProcessHandle, 
    PVOID* BaseAddress, 
    ULONG_PTR ZeroBits, 
    PSIZE_T RegionSize, 
    ULONG AllocationType, 
    ULONG Protect 
);

typedef NTSTATUS (NTAPI* NtWriteVirtualMemory)( 
    HANDLE ProcessHandle, 
    PVOID BaseAddress, 
    PVOID Buffer, 
    ULONG BufferSize, 
    PULONG NumberOfBytesWritten 
);

typedef NTSTATUS(NTAPI* NtCreateThreadEx)( 
    PHANDLE ThreadHandle, 
    ACCESS_MASK DesiredAccess, 
    POBJECT_ATTRIBUTES ObjectAttributes, 
    HANDLE ProcessHandle, 
    LPTHREAD_START_ROUTINE StartRoutine, 
    PVOID Argument, 
    ULONG CreateFlags, 
    SIZE_T ZeroBits, 
    SIZE_T StackSize, 
    SIZE_T MaximumStackSize, 
    PVOID AttributeList 
);

/////////////////////////////////////////////////

int Load(const DWORD PID, const char* DLLpath) {

        NTSTATUS status     = STATUS_SUCESS;
        HANDLE   hproc      = NULL;
        HANDLE   hthread    = NULL;
        LPVOID   buffer     = NULL;
        SIZE_T   dllLen     = strlen(DLLpath) + 1;

    HMODULE nt = GetModuleHandleA("ntdll.dll");
    if (!nt) {
        printf("[!] Failed to Load Native API Dll\n");
        return 0;
    }

    /////////////////////////////////////////////////

    NtOpenProcess openproc = (NtOpenProcess)GetProcAddress(nt, "NtOpenProcess");
    NtAllocateVirtualMemory valloc = (NtAllocateVirtualMemory)GetProcAddress(nt, "NtAllocateVirtualMemory");
    NtWriteVirtualMemory writevm = (NtWriteVirtualMemory)GetProcAddress(nt, "NtWriteVirtualMemory");
    NtCreateThreadEx cthread = (NtCreateThreadEx)GetProcAddress(nt, "NtCreateThreadEx");

    if (!openproc || !valloc || !writevm || !cthread) {
        printf("[!] Failed To Load NT Functions.");
        return 0;
    }

    OBJECT_ATTRIBUTES oa = { sizeof(oa), NULL };
    CLIENT_ID cid = { (HANDLE)PID, NULL };

    /////////////////////////////////////////////////

    status = openproc(
        &hproc,
        PROCESS_ALL_ACCESS,
        &oa,
        &cid
    );

    if (status != STATUS_SUCESS || !hproc) {
        printf("[!] Failed To Get a Handle To The Target Process.");
        return 0;
    }

    //////////////////////////////////////

    status = valloc(
        hproc,
        &buffer,
        0,
        &dllLen,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (status != STATUS_SUCESS || !buffer) {
        printf("[!] Failed To Allocate Memory in The Target Process.");
        CloseHandle(hproc);
        return 0;
    }

    //////////////////////////////////////

    status = writevm(
        hproc,
        buffer,
        (PVOID)DLLpath,
        (ULONG)dllLen,
        NULL
    );

    if (status != STATUS_SUCESS) {
        printf("[!] Failed To Write The DLL Path To Remote Memory.");
        CloseHandle(hproc);
        return 0;
    }

    //////////////////////////////////////

    LPVOID loadlib = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    if (!loadlib) {
        printf("[!] Failed To Get Address Of LoadLibraryA.\n");
        CloseHandle(hproc);
        return 0;
    }

    //////////////////////////////////////

    status = cthread(
        &hthread,
        THREAD_ALL_ACCESS,
        NULL,
        hproc, 
        (LPTHREAD_START_ROUTINE)loadlib, 
        buffer,
        0,
        0, 
        0, 
        0, 
        NULL 
    );

    if (status != STATUS_SUCESS || !hthread) {
        printf("[!] Failed To Create Remote Thread.");
        CloseHandle(hproc);
        return 0;
    }

    //////////////////////////////////////

    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(hthread);
    CloseHandle(hproc);

    return 1;
}
