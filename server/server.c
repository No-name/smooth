
#define MSG_SUCCESS 			0
#define MSG_ERR_FAILED			1
#define MSG_ERR_AGAIN			2

typedef struct head_buffer 
{
	char * p;
	int left;
	char buffer[SMOOTH_MSG_HEAD_MAX_LEN];
} head_buffer_t;

struct connection 
{
	int sockfd;

	struct head_buffer head;
	void * content;
	int (*handler)(struct connection * conn);
};

typedef struct msg_head
{
	int version;
	int length;
	int type;
} msg_head_t;

typedef struct msg_string
{
	int length;
	char * data;
} msg_string_t;

#define SMOOTH_SET_FD_NONBLOCK(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)

struct msg_login
{
	struct msg_login * next;

	msg_string_t account;
	msg_string_t passwd;

	char * content;
	char * p;
	int left;
	char msg_buffer[0];
};

typedef struct msg_chart_text
{
	struct msg_chart_text * next;

	msg_string_t msg_from;
	msg_string_t msg_to;
	msg_string_t msg_content;

	char * content;
	char * p;
	int left;
	char msg_buffer[0];
} msg_chart_text_t;

void smooth_msg_parse_chart_text(msg_chart_text_t * chart_text)
{
	char * p = chart_text->content;

	chart_text->msg_from.length = ntohl(*(int *)p);
	p += 4;

	chart_text->msg_from.data = p;
	p += chart_text->msg_from.length;

	chart_text->msg_to.length = ntohl(*(int *)p);
	p += 4;

	chart_text->msg_to.data = p;
	p += chart_text->msg_to.length;

	chart_text->msg_content.length = ntohl(*(int *)p);
	p += 4;

	chart_text->msg_content.data = p;
	p += chart_text->msg_content.length;
}

void smooth_msg_process_chart_text(struct connection * conn)
{
	msg_chart_text_t * chart_text = conn->content;

	smooth_msg_parse_chart_text(chart_text);

	smooth_msg_manager_transfor_chart_text(chart_text);
}

int smooth_msg_chart_text_handler(struct connection * conn)
{
	msg_chart_text_t * chart_text = conn->content;

	int left = chart_text->left;
	char * p = chart_text->p;

	int rbyte;

	while (1)
	{
		rbyte = read(conn->sockfd, p, left);
		if (rbyte == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				chart_text->left = left;
				chart_text->p = p;

				return MSG_ERR_AGAIN;
			}

			return MSG_ERR_FAILED;
		}

		p += rbyte;
		left -= rbyte;

		if (left == 0)
		{
			smooth_msg_process_chart_text(conn);

			conn->handler = smooth_msg_head_handler;
			return smooth_msg_head_handler(conn);
		}
	}
}

void smooth_msg_parse_login(struct msg_login * login)
{
	char * p = login->content;

	login->account.length = htonl(*(int *)p);
	p += 4;

	login->account.data = p;
	p += login->account.length;

	login->passwd.length = htonl(*(int *)p);
	p += 4;

	login->passwd.data = p;
	p += login->passwd.length;
}

void smooth_msg_process_login(struct connection * conn)
{
	struct msg_login * login = conn->content;

	smooth_msg_parse_login(login);

	smooth_msg_manager_process_login(conn);
}

int smooth_msg_login_handler(struct connection * conn)
{
	struct msg_login * login = conn->content;

	int left = login->left;
	char * p = login->p;

	int rbyte;

	while (1)
	{
		rbyte = read(conn->sockfd, p, left);
		if (rbyte == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				login->left = left;
				login->p = p;

				return MSG_ERR_AGAIN;
			}

			return MSG_ERR_FAILED;
		}

		p += rbyte;
		left -= rbyte;

		if (left == 0)
		{
			smooth_msg_process_login(conn);

			conn->handler = smooth_msg_head_handler;
			return smooth_msg_head_handler(conn);
		}
	}
}

void smooth_msg_process_head(struct connection * conn)
{
	struct msg_head head;
	struct msg_chart_text * chart_text;
	struct msg_login * login;

	smooth_msg_parse_head(&conn->head, &head);

	switch (head->type)
	{
		case SMOOTH_MSG_CHART_TEXT:
			chart_text = smooth_msg_get_msg_chart_text();

			conn->content = chart_text;
			memcpy(chart_text->p, conn->head.buffer, SMOOTH_MSG_HEAD_MAX_LEN);
			chart_text->p += SMOOTH_MSG_HEAD_MAX_LEN;

			chart_text->left = head.length;
			chart_text->content = p;

			conn->handler = smooth_msg_chart_text_handler;

			return smooth_msg_chart_text_handler(conn);
		case SMOOTH_MSG_LOGIN:
			login = smooth_msg_get_login();

			conn->content = login;
			memcpy(chart_text->p, conn->head.buffer, SMOOTH_MSG_HEAD_MAX_LEN);
			chart_text->p += SMOOTH_MSG_HEAD_MAX_LEN;

			chart_text->left = head.length;
			chart_text->content = p;

			conn->handler = smooth_msg_login_handler;

			return smooth_msg_login_handler(conn);
			break;
		default:
			assert(0 && "message type invalid");
			return;
	}
}

void smooth_msg_parse_head(head_buffer_t * head_buf, msg_head_t * head)
{
	char * p;

	head->version = ntohl(*(int *)p);
	p += 4;

	head->type = ntohl(*(int *)p);
	p += 4;

	head->length = ntohl(*(int *)p);
}

int smooth_msg_head_handler(struct connection * conn)
{
	int left = conn->head.left;
	char * p = conn->head.p;

	int rbyte;

	while (1)
	{
		rbyte = read(conn->sockfd, p, left);
		if (rbyte == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				conn->head.left = left;
				conn->head.p = p;

				return MSG_ERR_AGAIN;
			}

			return MSG_ERR_FAILED;
		}

		p += rbyte;
		left -= rbyte;

		if (left == 0)
		{
			smooth_msg_process_head(conn);
			return MSG_SUCCESS;
		}
	}
}

#define SMOOTH_CONNECTION_MAX 1024
struct connection * gpstConnectionArray[SMOOTH_CONNECTION_MAX];
int gConnectionCount;

int epollfd_waiter;

int sockfd_listener;

int smooth_manager_conn_handler(int conn_fd)
{
	struct connection * conn;

	conn = gpstConnectionArray[conn_fd];

	return conn->handler(conn);
}

void smooth_manager_listen_handler()
{
	struct connection * conn;
	struct epoll_event event;
	int conn_fd;

	while (1)
	{
		conn_fd = accept(sockfd, &sockaddr, sockaddr_len);
		if (conn_fd == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return MSG_ERR_AGAIN;

			return MSG_ERR_FAILED;
		}

		conn = smooth_manager_get_connection();

		SMOOTH_SET_FD_NONBLOCK(conn_fd);
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = conn_fd;
		ret = epoll_ctl(epollfd_waiter, EPOLL_CTL_ADD, conn_fd, &event);
		if (ret == -1)
		{
			close(conn_fd);
			smooth_manager_free_connection(conn);

			return MSG_ERR_FAILED;
		}

		smooth_msg_init_head_buffer(&conn->head);

		conn->sockfd = conn_fd;
		conn->content = NULL;
		conn->handler = smooth_msg_head_handler;

		gpstConnectionArray[conn_fd] = conn;
	}
}

int smooth_manager_create_listener_sockfd(char * host, short port)
{
	int ret;
	int sockfd;
	struct sockaddr saddr;
	struct event;

	saddr.sa_family = AF_INET;
	saddr.sa_port = htons(port);

	ret = inet_pton(AF_INET, host, &saddr.sa_addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		return MSG_ERR_FAILED;

	ret = bind(sockfd, &saddr, sizeof(struct sockaddr));
	if (ret == -1)
		return MSG_ERR_FAILED;

	ret = listen(sockfd, SMOOTH_LISTEN_POOL);
	if (ret == -1)
		return MSG_ERR_FAILED;

	SMOOTH_SET_FD_NONBLOCK(sockfd);

	sockfd_listener = sockfd;

	event.events = EPOLLIN | EPOLLET;
	event.data.fd = sockfd;
	epoll_ctl(epollfd_waiter, EPOLL_CTL_ADD, sockfd, &event);

	return sockfd_listener;
}

int main()
{
	int nfds, fd;

	struct epoll_event event_responed[SMOOTH_CONNECTION_MAX];

	epollfd_waiter = epoll_create(SMOOTH_EPOLL_SIZE_DEFAULT);
	if (epollfd_waiter == -1)
		return;

	smooth_manager_create_listener_sockfd(SMOOTH_HOST_DEFAULT, SMOOTH_PORT_DEFAULT);

	while (1)
	{
		fds = epoll_wait(epollfd_waiter, event_responed, SMOOTH_CONNECTION_MAX, -1);
		if (fds == -1)
			return;

		for (i = 0; i < fds; ++i)
		{
			fd = event_responed[i].data.fd;
			if (fd == sockfd_listener)
			{
				smooth_manager_listen_handler();
			}
			else
			{
				smooth_manager_conn_handler(fd);
			}
		}
	}
}
