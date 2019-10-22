#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#define MAXHOSTNAMELEN 128

/* Prototypes */
void *train_thread(void *arg);
int listen_connection(int port, char *hostName); 
void accept_connections(int fdServer);
void add_station(char *carriage);

/* Global varibales */
char *name; //name of current station
char *authentication; //authentication string
int threadCount = 0; //Number of threads
pthread_t *allThreads; //array containing thread ID's
pthread_mutex_t resourceLock; //mutex for stations resources list
pthread_mutex_t allStationsLock; //mutex for list of stations
pthread_mutex_t doomLock;
pthread_t handlerID; //thread ID for thread handling SIGHUP
sigset_t setting;  //contains SIGHUP signal to be caught by handler 
int summonDoom = 0; //integer indication whether doom train reciecved

enum ExitStatus {
    EXIT_OK = 0,
    EXIT_ARGS = 1,
    EXIT_NAME = 2,
    EXIT_LOG = 3,
    EXIT_PORT = 4,
    EXIT_LISTEN = 5,
    EXIT_CONNECT = 6,
    EXIT_SAMENAME = 7,
    EXIT_UNSPECIFIED = 99
}; 

enum CarriageType { 
    PROCESSED = 0,
    MINE = 1,
    ADD_STATION = 2,
    DOOM = 3,
    STOP = 4,
    RESOURCES = 5,
    WRONG_STATION = 6,
    STATION_NAME = 7,
    WRONG_FORMAT = 8,
    INVALID_NEXT = 9,
    INVALID = 10,
    MATCH = 11,
    MISMATCH = 12
};

struct Log {
    FILE *stream;
    char *fileName;
    int processed;
    int notMine;
    int badFormat;
    int noNext;
    char *shutDown;
    pthread_mutex_t lock;
};
struct Log logFile; // contains all log file information

typedef struct ThreadData {
    int fd;
    FILE *inbound;
    FILE *outbound;
    char *name;
    char connect;
    int verified;
    pthread_t id;
    struct ThreadData *next;
} ThreadData;

ThreadData *firstStation = NULL; //points to the head of list of stations

typedef struct Goods {
    char *name;
    int goods;
    struct Goods *next;
} Goods;

Goods *firstResource = NULL; // points to head in resources linked list

/* Write station's current resources to the log */
void log_resources(Goods *head) {
    Goods *current = head;
    pthread_mutex_lock(&resourceLock);
    while (current->next != NULL) {
        fprintf(logFile.stream, "%s %d\n", current->name, current->goods);
        current = current->next;
    }
    fprintf(logFile.stream, "%s %d\n", current->name, current->goods);
    pthread_mutex_unlock(&resourceLock);
}

/*compares two strings to determine dictionary order 
 * when called by qsort*/
int compare(const void *one, const void *two) {
    return strcmp(*(char**)one, *(char**)two);
}

/* writes to logFile the names of connected stations 
 * in dictionary order */
int log_stations() {
    int valid, count = 0;
    char **stations;
    ThreadData *current;
    int total = threadCount - 1;
    current = firstStation;
    if (firstStation == NULL || !firstStation->verified) { 
    //no stations connected
        return 0;
    }
    stations = malloc(sizeof(char*) * total);
    while (current != NULL) {
        if (current->verified) {
            stations[count] = strdup(current->name);
            count++;
        }
        current = current->next;
    }
    valid = count;//total verified station connections 
    qsort(stations, valid, sizeof(stations), compare);
    for (count = 0; count < valid - 1; ++count) {
        fprintf(logFile.stream, "%s,", stations[count]);
    }
    fprintf(logFile.stream, "%s\n", stations[count]);
    for (count = 0; count < valid; ++count) {
        free(stations[count]); 
    }    
    free(stations);
    return 0;
}

/* writes to logFile the information required */
void write_log() {
    pthread_mutex_lock(&logFile.lock);
    fprintf(logFile.stream, "=======\n%s\n", name);
    fprintf(logFile.stream, "Processed: %d\n", logFile.processed);
    fprintf(logFile.stream, "Not mine: %d\n", logFile.notMine);    
    fprintf(logFile.stream, "Format err: %d\n", logFile.badFormat); 
    fprintf(logFile.stream, "No fwd: %d\n", logFile.noNext); 
    log_stations();
    if (firstResource) {
        log_resources(firstResource);
    }
    if (logFile.shutDown) {
        fprintf(logFile.stream, "%s\n", logFile.shutDown);
    }
    fflush(logFile.stream);
    pthread_mutex_unlock(&logFile.lock);
}
    
/* Adds to the top of the list of the station's 
  * resources and names the new resource */  
void add_on_top(Goods **head, char *resource) {
    Goods *newGoods;
    newGoods = malloc(sizeof(Goods));
    newGoods->goods = 0;
    newGoods->name = strdup(resource);
    newGoods->next = *head;
    *head = newGoods;
}

/*  Adds to the bottom of the list of the station's 
  * resources and names the new resource */ 
void add_on_bottom(Goods *current, char *resource) {
    current->next = malloc(sizeof(Goods));
    current->next->goods = 0;
    current->next->name = strdup(resource);
    current->next->next = NULL;
} 

/* loads or unloads specified resource to the station */
void update_goods(char *goods, Goods *current, char sign) {
    char *words;
    int total = strtol(goods, &words, 10);
    if (sign == '+') {
        current->goods = current->goods + total;
    } else if (sign == '-') {
        current->goods = current->goods - total;
    }
}    

/* adds resource in specified position in the list */
Goods *update(Goods *previous, Goods *current, char *name, int check) {
    if (check == 0) {
        return current;
    } else if (strcmp(name, firstResource->name) < 0) { //add on top
        add_on_top(&firstResource, name);
        current = firstResource;
    } else if (current->next == NULL && check > 0) { //add on botton
        add_on_bottom(current, name);
        current = current->next;
    } else { // add at specific position in list
        previous->next = malloc(sizeof(Goods));
        previous->next->goods = 0;
        previous->next->name = strdup(name);
        previous->next->next = current;
        current = previous->next;
    }
    return current;
}

/*  Adds new resources to the list of resources
 *  * in dictionary order */ 
int update_resources(char *carriage) {
    int check;
    char *name, *goods;
    char *sign, *copy, *key = "+-,";
    Goods *previous;
    Goods *current;
    copy = strdup(carriage);
    name = strtok(copy, key);
    goods = strtok(NULL, key);
    sign = strpbrk(carriage, "+-");
    while (goods != NULL && name != NULL) {
        if (!firstResource) {
            add_on_top(&firstResource, name);
            firstResource->next = NULL;
            update_goods(goods, firstResource, *sign);
        } else {
            previous = firstResource;
            current = firstResource;
            check = strcmp(name, current->name);
            while (current->next != NULL) {
                if (check < 0 || check == 0) {
                    break;
                }
                previous = current;
                current = current->next;
                check = strcmp(name, current->name);
            }
            current = update(previous, current, name, check);
            update_goods(goods, current, *sign);
        }
        name = strtok(NULL, key);
        goods = strtok(NULL, key);
        sign = strpbrk(sign + 1, "+-"); 
    }
    free(copy);
    return 0;
}

/* Takes in exit code of player, prints corresponding exit messages 
 * and exits with the status number*/
void station_exit_message(enum ExitStatus status) {
    const char *message;
    switch (status) {
        case EXIT_ARGS:
            message = "Usage: station name authfile logfile [port [host]]";
            break;                                                    
        case EXIT_NAME:
            message = "Invalid name/auth";
            break;
        case EXIT_LOG:
            message = "Unable to open log";
            break;
        case EXIT_PORT:
            message = "Invalid port";
            break;
        case EXIT_LISTEN:
            message = "Listen error";
            break;
        case EXIT_CONNECT:
            message = "Unable to connect to station";
            break;
        case EXIT_SAMENAME:
            message = "Duplicate station names";
            break;
        case EXIT_UNSPECIFIED:
            message = "Unspecified system call failure";
            break;
        default:
            exit(status);
    }
    fprintf(stderr, "%s\n", message);
    exit(status);
}   

/* checks string is a number only returning 1 if true */
int all_numbers(char *word) {
    int count = 0;
    while (count < strlen(word)) {
        if (!isdigit(word[count])) {
            return 0;
        }
        count++;
    }
    return 1;
}

/* reads an open fd of a socket, returns a
 * partition broken by ':' or a single line if value
 * next = 1 (this means get a whole line of text) */
char *get_a_carriage(FILE *stream, int next, int status[2]) {
    char *token = NULL;
    int count = 0;
    char c = fgetc(stream);
    while (c != '\n') {
        count++;
        token = realloc(token, count);
        if (c == ':' && !next) { //seperate carriages at :
            token[count - 1] = '\0'; 
            break;
        } else if (c < 0) {//a station disconnected
            pthread_exit(NULL);
        } 
        token[count - 1] = c;
        c = fgetc(stream);
    } 
    if (status) {
        //finished with a train (new line)
        status[1] = c == '\n' ? 1 : status[1];
    }
    count++;
    token = realloc(token, count);
    token[count - 1] = '\0'; 
    return token;
}

/* Called when doom train or stopstation occurs 
* to terminate all threads and print to log*/
void shut_down() {
    int thread;
    pthread_mutex_lock(&allStationsLock);
    write_log();
    for (thread = 0; thread < threadCount; thread++) {
        if (allThreads[thread] != pthread_self()) { 
            pthread_cancel(allThreads[thread]);
        }
        pthread_cancel(handlerID);
    }
    pthread_mutex_unlock(&allStationsLock);
}

/* Checks if carriage sepertated by ":" is a station name, 
 * or invalid formarts. If neither it returns mistatch which 
 * indicates its a different type such as add( etc. */
int is_a_station_name(char *carriage, int status[2]) {
    ThreadData *current;
    char *copy = strdup(carriage);
    char *check = strtok(copy, "+-\n()");
    if (strcmp(check, carriage)) {
        free(copy);
        return MISMATCH;
    } else if (!strcmp(carriage, name)) {
        if (!status[1]) {
            //trying to fowarding to itself
            return INVALID_NEXT;
        }
        return MINE;
    } else if (status[2]) { //expecting a carriage
        status[2] = 0;
        if (!strcmp(carriage, "doomtrain")) {
            return DOOM;
        } else if (!strcmp(carriage, "stopstation")) {
            return STOP;
        }
        return MISMATCH;//not of this type
    } else if (status[1]) {
        status[1] = 0;//hasn't finished with the train
        return WRONG_STATION;
    }
    current = firstStation;
    pthread_mutex_lock(&allStationsLock);
    status[1] = 1;//next carriage about to be processed
    while (strcmp(carriage, current->name)) {
        if (current->next == NULL) {
            pthread_mutex_unlock(&allStationsLock);
            return INVALID_NEXT;
        }
        current = current->next;
        if (!current->verified) {
            pthread_mutex_unlock(&allStationsLock);  
            return INVALID_NEXT;
        }
    }
    pthread_mutex_unlock(&allStationsLock);  
    return STATION_NAME; //this station is connected to me
}

/* Checks if formart is for adding resources returning MATCH on 
 * success, INVALID on bad formart and MISTMATCH if its a different type */
int is_add_resources(char *carriage) {
    char *copy, *key = "+-,";
    copy = strdup(carriage);
    char *token = strtok(copy, "+-,");
    char *sign = strpbrk(carriage, key);
    while (token != NULL) {
        if (sign != NULL) { //check if it has - or +
            token = strtok(NULL, key);
            if (!all_numbers(token)) {
                free(copy);
                return INVALID;
            } else if ((int)strspn(sign, "+-") != 1) {
                free(copy);//has more than one sign
                return INVALID;
            }
        } else { 
            free(copy); // no sign
            return INVALID;
        }
        sign = strpbrk(sign + 1, "+-"); //sign of next resource
        token = strtok(NULL, key);//next resource
    }
    free(copy);
    return MATCH;
}

/* Checks if formart is add(...) returning MATCH if true, INVALID bad
 * format and MISTMATCH if false */
int is_add_stations(char *carriage) {
    int check;
    char *host, *port, *token;
    token = strtok(carriage, "("); //seperate stations 
    if (*carriage == '\0') {
        return INVALID;
    } else if (strcmp(token, "add")) {
        return MISMATCH;
    }
    port = strtok(NULL, "@"); //port number
    while (host != NULL && port != NULL) {
        host = strtok(NULL, ",)"); //hostname
        check = all_numbers(port);//check is a number 
        if (check == 0 || !strlen(host) || !strlen(port)) {
            //printf("FAIL port %s check %d %s\n", port, check, host);
            return INVALID;
        }
        port = strtok(NULL, "@"); //port number
    }
    return MATCH;
}

/* checks what format the string is using, with other functions and returns
the type, whether add(...) , station name, bad formart, resources, etc */ 
int carriage_type(char *carriage, int status[2]) {
    int check;
    char *copy = strdup(carriage);
    check = is_a_station_name(copy, status);
    free(copy);
    if (check == STOP) {
        return check;
    } else if (check != MISMATCH) {
        status[1] = check > 7 ? 1 : 0;//1 means next one is a newtrain
        return check;
    }
    status[2] = 0; //alread recieve carriage e.g add, resource 
    // Not a station name
    copy = strdup(carriage);
    check = is_add_stations(copy);
    free(copy);
    if (check == MATCH) {
        return ADD_STATION;
    } else if (check == INVALID) {
        return WRONG_FORMAT;
    }
    //not add station formart
    copy = strdup(carriage);
    check = is_add_resources(copy);
    free(copy);
    if (check == MATCH) {// formart is resource 
        return RESOURCES;
    }
    //not any of the formarts
    return WRONG_FORMAT;
}

/* sends train to next station, the return value just indicates 
 * If success or fail to find that station */
int send_train(char *station, char *train) {
    ThreadData *current;
    current = firstStation;
    pthread_mutex_lock(&allStationsLock);
    while (strcmp(station, current->name)) {
        if (current->next == NULL) {
            pthread_mutex_unlock(&allStationsLock);
            return INVALID_NEXT;
        }
        current = current->next;
    }
    fprintf(current->outbound, "%s:%s\n", station, train);
    fflush(current->outbound);
    pthread_mutex_unlock(&allStationsLock);  
    return STATION_NAME;
}

/* Updates the contents that are to be written to the log upon SIGHUP 
 * according to the specified enum CarriageType */
void update_log(enum CarriageType format) {
    pthread_mutex_lock(&logFile.lock);
    switch (format) {
        case PROCESSED:
           // printf("processed\n");
            logFile.processed++;
            break;
        case WRONG_FORMAT:
          //  printf("BAD FORMART\n");
            logFile.badFormat++;
            break;
        case WRONG_STATION:
         //   printf("wrong station\n");
            logFile.notMine++;
            break;
        case INVALID_NEXT:
        //    printf("no next\n");
            logFile.noNext++;
            break;
        case DOOM:
            logFile.shutDown = "doomtrain";
            logFile.processed++;
            printf("doomtrain\n");
            break;
        case STOP:
            logFile.shutDown = "stopstation";
            logFile.processed++;
            printf("stopstation\n");
        default:
            break;
    }
    pthread_mutex_unlock(&logFile.lock);
}

/* Checks the type of train is and processes it accordingly */
void process_train(FILE *stream, char *token, int type, int status[2]) {
    char *endCarriages = NULL;
    switch (type) {
        case MINE: 
            status[2] = 1;//expecting new carriage
            break;
        case WRONG_STATION:
            !status[1] ? free(get_a_carriage(stream, 1, status)) : (void)0;
            break;
        case STATION_NAME:
            endCarriages = get_a_carriage(stream, 1, status);
            send_train(token, endCarriages);
            break;
        case WRONG_FORMAT:
            !status[1] ? free(get_a_carriage(stream, 1, status)) : (void)0;
            status[1] = 1;
            break;
        case ADD_STATION:
            add_station(token);
            type = PROCESSED;
            break;
        case RESOURCES:
            pthread_mutex_lock(&resourceLock);
            update_resources(token);
            pthread_mutex_unlock(&resourceLock);
            type = PROCESSED;
            break;
        case INVALID_NEXT:
            free(get_a_carriage(stream, 1, status));
            break;
        case DOOM:
            summonDoom = 1;
            update_log(DOOM);
            shut_down(); 
            pthread_exit(NULL);
        case STOP:
            status[0] = 1; //stopstation recieved
    }
    update_log(type);
}

/* Runs the functions which manage the trains coming a station (stream) */
void run_station(FILE *stream, int status[2]) {
    char *token;
    char type;
    status[0] = 0; // 1 = Stopstation
    status[1] = 1; // 1 = newtrain
    status[2] = 0;  // 1 = about to process next carriage e.g resources
    do {
        token = get_a_carriage(stream, 0, status);
        type = carriage_type(token, status);
        process_train(stream, token, type, status);
        free(token);
        token = NULL;
    } while (!status[1]); 
    //while loop exits when finished with a newline of text
}

/* function binds a port to a socket listening for connections
 * returns fd of socket */
int listen_connection(int port, char *hostName) {
    int fd;
    struct sockaddr_in serverAddr;
    int optVal;
    socklen_t size;
    fd = socket(AF_INET, SOCK_STREAM, 0); //IPv4 TCP socket
    if (fd < 0) {
        station_exit_message(EXIT_LISTEN);
    }
    // Allow address (IP addr + port num) to be reused immediately
    optVal = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        station_exit_message(EXIT_LISTEN); 
    }
    // Set up address structure for the server address IPv4, 
    // given port number (network byte order), any IP address
    // on this machine or one specified
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (hostName != NULL) {
        serverAddr.sin_addr.s_addr = inet_addr(hostName);
    } else {
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    }
    size = sizeof(struct sockaddr_in);
    if (bind(fd, (struct sockaddr*)&serverAddr, size) < 0) {
        station_exit_message(EXIT_LISTEN);
    }
    if (port == 0) {
        if (getsockname(fd, (struct sockaddr*)&serverAddr, &size) < 0) {
            station_exit_message(EXIT_LISTEN);
        }
    }
    printf("%d\n", ntohs(serverAddr.sin_port)); //listening port
    fflush(stdout);
    // SOMAXCONN is max num of connection requests to be queued by OS 
    if (listen(fd, SOMAXCONN) < 0) {
        station_exit_message(EXIT_LISTEN);
    }
    return fd;
}

/* removes a station's thread resources from 
 * linked list after it's connection breaks */
int remove_station(pthread_t stationID) {
    ThreadData *previous, *current;
    pthread_mutex_lock(&allStationsLock);
    previous = firstStation;
    current = firstStation;
    while ((current = previous->next) != NULL) {
        if (stationID == current->id) {
            previous->next = current->next;
            free(current); 
            threadCount--;
            pthread_mutex_unlock(&allStationsLock);
            return 0;
        }
        previous = current;
    }
    free(current); 
    threadCount--;
    pthread_mutex_unlock(&allStationsLock);
    return 0;
}

/* creates a new connection with another station. If type = 'S' 
 * it sent a request to connect, it 'R' it accepted a connection */
void new_connection(int fd, char type) {
    pthread_t threadID;
    ThreadData *currentStation;
    // ssetup struct containing info to be used by station thread
    pthread_mutex_lock(&allStationsLock);
    currentStation = firstStation;
    if (firstStation == NULL) {
        firstStation = malloc(sizeof(ThreadData));
        firstStation->fd = fd;
        firstStation->connect = type; 
        firstStation->verified = 0; //value = 1 when station is verified
        firstStation->next = NULL;
        currentStation = firstStation;
    } else {
        while (currentStation->next != NULL) {
            currentStation = currentStation->next;
        }
        currentStation->next = malloc(sizeof(ThreadData));
        currentStation = currentStation->next;
        currentStation->fd = fd;
        currentStation->verified = 0;
        currentStation->connect = type; 
        currentStation->next = NULL;
    }
    pthread_create(&threadID, NULL, train_thread,
            (void*)currentStation);
    pthread_detach(threadID);
    currentStation->id = threadID;
    threadCount++;  
    allThreads = realloc(allThreads, sizeof(pthread_t) * threadCount);
    allThreads[threadCount - 1] = threadID;
    pthread_mutex_unlock(&allStationsLock); 
}

/* Waits for new connections to process and assign a valid socket fd */ 
void accept_connections(int fdServer) {
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    int error, fd;
    char hostname[MAXHOSTNAMELEN];
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            station_exit_message(EXIT_UNSPECIFIED);
        }
        error = getnameinfo((struct sockaddr*)&fromAddr, fromAddrSize,
                hostname, MAXHOSTNAMELEN, NULL, 0, 0);
        if (error) {
            station_exit_message(EXIT_UNSPECIFIED);
        }
        if (fd > 2) {
            new_connection(fd, 'R'); //R means reciving a connection
        }
    }
}

/* populates an address struct with the hostname's info */
struct in_addr *ip_address_name(char *hostname) {
    int error;
    struct addrinfo *addressInfo;
    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if (error) {
        return NULL;
    }
    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

/* connect to a known ipAddress through station's server port
 * returns socket fd*/
int connect_to(struct in_addr *ipAddress, int port) {
    struct sockaddr_in socketAddr;
    int fd;  //IS IT OK TO USE fd 
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        station_exit_message(EXIT_CONNECT);
    }
    //populate the address structure
    socketAddr.sin_family = AF_INET;    // ip v4 type address
    socketAddr.sin_port = htons(port);  // port num - network byte order
    socketAddr.sin_addr.s_addr = ipAddress->s_addr; 
    if (connect(fd, (struct sockaddr*)&socketAddr,
            sizeof(socketAddr)) < 0) {
        station_exit_message(EXIT_CONNECT);
    }
    return fd;
}

/* checks if new connection sends a duplicate station name */
int duplicate_check(char *newStation) {
    ThreadData *current = firstStation;
    int count = 0;
    !strcmp(name, newStation) ? station_exit_message(EXIT_SAMENAME) : (void)0;
    while (current != NULL && current->verified) {
        if (!strcmp(current->name, newStation)) {
            count++; // if 0 < means someone else has this name
        }
        current = current->next;
    }
    count > 0 ? station_exit_message(EXIT_SAMENAME) : (void)0;
    return 0;
}

/* checks if a newly connected station has the correct 
 * authentication string */
void verify_station(ThreadData *station) {
    char *password;
    if (station->connect == 'R') {
        password = get_a_carriage(station->inbound, 1, 0);
        station->name = get_a_carriage(station->inbound, 1, 0);
        if (strcmp(authentication, password)) {
            station_exit_message(EXIT_NAME);
        } else { //recived connection request
            fprintf(station->outbound, "%s\n", name);
            fflush(station->outbound);
            duplicate_check(station->name);
        }
    } else { //sent connection request
        fprintf(station->outbound, "%s\n%s\n", authentication, name);
        fflush(station->outbound);
        station->name = get_a_carriage(station->inbound, 1, 0);
        duplicate_check(station->name);
    }
}

/* sends a connection request to add a new station */
void add_station(char *carriage) {
    struct in_addr *ipAddress;
    char *word, *key = "(@,)";
    int port, fd;
    char *token = strtok(carriage, key); //seperate stations 
    token = strtok(NULL, key); //token = port number
    while (token != NULL) {
        port = strtol(token, &word, 10); 
        token = strtok(NULL, key); // token = hostname 
        ipAddress = ip_address_name(token);
        if (!ipAddress) {
            station_exit_message(EXIT_CONNECT);
        }
        fd = connect_to(ipAddress, port);
        new_connection(fd, 'S'); //sent a request to connect
        token = strtok(NULL, key); //token = port number
    }
}

/* release system resources upon threads exit such a fd 
 * in order for them to be used by new threads */
void clean_handler(void *data) {
    ThreadData *station;
    station = (ThreadData*)data;
    if (summonDoom) {
        fprintf(station->outbound, "%s:doomtrain\n", station->name);
        fflush(station->outbound);
    }
    close(station->fd);
    remove_station(station->id);
}

/* function executed in thread, to send and 
 * recieve trains from a station */
void *train_thread(void *data) {
    int status[2] = {0};
    ThreadData *station;
    station = (ThreadData*)data;
    pthread_cleanup_push(clean_handler, (void*)station);
    station->outbound = fdopen(station->fd, "w");
    station->inbound = fdopen(station->fd, "r");
    verify_station(station);
    station->verified = 1;
    while (!status[0]) {// while loop exits when stopstation recieved
        run_station(station->inbound, status);
    }
    shut_down(); 
    pthread_cleanup_pop(0);
    pthread_exit(NULL);
}

/* Called by thread specified to handle SIGHUP
 * in order to write to logFile */
void *handle_signal(void *arg) {
    int signalType;
    while (1) {
        sigwait(&setting, &signalType);
        if (signalType == SIGHUP) {
            write_log();
            signalType = 0;
        }
    }
}

/* initiates station as a server, which waits for new connections 
 * and initialises mutext lock to be used by threads */
void initiate_station(int port, char *hostName) {
    int station;
    threadCount++;  
    allThreads = realloc(allThreads, sizeof(pthread_t) * threadCount);
    allThreads[threadCount - 1] = pthread_self();

    sigemptyset(&setting);     // Ignore SIGHUP in main thread
    sigaddset(&setting, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &setting, NULL);
    pthread_create(&handlerID, NULL, handle_signal, NULL);
    pthread_detach(handlerID);

    pthread_mutex_init(&allStationsLock, NULL);
    pthread_mutex_init(&resourceLock, NULL);
    pthread_mutex_init(&logFile.lock, NULL);
    station = listen_connection(port, hostName);
    accept_connections(station);
    pthread_mutex_destroy(&allStationsLock);
    pthread_mutex_destroy(&resourceLock);
}

/* Checks arguments are valid, sets name of station, opens logFile,
 * prepares arguments for other functions to use to set up the
 * server to listen for connections */
int setup_station(int allInputs, char *inputs[]) {
    int portNumber;
    char *text, *fileName, *hostName;
    FILE *stream;
    size_t size = 0;
    ssize_t bytesRead;
    //check arguments and thi station's name
    if (allInputs < 4 || allInputs > 6) {
        station_exit_message(EXIT_ARGS);
    }
    name = strdup(inputs[1]);
    *name ? (void)0 : station_exit_message(EXIT_NAME); 
    //setup authentication string
    fileName = inputs[2];
    stream = fopen(fileName, "r");
    stream == NULL ? station_exit_message(EXIT_NAME) : (void)0;
    bytesRead = getline(&authentication, &size, stream);
    bytesRead > 1 ? (void)0 : station_exit_message(EXIT_NAME); 
    authentication[strlen(authentication) - 1] = '\0'; //remove newline
    fclose(stream);
    //setup logfile attributes
    logFile.fileName = inputs[3];
    logFile.stream = fopen(logFile.fileName, "a");
    logFile.stream == NULL ? station_exit_message(EXIT_LOG) : (void)0;
    logFile.processed = 0;
    logFile.noNext = 0;
    logFile.badFormat = 0;
    logFile.notMine = 0;
    logFile.shutDown = NULL;
    //set up port number for listen
    bytesRead >= 1 ? (void)0 : station_exit_message(EXIT_NAME); 
    portNumber = 0;
    if (allInputs > 4) {
        portNumber = strtol(inputs[4], &text, 10);
        if (portNumber <= 0 || portNumber >= 65535 || *text != 0) {
            station_exit_message(EXIT_PORT);
        }
    }
    hostName = allInputs == 6 ? inputs[5] : NULL;
    initiate_station(portNumber, hostName);
    free(name);
    return 0;
}

int main(int argc, char *argv[]) {
    setup_station(argc, argv);
}


