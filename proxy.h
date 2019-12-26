#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include <string.h> 
#include <string>
#include <vector>
#include <map>

#include <pcap.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <thread>
#include <libnet.h>
#include <mutex>

using namespace std;

#pragma comment(lib,"lphlpapi.lib")

int lookup_host (const char *host);

class TCP_proxy{
private:
  map<int,int> proxy_fd;
  const static int BUFSIZE = 1024;
public:
  TCP_proxy(){};
  int init_proxy(int port, int ip_addr);
  bool find_url(char *data, char *url, int data_len);
  void client2server(int clientfd, int port, mutex &m);
  void server2client(int clientfd, int port, mutex &m);
  //void proxy_thread(int port);
};

void proxy_thread(TCP_proxy tp, int port);

