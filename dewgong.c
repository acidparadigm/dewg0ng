#include <locale.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strtok_r */
#include <unistd.h>

/* constant for IRC line length */
#define LINE_LEN 510
#define l(x) sizeof(x)/sizeof(int)
#define LOCALE "en_US.UTF-8"

int conn;
char sbuf[LINE_LEN];
typedef struct {
	char* prefix;
	char* nick;
	char* ident;
	char* host;
	char* command;
	char* argv[15];
	int argc;
} request;

void raw (char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, LINE_LEN, fmt, ap);
    va_end(ap);
    printf("<< %s", sbuf);
    write(conn, sbuf, strlen(sbuf));
}

/* chops up *line and returns a struct that contains pointers to the newly
 * modified *line */
request* parse_line (char* line)
{
	/* calloc will set everything to 0, making things safer
	 * and allowing you to test if req->nick is NULL for example
	 */
	request* req = calloc(1, sizeof(request)); sizeof(req);

	char* ptr;
	char* save;

	/* get first word; may either be prefix or command */
	if ( !(ptr = strtok_r(line, " ", &save)) )
		return 0;

	/* check if first word is prefix */
	if (*ptr == ':') {
		req->prefix = ++ptr;

		if(!(ptr = strtok_r(0, " ", &save)))
			return 0;
	}

	req->command = ptr;

	/* get args if any */
	for (; req->argc < 16; ++req->argc) {
		/* if this is a trail, don't split arguments anymore */
		if (*save == ':') {
			req->argv[req->argc] = ++save;
			++req->argc;
			return req;
		} else {
			if (!(ptr = strtok_r(0, " ", &save)))
				return req;

			req->argv[req->argc] = ptr;
		}
	}
	return req;
	puts("Oh bad oh bad oh bad bad bad!");
}

/* chops up req->prefix and sets pointers to modified mask */
void parse_full_mask (request* req)
{
	char* excl = strchr(req->prefix, '!');
	char* at = strchr(req->prefix, '@');

	if (at && at < excl)
		excl = 0;

	if (!at && !excl) {
		/* "hostname.tld" - treat as hostname */
		req->host = req->prefix;
	} else if (!at && excl) {
		/* "nick!ident" */
		*excl = '\0';
		req->nick = req->prefix;
		req->ident = (excl + 1);	//dont dereference excl
	} else if (at && !excl) {
		/* "ident@hostname.tld" */
		*at = '\0';
		req->ident = req->prefix;
		req->host = (at + 1);	   //dont dereference at
	} else {
		/* "nick!ident@hostname.tld" */
		*excl = *at = '\0';
		req->nick = req->prefix;
		req->ident = (excl + 1); //dont dereference excl
		req->host = (at + 1); //dont dereference at
	}
}

void ascii (const char *fileName, const char *channel)
{
	FILE *fd;

	char line[LINE_LEN];

	fd = fopen(fileName, "r");

	if(!fd) return;
	while (fgets(line, LINE_LEN, fd)) {
		raw("PRIVMSG %s :%s", channel, line);
	}

	fclose(fd);
}

void join (const char *chan)
{
	raw("JOIN %s\r\n", chan);
}

void part (const char* chan)
{
	raw("PART %s\r\n", chan);
}

int main (void)
{

	char *locale;
	setlocale(LC_ALL, LOCALE);
	const char nick[] = "dewg0ng";
	const char channel[] = "#dev";
	const char host[] = "irc.supernets.org";
	const char port[] = "6667";

	char *user, *message;

	request *req = malloc(sizeof(request));
	char *where, *sep, *target, *command, *args;
	int i, j, numbytes, o, wordcount;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(host, port, &hints, &res);

	/* this socket needs to be checked for errors. you can't just use it blindly. */
	conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	connect(conn, res->ai_addr, res->ai_addrlen);

	raw("USER %s 0 0 :%s\r\n", nick, nick);

	raw("USER %s 0 0 :%s\r\n", nick, nick);
	raw("NICK %s\r\n", nick);

	while ((numbytes = read(conn, sbuf, LINE_LEN))) {
		sbuf[numbytes] = '\0';
		printf(">> %s", sbuf);
		req = parse_line(sbuf);

		if (req->prefix) {
			/* this populates req->nick and stuff */
			parse_full_mask(req);
		}

		/* strncmp is only useful when comparing two variables
		 * strcmp is safe when comparing a variable with a constant string
		 * like "PING"
		 */
		if (!strcmp(req->command, "PING")) {
			raw("PONG :%s\r\n", req->argv[0]);
		} else if (!strcmp(req->command, "001") && channel != NULL) {
			raw("JOIN %s\r\n", channel);
			raw("PRIVMSG %s :\00304 DEWGONG GONG GONG\00300!\r\n", channel);
		} else if (!strcmp(req->command, "PRIVMSG")) {
			user = req->nick;
			message = req->argv[1];
			where = req->argv[0];
			/* with strdup we're making a new string from the old one, so we can
			 * modify it without modifying the old one
			 */
			command = strdup(req->argv[1]);
			if (strlen(command) > 2)
			{
				command[strlen(command) - 2] = '\0';
			}
			/* this should split !command from the rest of the string,
			 * and store the rest in args. there's probably a better way to do
			 * this though
			 */
			command = strtok_r(command, " ", &args);

			if (where[0] == '#' || where[0] == '&'
			 || where[0] == '+' || where[0] == '!')
				target = where; else target = user;

			printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s",
				user, command, where, target, message);

			if (!strcmp(command, "gong")) {
				puts("A");
				raw("PRIVMSG %s :gong\r\n", target);
			} else if (!strcmp(command, "h")) {
				puts("B");
				raw("PRIVMSG %s :h\r\n", target);
			} else if (!strcmp(message, "smack")) {
				puts("C");
				raw("PRIVMSG %s :smacked %s for \00308,05%d\003 damage!\r\n", target, req->nick, rand()%20 + 1);
			} else if (!strncmp(message, "!ascii ", 6)) {
				puts("D");
				ascii(args, target);
			} else if (!strncmp(message, "!join ", 6)) {
				puts("E");
				join(args);
			} else if (!strncmp(message, "!part ", 6)) {
				puts("F");
				part(args);
			}
		}
	}
	return 0;
}
