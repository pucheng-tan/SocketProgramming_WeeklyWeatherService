/*
** proxy2.c - a proxy server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "38500" // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 200

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
    char buf[MAXDATASIZE];
    char message_to_client[MAXDATASIZE] = "";
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int i;

    const char *days[7] = {"Monday", "Tuesday", "Wednesday",
                           "Thursday", "Friday", "Saturday", "Sunday"};

    // from client.c
    int proxy_client_sd, proxy_client_numbytes;
    char proxy_client_buf[MAXDATASIZE];
    struct addrinfo proxy_client_hints, *proxy_client_servinfo, *proxy_client_p;
    int proxy_client_rv;
    char proxy_client_s[INET6_ADDRSTRLEN];

    if (argc != 3)
    {
        fprintf(stderr, "usage: proxy hostname port_number_of_the_server\n");
        exit(1);
    }

    // from client.c

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1)
    { // main accept() loop

        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork())
        {                  // this is the child process
            close(sockfd); // child doesn't need the listener

            if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';
            printf("server: received '%s'\n", buf);

            if (strcmp(buf, "all") == 0)
            {
                for (i = 0; i < 7; i++)
                {
                    // from client.c

                    memset(&proxy_client_hints, 0, sizeof proxy_client_hints);
                    proxy_client_hints.ai_family = AF_UNSPEC;
                    proxy_client_hints.ai_socktype = SOCK_STREAM;

                    if ((proxy_client_rv = getaddrinfo(argv[1], argv[2], &proxy_client_hints, &proxy_client_servinfo)) != 0)
                    {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(proxy_client_rv));
                        return 1;
                    }

                    // loop through all the results and bind to the first we can
                    for (proxy_client_p = proxy_client_servinfo; proxy_client_p != NULL; proxy_client_p = proxy_client_p->ai_next)
                    {
                        if ((proxy_client_sd = socket(proxy_client_p->ai_family, proxy_client_p->ai_socktype, proxy_client_p->ai_protocol)) == -1)
                        {
                            perror("client: socket");
                            continue;
                        }

                        if (connect(proxy_client_sd, proxy_client_p->ai_addr, proxy_client_p->ai_addrlen) == -1)
                        {
                            close(proxy_client_sd);
                            perror("client: connect");
                            continue;
                        }

                        break;
                    }

                    if (p == NULL)
                    {
                        fprintf(stderr, "client: failed to connect\n");
                        return 2;
                    }

                    inet_ntop(proxy_client_p->ai_family, get_in_addr((struct sockaddr *)proxy_client_p->ai_addr), proxy_client_s, sizeof proxy_client_s);

                    printf("client: connecting to %s\n", proxy_client_s);

                    freeaddrinfo(proxy_client_servinfo); // all done with this structure

                    // from client.c

                    if (send(proxy_client_sd, days[i], 13, 0) == -1)
                        perror("send");

                    if ((proxy_client_numbytes = recv(proxy_client_sd, proxy_client_buf, MAXDATASIZE - 1, 0)) == -1)
                    {
                        perror("recv");
                        exit(1);
                    }

                    proxy_client_buf[proxy_client_numbytes] = '\0';

                    close(proxy_client_sd);

                    strcat(message_to_client, "\n");
                    strcat(message_to_client, days[i]);
                    strcat(message_to_client, ": ");
                    strcat(message_to_client, proxy_client_buf);
                }
            }
            else
            {
                // from client.c
                memset(&proxy_client_hints, 0, sizeof proxy_client_hints);
                proxy_client_hints.ai_family = AF_UNSPEC;
                proxy_client_hints.ai_socktype = SOCK_STREAM;

                if ((proxy_client_rv = getaddrinfo(argv[1], argv[2], &proxy_client_hints, &proxy_client_servinfo)) != 0)
                {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(proxy_client_rv));
                    return 1;
                }

                // loop through all the results and bind to the first we can
                for (proxy_client_p = proxy_client_servinfo; proxy_client_p != NULL; proxy_client_p = proxy_client_p->ai_next)
                {
                    if ((proxy_client_sd = socket(proxy_client_p->ai_family, proxy_client_p->ai_socktype, proxy_client_p->ai_protocol)) == -1)
                    {
                        perror("client: socket");
                        continue;
                    }

                    if (connect(proxy_client_sd, proxy_client_p->ai_addr, proxy_client_p->ai_addrlen) == -1)
                    {
                        close(proxy_client_sd);
                        perror("client: connect");
                        continue;
                    }

                    break;
                }

                if (p == NULL)
                {
                    fprintf(stderr, "client: failed to connect\n");
                    return 2;
                }

                inet_ntop(proxy_client_p->ai_family, get_in_addr((struct sockaddr *)proxy_client_p->ai_addr), proxy_client_s, sizeof proxy_client_s);

                printf("client: connecting to %s\n", proxy_client_s);

                freeaddrinfo(proxy_client_servinfo); // all done with this structure

                if (send(proxy_client_sd, buf, 13, 0) == -1)
                    perror("send");

                if ((proxy_client_numbytes = recv(proxy_client_sd, proxy_client_buf, MAXDATASIZE - 1, 0)) == -1)
                {
                    perror("recv");
                    exit(1);
                }

                proxy_client_buf[proxy_client_numbytes] = '\0';

                close(proxy_client_sd);
                // from client.c

                strcat(message_to_client, proxy_client_buf);
            }

            if (send(new_fd, message_to_client, MAXDATASIZE, 0) == -1)
                perror("send");
            close(new_fd);

            strcpy(message_to_client, "");

            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}