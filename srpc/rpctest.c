#include "srpc.h"
#include <time.h>

int main(int argc, char *argv[]) {
    unsigned short port = 0;
    struct timespec ts = {1, 0};

    rpc_init(port);
    nanosleep(&ts, NULL);
    rpc_shutdown();
    nanosleep(&ts, NULL);
    return 0;
}
