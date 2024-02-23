/*
 * httpserver.h - global header file
 * author: larkinsb@rhodes.edu
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>


/* 
 * constants 
 */
#define PORTNO       "1266" // default port number
#define TIMEOUT_SECS     10 // keep-alive timeout
#define MAX_MIMETYPES  1024 // used for mime-type lookup
#define BACKLOG           5 // TCP socket backlog
#define BUFSIZE        4096 // general purpose file buffer
#define MAXTHREADS      100 // don't worry having more than 100 threads

#define USE_THREADS
#undef USE_THREADS


#ifdef USE_THREADS
#include <pthread.h>
#endif


/*
 * type definitions
 */

// HTTP method
enum method_e { GET, HEAD };
typedef enum method_e method_t;

// HTTP request status
enum status_e { OK, BadRequest, NotFound, NotImplemented, VersionUnsupported, ServerError };
typedef enum status_e status_t;

// connection status
enum connstatus_e { RecvOK, RecvTimeout, RecvError };
typedef enum connstatus_e connstatus_t;

// request object
struct request_s {
  method_t  method;
  char     *object;
  char     *type;
  int       keepalive;
};
typedef struct request_s request_t;

// used for mime type lookups
struct extension_s {
  char *ext;
  char *type;
};
typedef struct extension_s extension_t;

// arguments to request thread
struct reqthread_s {
  int cfd;
  char *wwwpath;
};
typedef struct reqthread_s reqthread_t;


/* 
 * global variables
 */
extern extension_t mimetypes[MAX_MIMETYPES]; // mime-type lookup table
extern int ecount;                           // number of entries in table
extern char *def_obj;                        // default object for / request


/*
 * function prototypes
 */

// comm.c
void            set_timeout(int fd);
void            set_reuse(int fd);
int             check_buf(char *buf, int len);

// mime.c
void            init_mimetypes(void);
char           *search_mimetypes(char *ext);

// httpserver.c
void           *handle_thread(void *arg);
void            handle_req(int sfd, char *wwwpath);
connstatus_t    get_req(int cfd, char *buf);
status_t        parse_req(char *buf, request_t *req);
void            send_reply(int cfd, status_t sc, request_t *req, char *wwwpath);
