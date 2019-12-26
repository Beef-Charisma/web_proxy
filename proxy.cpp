#include "proxy.h"

// https://gist.github.com/jirihnidek/bf7a2363e480491da72301b228b35d5d
int lookup_host (const char *host){
    struct addrinfo hints, *res;
    int errcode;
    char addrstr[100];
    void *ptr;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo (host, NULL, &hints, &res);
    if (errcode != 0)
    {
        perror ("getaddrinfo");
        return -1;
    }

    printf ("Host: %s\n", host);
while (res){
        inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);
        switch (res->ai_family){
        case AF_INET:
            ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
            break;
        case AF_INET6:
            ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
            break;
        }
        inet_ntop (res->ai_family, ptr, addrstr, 100);
        printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
                addrstr, res->ai_canonname);
        res = res->ai_next;
    }
    return 0;
}

/* init server connect
   int: return file desciptor 
   basic setting for ip_addr: proxy server 
*/
int TCP_proxy::init_proxy(int port, int ip_addr){
    int proxy_fd=socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_fd==-1){
        perror("socket failed");
        return -1;
    }
    int optval = 1;
    setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip_addr;
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    int res = bind(proxy_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
    if (res == -1) {
        perror("bind failed");
        return -1;
    }

    res = listen(proxy_fd, 2);
    if (res == -1) {
        perror("listen failed");
        return -1;
    }
    puts("TCP server established");
    return proxy_fd;    
}

/* data_len: total length for data
   url: buffer for save finded url
*/
bool TCP_proxy::find_url(char *data, char *url, int data_len){
    //printf("%d\n",data_len);
    int start=-1, len=0;
	char host[10]="Host: ";
	for(int i=0;i<data_len-6;i++){
		if(memcmp(data+i,host,strlen(host))==0){
			start=i+6;
			break;
		}
	}
    if(start==-1) return false;
	for(int i=start;i<data_len-2;i++){
		if(memcmp(data+i,"\x0d\x0a",2)==0){
			len=(i-start);
			break;
		}
	}
    memcmp(url,data,len);
    url[len]=0;
    return true;
}

void TCP_proxy::client2server(int proxyfd, int port, mutex &m){
    while(true){
        char buf[BUFSIZE]={0,};
        m.lock();
		ssize_t received = recv(proxyfd, buf, BUFSIZE - 1, 0);  
		if(received==0 || received==-1){
            cout<<"[-] recv failed..."<<'\n';
            continue;
        }
        buf[received] = '\0';  
        for(int i=0;i<received;i++){
            printf("%02x ",buf[i]);
            if(i%8==7) cout<<'\n';
        }         
        char server_url[BUFSIZE]={0,};
        if(find_url(buf, server_url, received)==false){
            cout<<"[-] Cannot find URL..."<<'\n';
            continue;
        }
        int server_ip=lookup_host(server_url);
        m.unlock();
        int server_fd;
        if(proxy_fd.find(server_ip)==proxy_fd.end()){
            server_fd=socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd==-1){
                perror("server socket failed");
                continue;
            }
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = server_ip;
            
            memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

            int res = connect(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
            if (res == -1) {
                perror("connect failed");
                continue;
            }            
        }
        else{
            server_fd=proxy_fd[server_ip];
        }
        printf("socket %d connected\n",server_fd);
        ssize_t sent=send(server_fd, buf, strlen(buf), 0);
        if(sent==0){
            perror("sent failed");
            continue;
        }
    }
}

void TCP_proxy::server2client(int proxyfd, int port, mutex &m){
    while(true){
        char buf[BUFSIZE]={0,};
        m.lock();
		ssize_t received = recv(proxyfd, buf, BUFSIZE - 1, 0);  
		if(received==0 || received==-1){
            cout<<"[-] recv failed..."<<'\n';
            continue;
        }
        buf[received] = '\0'; 
        for(int i=0;i<received;i++){
            printf("%02x ",buf[i]);
            if(i%8==7) cout<<'\n';
        } 
        char client_url[BUFSIZE]={0,};
        if(find_url(buf, client_url, received)==false){
            cout<<"[-] Cannot find URL..."<<'\n';
            continue;
        }
        int server_ip=lookup_host(client_url);
        m.unlock();
        int server_fd;
        if(proxy_fd.find(server_ip)==proxy_fd.end()){
            server_fd=socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd==-1){
                perror("client socket failed");
                continue;
            }
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = server_ip;
            
            memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

            int res = connect(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
            if (res == -1) {
                perror("connect failed");
                continue;
            }            
        }
        else{
            server_fd=proxy_fd[server_ip];
        }
        printf("socket %d connected\n",server_fd);
        ssize_t sent=send(server_fd, buf, strlen(buf), 0);
        if(sent==0){
            perror("sent failed");
            continue;
        }
    }
}

void proxy_thread(TCP_proxy tp, int port){
    int proxy_fd=tp.init_proxy(port, 0);
    mutex m;
    thread t1=thread(&TCP_proxy::client2server, &tp, proxy_fd, port, ref(m));
    thread t2=thread(&TCP_proxy::server2client, &tp, proxy_fd, port, ref(m));
    t1.join();
    t2.join();
}

