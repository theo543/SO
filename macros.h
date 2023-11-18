#include <stdlib.h>

#define PERROR_WITH_DETAILS(CALL)\
do {\
    /* SOURCE_PATH_SIZE defined in CMake */\
    fprintf(stderr, "Error at file %s:%d => ", __FILE__ + SOURCE_PATH_SIZE, __LINE__);\
    perror(#CALL);\
} while (0)

// Call pthread function, report any errors.
#define PT_CALL(CALL)\
do {\
    int result = CALL;\
    if(result != 0) {\
        errno = result;\
        PERROR_WITH_DETAILS(CALL);\
        exit(errno);\
    }\
} while (0)

// Call semaphore function, report any errors.
#define SEM_CALL(CALL)\
do {\
    int result = CALL;\
    if(result != 0) {\
        PERROR_WITH_DETAILS(CALL);\
        exit(errno);\
    }\
} while (0)
