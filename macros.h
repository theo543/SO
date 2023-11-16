// Call pthread function, report any errors.
#define PT_CALL(CALL)\
do {\
    int result = CALL;\
    if(result != 0) {\
        errno = result;\
        perror(#CALL);\
        exit(errno);\
    }\
} while (0)

// Call semaphore function, report any errors.
#define SEM_CALL(CALL)\
do {\
    int result = CALL;\
    if(result != 0) {\
        perror(#CALL);\
        exit(errno);\
    }\
} while (0)
