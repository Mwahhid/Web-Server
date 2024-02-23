/*
 * httpserver.c - a simple HTTP server program
 * author: Mwahhid Majeed
 *
 */

#include "httpserver.h"

/*
 * global variables
 */
extension_t mimetypes[MAX_MIMETYPES];
int ecount = 0;
char *def_obj = "/index.html";

/*
 * parse_req - parses and validates an HTTP request
 *
 * @param buf buffer containing a full request from the client
 * @param req a request object filled out by this function
 * @returns a status code corresponding to the validity of the request
 *
 */
status_t parse_req(char *buf, request_t *req) {
  status_t rc = OK;

  /* implement me */
  char *tok, *reqline, *method, *url, *version;
  int host_present = 0;
  reqline = strtok(buf, "\r\n");

  if (reqline == NULL)
    return BadRequest;

  tok = strtok(NULL, "\n");
  while (tok != NULL) { // Looping line by line over the HTTP request
    if (host_present == 0 && strncmp(tok, "Host:", 5) == 0)
      host_present = 1;
    else if (strncmp(tok, "Connection: keep-alive", 22) == 0)
      req->keepalive = 1; // Setting keep-alive flag
    tok = strtok(NULL, "\n");
  }

  if (host_present == 0)
    return BadRequest;

  method = strtok(reqline, " ");
  url = strtok(NULL, " ");
  version = strtok(NULL, " ");
  // Verifying the method
  if (strcmp(method, "GET") == 0)
    req->method = GET;
  else if (strcmp(method, "HEAD") == 0)
    req->method = HEAD;
  else
    return BadRequest;

  if (strcmp(url, "/") == 0) {
    req->object = strdup("/index.html");
    req->type = search_mimetypes("html");
  } else {
    req->object = strdup(url);
    tok = strrchr(url, '.'); // points to the last '.' character in url
    tok++;                   // points to the first character of extension
    req->type = search_mimetypes(tok);
  }

  if (strncmp(version, "HTTP/1.0", 8) != 0 &&
      strncmp(version, "HTTP/1.1", 8) != 0)
    return VersionUnsupported;

  return rc;
}

/*
 * send_reply - sends a reply to the client based on the requested resource
 *
 * @param cfd client socket file descriptor
 * @param req request object completed by parse_req
 * @param wwwpath path to the directory containing the web documents
 *
 */
void send_reply(int cfd, status_t sc, request_t *req, char *wwwpath) {

  /* implement me */
  char retHeader[1024];
  char filePath[strlen(wwwpath) + strlen(req->object) + 1];
  char *httpStatus;
  int filePresent;
  long fileSize;
  time_t currTime;
  char dateStr[50];
  char lastModStr[50];
  char contStr[50];

  struct tm *timeInfo;
  struct stat filemd;

  // Creating the full file path
  strcpy(filePath, wwwpath);
  strcat(filePath, req->object);

  printf("%s\n", req->object);

  switch (sc) {
  case BadRequest:
    httpStatus = "400 Bad Request";
    break;
  case NotFound:
    httpStatus = "404 Not Found";
    break;
  case NotImplemented:
    httpStatus = "501 Not Implemented";
    break;
  case VersionUnsupported:
    httpStatus = "505 HTTP Version Not Supported";
    break;
  case ServerError:
    httpStatus = "500 Internal Server Error";
    break;
  default:
    httpStatus = "200 OK"; // else case is OK
  }

  // Checking if file exists and getting file metadata
  if (stat(filePath, &filemd) == 0) {
    fileSize = filemd.st_size;
    filePresent = 1;
  } else {
    filePresent = 0;
    httpStatus = "404 Not Found";
  }

  // Get current time and convert to UTC
  time(&currTime);
  timeInfo = gmtime(&currTime);

  if (timeInfo == NULL) {
    perror("error with gmtime");
    exit(1);
  }

  // Create the date header string
  strftime(dateStr, sizeof(dateStr), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n",
           timeInfo);

  timeInfo = gmtime(&filemd.st_mtim.tv_sec);
  if (timeInfo == NULL) {
    perror("error with gmtime");
    exit(1);
  }

  // Create last modified header string
  strftime(lastModStr, sizeof(lastModStr),
           "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n", timeInfo);

  // Put everything together to form header
  strcpy(retHeader, "HTTP/1.1 ");
  strcat(retHeader, httpStatus);
  strcat(retHeader, "\r\n");

  if (req->keepalive)
    strcat(retHeader, "Connection: keep-alive\r\n");
  else
    strcat(retHeader, "Connection: close\r\n");

  strcat(retHeader, dateStr);
  if (filePresent) {
    strcat(retHeader, lastModStr);

    sprintf(contStr, "Content-Length: %ld\r\n", fileSize);
    strcat(retHeader, contStr);

    contStr[0] = '\0'; // Cleared the string for reuse
    sprintf(contStr, "Content-Type: %s\r\n\r\n", req->type);
    strcat(retHeader, contStr);
  }

  printf("%s", retHeader); // Print out the Headers

  if (write(cfd, retHeader, strlen(retHeader)) == -1) {
    perror("Error writing headers to client");
    exit(1);
  }

  retHeader[0] = '\0'; // Cleared the string for reuse

  if (filePresent && req->method == GET) {
    int readBytes;
    int filefd = open(filePath, O_RDONLY);
    if (filefd == -1) {
      perror("Error with opening file");
      exit(1);
    }

    while ((readBytes = read(filefd, retHeader, sizeof(retHeader))) > 0) {
      int wrtBytes = write(cfd, retHeader, readBytes);
      if (wrtBytes < 0) {
        perror("Error writing to client");
        exit(1);
      }
    }

    if (readBytes == -1) {
      perror("Error reading file");
      exit(1);
    }

    close(filefd);
  }
  free(req->object);
}

/*
 * get_req - reads a complete HTTP request into a buffer
 *
 * @param cfd client socket file descriptor
 * @param buf buffer to read request into
 * @returns RecvOK if successful, RecvTimeout on timeout, RecvError on error
 *
 */
connstatus_t get_req(int cfd, char *buf) {
  int nbytes, rbytes, remaining;
  connstatus_t rv = RecvOK;

  rbytes = 0;
  remaining = BUFSIZE;

  do {
    nbytes = recv(cfd, &buf[rbytes], remaining, 0);
    if ((nbytes == -1) && (errno == EAGAIN)) {
      rv = RecvTimeout;
      goto done;

    } else if (nbytes == -1) {
      perror("recv");
      rv = RecvError;
      goto done;
    }

    rbytes += nbytes;
    remaining -= nbytes;

  } while (check_buf(buf, rbytes));

done:
  return rv;
}

/*
 * handle_req - handles a one or more HTTP requst/reply pairs with a client
 *
 * @param cfd client socket file descriptor
 * @param wwwpath path to the directory containing the web documents
 *
 */
void handle_req(int cfd, char *wwwpath) {
  char *buf = malloc(BUFSIZE);
  status_t rc = OK;
  connstatus_t cs;
  request_t req;
  int keepalive = 0;

  set_timeout(cfd);

  cs = get_req(cfd, buf);
  if (cs != RecvOK)
    goto done;

  rc = parse_req(buf, &req);
  keepalive = req.keepalive;
  send_reply(cfd, rc, &req, wwwpath);

  while (keepalive) {
    memset(buf, 0, BUFSIZE);

    cs = get_req(cfd, buf);
    if (cs != RecvOK)
      goto done;

    rc = parse_req(buf, &req);
    send_reply(cfd, rc, &req, wwwpath);
  }

done:
  close(cfd);
  free(buf);
}

/*
 * handle_thread - wrapper function for pthreads calling handle_req
 * @param arg - a reqthread_t object with the client fd and www path
 * @returns NULL
 */
void *handle_thread(void *arg) {
  reqthread_t *rt = (reqthread_t *)arg;

  handle_req(rt->cfd, rt->wwwpath);
  close(rt->cfd);
  return NULL;
}

int main(int argc, char **argv) {
  char *portstr = strdup(PORTNO);
  char *wwwpath = strdup("htdocs");
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in them;
  socklen_t themlen;
  char ch, s[INET_ADDRSTRLEN];
  int ret, sfd = -1, afd = -1;
  int threadcount = 0;
  pthread_t tids[MAXTHREADS];
  reqthread_t rt[MAXTHREADS];

  while ((ch = getopt(argc, argv, "d:p:")) != -1) {
    switch (ch) {
    case 'd':
      wwwpath = strdup(optarg);
      break;
    case 'p':
      portstr = strdup(optarg);
      break;
    default:
      fprintf(stderr, "usage: httpserver [-p port] [-d directory]\n");
    }
  }

#ifdef USE_THREADS
  printf("httpserver starting (threaded)\n");
#else
  printf("httpserver starting (non-threaded)\n");
#endif

  init_mimetypes();

  /* implement me */

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; // use IPv4
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill in my IP for me

  int status;
  if ((status = getaddrinfo(NULL, portstr, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  // Creating a socket
  sfd =
      socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (sfd == -1) {
    perror("error creating socket \n");
    exit(1);
  }

  set_reuse(sfd); // Setting options for the socket

  // Binding socket with a port
  status = bind(sfd, servinfo->ai_addr, servinfo->ai_addrlen);
  if (status == -1) {
    perror("error with bind\n");
    exit(1);
  }

  // Listening for incoming requests
  status = listen(sfd, BACKLOG);
  if (status == -1) {
    perror("error with listen\n");
    exit(1);
  }

  printf("Server listening on port %s\n", portstr);

  // Accept incoming requests
  themlen = sizeof them;
  afd = accept(sfd, (struct sockaddr *)&them, &themlen);
  if (afd == -1) {
    perror("error with accept\n");
  }

  inet_ntop(AF_INET, &(them.sin_addr), s, INET_ADDRSTRLEN);
  printf("received connection from %s\n", s);

  /* example code to have a single-thread or multi-thread server */

#ifdef USE_THREADS
  rt[threadcount].cfd = afd;         // client file descriptor
  rt[threadcount].wwwpath = wwwpath; // hdocs path
  pthread_create(&tids[threadcount], NULL, handle_thread,
                 &rt[threadcount]); // create thread
  threadcount++;                    // increment thread cont
  assert(threadcount < MAXTHREADS); // die if too many threads
#else
  handle_req(afd, wwwpath); // single-thread -- handle request
#endif /* USE_THREADS */

  free(wwwpath);
  free(portstr);
  freeaddrinfo(servinfo);
  close(afd);
  close(sfd);
  return 0;
}
