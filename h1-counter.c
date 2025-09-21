/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
 * man pages, mostly getaddrinfo(3).
 * Program 1
 * Lauren Runion and Aaron Partridge
 * We used the starter code provided*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect(const char *host, const char *service);

/*******************************************************************************
 * Function Name: sendall()
 * Parameters: int s, char *buf, int *len
 * Return Value: int
 * Purpose: taken from Beej's Guide chapter 7.4 - used to deal with partial sends until the full packet has been sent.
 *******************************************************************************/
int sendall(int s, char *buf, int *len)
{
	int total = 0;		  // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;

	while (total < *len)
	{
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

/*******************************************************************************
 * Function Name: recvall()
 * Parameters: int s, char *buf, int len
 * Return Value: int
 * Purpose: modified from sendall() in Beej's Guide chapter 7.4 - used to make sure the full packet is received when dealing with partial sends
 *******************************************************************************/
int recvall(int s, char *buf, int len)
{
	int totalReceived = 0;
	while (totalReceived < len)
	{
		int bytesReceived = recv(s, buf + totalReceived, len - totalReceived, 0);
		if (bytesReceived < 0)
		{
			return -1;
		}
		else if (bytesReceived == 0)
		{
			break;
		}
		else
		{
			totalReceived += bytesReceived;
		}
	}
	return totalReceived;
}

/*******************************************************************************
 * Function Name: countTags()
 * Parameters: char *buf, int len
 * Return Value: int
 * Purpose: to count only exact matches for "<h1>" and not partial matches
 *******************************************************************************/
int countTags(char *buf, int len)
{
	int count = 0;
	const char *h1tag = "<h1>";
	const int tagLength = strlen(h1tag);

	// Searches current buffer for <h1>tags
	for (int i = 0; i <= len - tagLength; i++)
	{
		// Looks in current chunks for full <h1> tags
		if (strncasecmp(&buf[i], h1tag, tagLength) == 0)
		{
			count++;
			i += tagLength - 1; // moves onto next tag using length
		}
	}
	return count;
}

int main(int argc, char *argv[])
{
	int s;
	const char *host = "www.ecst.csuchico.edu";
	const char *port = "80";
	char request[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n"; // http request
	int len = strlen(request);									// gets string length of char request
	char buf[1000];
	int chunkSize = 0;
	int receivedBytes = 0;
	int totalBytes = 0;
	int totalTags = 0;

	if (argc == 2)
	{
		chunkSize = atoi(argv[1]);
	}
	else
	{
		fprintf(stderr, "usage: %s host\n", argv[0]);
		exit(1);
	}

	/* Lookup IP and connect to server */
	if ((s = lookup_and_connect(host, port)) < 0)
	{
		exit(1);
	}

	if (sendall(s, request, &len) == -1)
	{
		perror("Request failed");
		close(s);
		exit(1);
	}

	// Loop to receive, count and total, bytes and tags
	while ((receivedBytes = recvall(s, buf, chunkSize)) > 0)
	{
		totalBytes += receivedBytes;
		if (receivedBytes == -1)
		{
			perror("Error: recvall");
			close(s);
			exit(1);
		}
		totalTags += countTags(buf, receivedBytes);
	}

	printf("Number of bytes: %d\n", totalBytes);
	printf("Number of <h1> tags: %d\n", totalTags);
	close(s);

	return 0;
}

int lookup_and_connect(const char *host, const char *service)
{
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Translate host name into peer's IP address */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ((s = getaddrinfo(host, service, &hints, &result)) != 0)
	{
		fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	/* Iterate through the address list and try to connect */
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
		{
			continue;
		}

		if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			break;
		}

		close(s);
	}
	if (rp == NULL)
	{
		perror("stream-talk-client: connect");
		return -1;
	}
	freeaddrinfo(result);

	return s;
}
