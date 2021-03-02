/*
** client.c -- a stream socket client
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 200 // max number of bytes we can get at once 

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
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    char day[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    int valid_input = 0;
    int i;

    const char *days[7] = {"Monday", "Tuesday", "Wednesday",
                           "Thursday", "Friday", "Saturday", "Sunday"};

    if (argc != 3)
    {
        fprintf(stderr, "usage: client hostname port_number_of_the_server\n");
        exit(1);
    }

    while (1)
    {
        while(valid_input == 0)
        {
            printf("Enter the day of a week\n");
            scanf("%s", day);
            if (strcmp(day, "quit") == 0)
                return 0;

            if (strcmp(day, "all") == 0)
                valid_input = 1;

            for (i = 0; i < 7; i++)
            {
                if (strcmp(day, days[i]) == 0)
                    valid_input = 1;
            }
        }
        valid_input = 0;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and connect to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("client: socket");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(sockfd);
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

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

        printf("client: connecting to %s\n", s);

        freeaddrinfo(servinfo); // all done with this structure

        // send a request

        if (send(sockfd, day, 13, 0) == -1)
            perror("send");

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        
        printf("\nWeather Summary: %s\n\n", buf);

        close(sockfd);
    }

    return 0;
}
