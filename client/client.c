#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "smooth_lib.h"

#define SMOOTH_MESSAGE_HEAD_LEN 12 

#define SMOOTH_LOGIN 0
#define SMOOTH_MESSAGE_CHART_TEXT 1

#define SMOOTH_CHART_MESSAGE_BUFFER_MAX_LEN 1024

#define SMOOTH_MSG_DEFAULT_HOST_IP "127.0.0.1"
#define SMOOTH_MSG_DEFAULT_PORT 45000

#define SMOOTH_CLIENT_PROMOT "smooth>> "

struct msg_head
{
	int version;
	int type;
	int length;
};

typedef struct msg_string
{
	char * data;
	int length;
} msg_string_t;

struct msg_chart_text
{
	struct msg_chart_text * next;

	msg_string_t from;
	msg_string_t to;
	msg_string_t msg;

	char buffer[0];
};

void smooth_msg_manager_push_msg(struct msg_chart_text * chart_text)
{
	char * p;
	char back;

	p = chart_text->from.data + chart_text->from.length;

	back = *p;
	*p = '\0';
	printf("Message\nFrom: %s\n", chart_text->from.data);
	*p = back;

	p = chart_text->msg.data + chart_text->msg.length;
	back = *p;
	*p = '\0';
	printf("Content: %s\n", chart_text->msg.data);
	*p = back;
}


int smooth_msg_pack_head(char * buffer, int version, int type, int length)
{
	char * p;
	
	p = buffer;

	*(int *)p = htonl(version);
	p += 4;

	*(int *)p = htonl(type);
	p += 4;

	*(int *)p = htonl(length);
	p += 4;

	return p - buffer;
}

int smooth_msg_pack_login(char * buffer, char * account, char * passwd)
{
	char * p;
	int len;

	p = buffer;

	len = strlen(account);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, account, len);
	p += len;

	len = strlen(passwd);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, passwd, len);
	p += len;

	return p - buffer;
}

int smooth_msg_pack_chart_text(char * buffer, char * from, char * to, char * msg)
{
	char * p;
	int len;

	p = buffer;

	len = strlen(from);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, from, len);
	p += len;

	len = strlen(to);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, to, len);
	p += len;

	len = strlen(msg);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, msg, len);
	p += len;

	return p - buffer;
}

/*
 * function only can be used in client side,
 * as the read routine maybe blocked
 */
void smooth_msg_read_msg_head(int sockfd, char * buffer, struct msg_head * head)
{
	char * p;
	int left = SMOOTH_MESSAGE_HEAD_LEN;
	int rbyte;

	p = buffer;

	while (left)
	{
		rbyte = read(sockfd, p, left);
		p += rbyte;
		left -= rbyte;
	}

	p = buffer;

	head->version = ntohl(*(int *)p);
	p += 4;

	head->type = ntohl(*(int *)p);
	p += 4;

	head->length = ntohl(*(int *)p);
	p += 4;
}

/*
 * function can only used in client code
 * as the read routine maybe blocked
 */
void smooth_msg_read_msg_content(int sockfd, char * buffer, int length)
{
	char * p;
	int left = length;
	int rbyte;

	p = buffer;

	while (left)
	{
		rbyte = read(sockfd, p, left);
		p += rbyte;
		left -= rbyte;
	}
}

struct msg_chart_text * gpstMsgChartTextPool;

int smooth_msg_initailize_chart_text_pool(int size)
{
	int i;
	struct msg_chart_text * chart_text;

	for (i = 0; i < size; ++i)
	{
		chart_text = malloc(sizeof(struct msg_chart_text) + SMOOTH_MESSAGE_HEAD_LEN + SMOOTH_CHART_MESSAGE_BUFFER_MAX_LEN);

		assert(chart_text && "chart text struct alloc failed");

		chart_text->next = gpstMsgChartTextPool;
		gpstMsgChartTextPool = chart_text;
	}
}

struct msg_chart_text * smooth_msg_get_chart_text_node()
{
	struct msg_chart_text * chart_text;

	if (!gpstMsgChartTextPool)
	{
		smooth_msg_initailize_chart_text_pool(20);
	}

	//TODO here need a lock
	chart_text = gpstMsgChartTextPool;
	gpstMsgChartTextPool = chart_text->next;

	return chart_text;
}

void smooth_msg_free_chart_msg_buffer(struct msg_chart_text * chart_text)
{
	//TODO here need a lock
	chart_text->next = gpstMsgChartTextPool;
	gpstMsgChartTextPool = chart_text;
}

void smooth_msg_read_msg(int sockfd)
{
	char * p;
	int left;
	int rbyte;
	struct msg_head head;
	struct msg_chart_text * chart_text;
	char msg_head_buf[SMOOTH_MESSAGE_HEAD_LEN];

	smooth_msg_read_msg_head(sockfd, msg_head_buf, &head);

	switch (head.type)
	{
		case SMOOTH_MESSAGE_CHART_TEXT:
			chart_text = smooth_msg_get_chart_text_node();

			p = chart_text->buffer;
			memcpy(p, msg_head_buf, SMOOTH_MESSAGE_HEAD_LEN);
			p += SMOOTH_MESSAGE_HEAD_LEN;

			smooth_msg_read_msg_content(sockfd, p, head.length);

			chart_text->from.length = ntohl(*(int *)p);
			p += 4;

			chart_text->from.data = p;
			p += chart_text->from.length;

			chart_text->to.length = ntohl(*(int *)p);
			p += 4;

			chart_text->to.data = p;
			p += chart_text->to.length;

			chart_text->msg.length = ntohl(*(int *)p);
			p += 4;

			chart_text->msg.data = p;
			p += chart_text->msg.length;

			smooth_msg_manager_push_msg(chart_text);
			break;
		default:
			assert(0 && "message head with an invalid type");
			return;
	}
}

int smooth_msg_initailize_connection(char * host_ip, short host_port)
{
	int sockfd;
	int ret;
	struct sockaddr_in addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		fprintf(stderr, "create socket failed\n");
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(host_port);
	ret = inet_pton(AF_INET, host_ip, &addr.sin_addr);
	if (ret != 1)
	{
		fprintf(stderr, "trans ip failed\n");
		close(sockfd);
		return -1;
	}

	ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (ret != 0)
	{
		fprintf(stderr, "connect failed\n");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int sockfd_conn;

int main()
{
	char * line = NULL;
	size_t line_len;
	int input_count;
	int len;

	char * msg_from = "Bob";
	char * msg_to = "Alice";

	char msg_buffer[SMOOTH_CHART_MESSAGE_BUFFER_MAX_LEN];
	char * p;

	smooth_msg_initailize_chart_text_pool(20);

	sockfd_conn = smooth_msg_initailize_connection(SMOOTH_MSG_DEFAULT_HOST_IP, SMOOTH_MSG_DEFAULT_PORT);
	if (sockfd_conn == -1)
	{
		fprintf(stderr, "client connect to %s:%d failed\n", SMOOTH_MSG_DEFAULT_HOST_IP, SMOOTH_MSG_DEFAULT_PORT);
		return 0;
	}

	while (1)
	{
		printf(SMOOTH_CLIENT_PROMOT);
		input_count = getline(&line, &line_len, stdin);
		if (input_count == -1)
			return -1;

		line[input_count - 1] = '\0';

		len = smooth_msg_pack_chart_text(msg_buffer + SMOOTH_MESSAGE_HEAD_LEN, msg_from, msg_to, line);

		smooth_msg_pack_head(msg_buffer, 1, SMOOTH_MESSAGE_CHART_TEXT, len);

		write(sockfd_conn, msg_buffer, len + SMOOTH_MESSAGE_HEAD_LEN);
	}
}
