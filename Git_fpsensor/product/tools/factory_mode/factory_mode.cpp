
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>


#define FPTAG LOG_TAG


extern "C"  int testcase(void);

int main(void) {
    return testcase();
}
