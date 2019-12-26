#include "proxy.h"

void usage(){
    printf("syntax : web_proxy <tcp port>\n");
    printf("sample : web_proxy 8080\n");
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        usage();
        return -1;
    }

    TCP_proxy tp=TCP_proxy();
    int port=atoi(argv[1]);
    proxy_thread(tp, port);
    return 0;
}

