#ifndef _MESSAGE_DEF_H_
#define _MESSAGE_DEF_H_

/*
 * int ===> version
 * int ===> total length
 * int ===> message type
 */
struct message_head
{
	int version;
	int length;
	int type;
};

/*
 * int ===> message type
 * struct ===> message
 */

struct message_body
{

};

/*
 * string ===> account
 * string ===> password
 */
struct message_login
{

};

/*
 * string ===> who
 * string ===> message
 */
struct message_chart_text
{

};



#endif
