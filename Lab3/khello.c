#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
    syscall(331, "Hello from the kernel!");
}
