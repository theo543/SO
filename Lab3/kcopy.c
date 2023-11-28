#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
    {
        const char hello[] = "Hello World!\n";
        char dst[] = "Placeholder.\n";
        int retval = syscall(332, hello, dst, sizeof(hello));
        printf("Len:%lu\nSource: %sCopied: %sretval: %d\n\n", sizeof(hello), hello, dst, retval);
    }
    {
        char dst[] = "Placeholder.\n";
        int retval = syscall(332, NULL, dst, sizeof(dst));
        printf("Len:%lu\nSource: NULL\nCopied: %sretval: %d\n\n", sizeof(dst), dst, retval);
    }
    {
        const char hello[] = "Hello World!\n";
        int retval = syscall(332, hello, NULL, sizeof(hello));
        printf("Len:%lu\nSource: %sCopied: NULL\nretval: %d\n\n", sizeof(hello), hello, retval);
    }
}
