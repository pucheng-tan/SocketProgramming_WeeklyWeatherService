/*
** proxy3.c - a proxy server
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

#define PORT "39600" // the port users will be connecting to
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
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    const char *days[7] = {"Monday", "Tuesday", "Wednesday",
                           "Thursday", "Friday", "Saturday", "Sunday"};

    // from talker.c

    int udp_sockfd;
    struct addrinfo udp_hints, *udp_servinfo, *udp_p;
    int udp_rv;
    int udp_numbytes;
    struct sockaddr_storage udp_their_addr;
    socklen_t udp_addr_len;
    char udp_buf[MAXDATASIZE];
    char udp_s[INET6_ADDRSTRLEN];
    char message_to_client[MAXDATASIZE] = "";
    int i;

    if (argc != 3)
    {
        fprintf(stderr, "usage: proxy hostname port_number_of_the_server\n");
        exit(1);
    }

    // from talker.c

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
                    memset(&udp_hints, 0, sizeof udp_hints);
                    udp_hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
                    udp_hints.ai_socktype = SOCK_DGRAM;

                    if ((udp_rv = getaddrinfo(argv[1], argv[2], &udp_hints, &udp_servinfo)) != 0)
                    {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_rv));
                        return 1;
                    }

                    // loop through all the results and make a socket
                    for (udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next)
                    {
                        if ((udp_sockfd = socket(udp_p->ai_family, udp_p->ai_socktype,
                                                 udp_p->ai_protocol)) == -1)
                        {
                            perror("talker: socket");
                            continue;
                        }

                        break;
                    }

                    if (udp_p == NULL)
                    {
                        fprintf(stderr, "talker: failed to create socket\n");
                        return 2;
                    }

                    if ((udp_numbytes = sendto(udp_sockfd, days[i], strlen(days[i]), 0,
                                               udp_p->ai_addr, udp_p->ai_addrlen)) == -1)
                    {
                        perror("talker: sendto");
                        exit(1);
                    }

                    freeaddrinfo(udp_servinfo);

                    udp_addr_len = sizeof udp_their_addr;
                    if ((udp_numbytes = recvfrom(udp_sockfd, udp_buf, MAXDATASIZE - 1, 0,
                                                 (struct sockaddr *)&udp_their_addr, &udp_addr_len)) == -1)
                    {
                        perror("recvfrom");
                        exit(1);
                    }

                    printf("talker: got packet from %s\n",
                           inet_ntop(udp_their_addr.ss_family,
                                     get_in_addr((struct sockaddr *)&udp_their_addr),
                                     udp_s, sizeof udp_s));
                    printf("talker: packet is %d bytes long\n", udp_numbytes);
                    udp_buf[udp_numbytes] = '\0';
                    printf("talker: packet contains \"%s\"\n", udp_buf);

                    printf("talker: sent %d bytes to %s\n", udp_numbytes, argv[1]);
                    close(udp_sockfd);

                    strcat(message_to_client, "\n");
                    strcat(message_to_client, days[i]);
                    strcat(message_to_client, ": ");
                    strcat(message_to_client, udp_buf);
                }
            }
            else
            {
                memset(&udp_hints, 0, sizeof udp_hints);
                udp_hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
                udp_hints.ai_socktype = SOCK_DGRAM;

                if ((udp_rv = getaddrinfo(argv[1], argv[2], &udp_hints, &udp_servinfo)) != 0)
                {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_rv));
                    return 1;
                }

                // loop through all the results and make a socket
                for (udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next)
                {
                    if ((udp_sockfd = socket(udp_p->ai_family, udp_p->ai_socktype,
                                             udp_p->ai_protocol)) == -1)
                    {
                        perror("talker: socket");
                        continue;
                    }

                    break;
                }

                if (udp_p == NULL)
                {
                    fprintf(stderr, "talker: failed to create socket\n");
                    return 2;
                }

                if ((udp_numbytes = sendto(udp_sockfd, buf, strlen(buf), 0,
                                           udp_p->ai_addr, udp_p->ai_addrlen)) == -1)
                {
                    perror("talker: sendto");
                    exit(1);
                }

                freeaddrinfo(udp_servinfo);

                udp_addr_len = sizeof udp_their_addr;
                if ((udp_numbytes = recvfrom(udp_sockfd, udp_buf, MAXDATASIZE - 1, 0,
                                             (struct sockaddr *)&udp_their_addr, &udp_addr_len)) == -1)
                {
                    perror("recvfrom");
                    exit(1);
                }

                printf("talker: got packet from %s\n",
                       inet_ntop(udp_their_addr.ss_family,
                                 get_in_addr((struct sockaddr *)&udp_their_addr),
                                 udp_s, sizeof udp_s));
                printf("talker: packet is %d bytes long\n", udp_numbytes);
                udp_buf[udp_numbytes] = '\0';
                printf("talker: packet contains \"%s\"\n", udp_buf);

                printf("talker: sent %d bytes to %s\n", udp_numbytes, argv[1]);
                close(udp_sockfd);

                strcat(message_to_client, udp_buf);
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