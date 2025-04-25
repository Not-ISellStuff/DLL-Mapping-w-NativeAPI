# DLL-Mapping-w-NativeAPI
Basic program that injects a dll into a process using the native api

# Compile
```
gcc -c loader.c -o loader.o
gcc main.c loader.o -o main.exe
```

# Usage

```
./main -D hacks.dll -P Notepad.exe

or

main.exe -D hacks.dll -P Notepad.exe
```

# Resources
native api functions documentation
```
https://ntdoc.m417z.com/
https://www.vergiliusproject.com/kernels/x64/windows-11/24h2
```
