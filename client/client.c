#include <arpa/inet.h>
#include "include/message_def.h"

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

int smooth_msg_pack_chart_text(char * buffer, char * who, char * msg)
{
	char * p;
	int len;

	p = buffer;

	len = strlen(who);
	*(int *)p = htonl(len);
	p += 4;

	memcpy(p, who, len);
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
	int left = MESSAGE_HEAD_LEN;
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

struct msg_str
{
	char * data;
	int length;
};

struct msg_chart_text
{
	struct msg_chart_text * next;
	char * buffer;

	struct msg_str who;
	struct msg_str msg;
};

struct msg_chart_text * gpstMsgChartTextPool;


struct msg_chart_text * smooth_msg_get_chart_text_node()
{
	struct msg_chart_text * chart_text;

	if (!gpstMsgChartTextPool)
	{
		gpstMsgChartTextPool = malloc(sizeof(struct msg_chart_text) + MESSAGE_CHART_TEXT_CONTENT_MAX_LENGTH);
		if (!gpstMsgChartTextPool)
			return NULL;

		gpstMsgChartTextPool->next = NULL;
		gpstMsgChartTextPool->buffer = gpstMsgChartTextPool + sizeof(struct msg_chart_text);
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
	char * buffer;
	int left;
	int rbyte;
	struct msg_head head;
	struct msg_chart_text * chart_text;
	char msg_head_buf[MESSAGE_HEAD_LEN];

	smooth_msg_read_msg_head(sockfd, msg_head_buf, &head);

	switch (head.type)
	{
		case SMOOTH_MESSAGE_CHART_TEXT:
			chart_text = smooth_msg_get_chart_text_node();

			smooth_msg_read_msg_content(sockfd, chart_text->buffer, head.length);

			p = chart_text->buffer;

			chart_text->who.length = ntohl(*(int *)p);
			p += 4;

			chart_text->who.data = p;
			p += chart_text->who.length;

			chart_text->msg.length = ntohl(*(int *)p);
			p += 4;
			chart_text->who.data = p;

			smooth_msg_manager_push_msg(chart_text);
			break;
		default:
			smooth_msg_free_chart_msg_buffer(buffer);
			assert(0 && "message head with an invalid type");
			return;
	}
}

int smooth_msg_initailize_connection(char * host_ip, int host_port)
{
	int sockfd;
	struct sockaddr addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd != 0)
		return -1;

	addr.sa_family = AF_INET;
	addr.sa_port = htons(host_port);
	ret = inet_pton(AF_INET, host_ip, &addr.sa_addr);
	if (ret != 0)
	{
		close(sockfd);
		return -1;
	}

	ret = connect(sockfd, &addr, sizeof(struct sockaddr));
	if (ret != 0)
	{
		close(sockfd);
		return -1;
	}

	return sockfd;
}

struct msg_manager
{
	int sockfd;
};
