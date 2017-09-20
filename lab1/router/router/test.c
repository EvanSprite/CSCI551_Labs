
#ifdef _SOLARIS_
#define __EXTENSIONS__
#endif /* _SOLARIS_ */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

#ifdef _LINUX_
#include <getopt.h>
#endif /* _LINUX_ */

#include "sr_dumper.h"
#include "sr_router.h"
#include "sr_rt.h"

#define DEFAULT_RTABLE "rtable"
#define DEFAULT_TOPO 0
#define DEFAULT_HOST "vrhost"

static void usage(char* );
static void sr_init_instance(struct sr_instance* );
static void sr_destroy_instance(struct sr_instance* );
static void sr_set_user(struct sr_instance* );
static void sr_load_rt_wrap(struct sr_instance* sr, char* rtable);

struct in_addr longest_prefix_match(struct sr_instance *sr, struct in_addr target);

struct sr_instance * setup(int argc, char **argv){
	 int c;
	 char *host   = DEFAULT_HOST;
    char *user = 0;
    char *rtable = DEFAULT_RTABLE;
    char *template = NULL;

    unsigned int topo = DEFAULT_TOPO;
    char *logfile = 0;
    struct sr_instance sr;

    /* -- zero out sr instance -- */
    sr_init_instance(&sr);

    /* -- set up routing table from file -- */
    if(template == NULL) {
        sr.template[0] = '\0';
        sr_load_rt_wrap(&sr, rtable);
    }
    else
        strncpy(sr.template, template, 30);

    sr.topo_id = topo;
    strncpy(sr.host,host,32);

    if(! user )
    { sr_set_user(&sr); }
    else
    { strncpy(sr.user, user, 32); }

    

    if(template != NULL && strcmp(rtable, "rtable.vrhost") == 0) { /* we've recv'd the rtable now, so read it in */
        Debug("Connected to new instantiation of topology template %s\n", template);
        sr_load_rt_wrap(&sr, "rtable.vrhost");
    }
    else {
      /* Read from specified routing table */
      sr_load_rt_wrap(&sr, rtable);
    }

    /* call router init (for arp subsystem etc.) */
    sr_init(&sr);

    /* -- whizbang main loop ;-) */
    while( sr_read_from_server(&sr) == 1);

    sr_destroy_instance(&sr);

    return &sr;
}

int main(int argc, char **argv)
{
    printf("hello world: in main\n");
    struct sr_instance *sr;
    sr = setup(argc, argv);

    in_addr_t x;

  	char *z; /* well set this equal to the IP string address returned by inet_ntoa */

  	char *y = (char *)&x; /* so that we can address the individual bytes */

  	y[0] = 107;
  	y[1] = 23;
  	y[2] = 53;
  	y[3] = 142;

  	z = inet_ntoa(*(struct in_addr *)&x); /* cast x as a struct in_addr */

  	printf("z = %s\n", z);

    struct in_addr ip;
    ip.s_addr = 1072353142;
    bool found = longest_prefix_match(sr, ip);
    /*longest_prefix_match(3);*/
    return 0;
}/* -- main -- */



/*-----------------------------------------------------------------------------
 * Method: sr_set_user(..)
 * Scope: local
 *---------------------------------------------------------------------------*/

void sr_set_user(struct sr_instance* sr)
{
    uid_t uid = getuid();
    struct passwd* pw = 0;

    /* REQUIRES */
    assert(sr);

    if(( pw = getpwuid(uid) ) == 0)
    {
        fprintf (stderr, "Error getting username, using something silly\n");
        strncpy(sr->user, "something_silly", 32);
    }
    else
    {
        strncpy(sr->user, pw->pw_name, 32);
    }

} /* -- sr_set_user -- */

/*-----------------------------------------------------------------------------
 * Method: sr_destroy_instance(..)
 * Scope: Local
 *
 *
 *----------------------------------------------------------------------------*/

static void sr_destroy_instance(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    if(sr->logfile)
    {
        sr_dump_close(sr->logfile);
    }

    /*
    fprintf(stderr,"sr_destroy_instance leaking memory\n");
    */
} /* -- sr_destroy_instance -- */

/*-----------------------------------------------------------------------------
 * Method: sr_init_instance(..)
 * Scope: Local
 *
 *
 *----------------------------------------------------------------------------*/

static void sr_init_instance(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    sr->sockfd = -1;
    sr->user[0] = 0;
    sr->host[0] = 0;
    sr->topo_id = 0;
    sr->if_list = 0;
    sr->routing_table = 0;
    sr->logfile = 0;
} /* -- sr_init_instance -- */

/*-----------------------------------------------------------------------------
 * Method: sr_verify_routing_table()
 * Scope: Global
 *
 * make sure the routing table is consistent with the interface list by
 * verifying that all interfaces used in the routing table actually exist
 * in the hardware.
 *
 * RETURN VALUES:
 *
 *  0 on success
 *  something other than zero on error
 *
 *---------------------------------------------------------------------------*/

int sr_verify_routing_table(struct sr_instance* sr)
{
    struct sr_rt* rt_walker = 0;
    struct sr_if* if_walker = 0;
    int ret = 0;

    /* -- REQUIRES --*/
    assert(sr);

    if( (sr->if_list == 0) || (sr->routing_table == 0))
    {
        return 999; /* doh! */
    }

    rt_walker = sr->routing_table;

    while(rt_walker)
    {
        /* -- check to see if interface exists -- */
        if_walker = sr->if_list;
        while(if_walker)
        {
            if( strncmp(if_walker->name,rt_walker->interface,sr_IFACE_NAMELEN)
                    == 0)
            { break; }
            if_walker = if_walker->next;
        }
        if(if_walker == 0)
        { ret++; } /* -- interface not found! -- */

        rt_walker = rt_walker->next;
    } /* -- while -- */

    return ret;
} /* -- sr_verify_routing_table -- */

static void sr_load_rt_wrap(struct sr_instance* sr, char* rtable) {
    if(sr_load_rt(sr, rtable) != 0) {
        fprintf(stderr,"Error setting up routing table from file %s\n",
                rtable);
        exit(1);
    }


    printf("Loading routing table\n");
    printf("---------------------------------------------\n");
    sr_print_routing_table(sr);
    printf("---------------------------------------------\n");
}
