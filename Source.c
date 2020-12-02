#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>

int main(int argc, char *argv[]){
    /* argv[0] = "Source", [1] = x, [2] = y, [3] = NULL */
    if(argc != 3){
        fprintf(stderr, "\n%s: %d. WRONG NUMBER OF ARGUMENT RECEIVED :%d INSTEAD of 4!\n", __FILE__, __LINE__, argc);
	    exit(EXIT_FAILURE);
    }

    dprintf(1, "\nFIGLIO: %d",getpid());
    return(0);
}