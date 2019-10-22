/*  Adds a new resource to the list of resources
 * in dictionary order */ 

void proccess_goods(char *goods) {
    char *token, *carriage;
    token = strtok(goods, ",");
    while (token != NULL) {
        carriage = strdup(token);
        update_resource(carriage);
        free(carriage);
        token = strtok(NULL, ",")
    }
}

int update_resource(char *carriage) {
    char *goods, *name, *token, *resource;
    Resource *previous;
    Resource *current;
    int check;
    goods = strpbrk(token, "+-");
    name = strtok(resource, "+-");
    if (!firstResource) {
        add_on_top(&firstResource, name);
        firstResource->next = NULL;
        update_goods(goods, firstResource);
    } else {
        previous = firstResource;
        current = firstResource;
        check = strcmp(name, current->name);
        while (current->next != NULL) {
            if (check < 0) {
                break;
            } else if (check == 0) {
                update_goods(goods, current);
            }
            previous = current;
            current = current->next;
            check = strcmp(name, current->name);
        }
        if (strcmp(name, firstResource->name) < 0) {
            add_on_top(&firstResource, name);
            current = firstResource;
        } else if (current->next == NULL && check > 0) {
            add_on_bottom(name, current);
        } else { // add at specific position in list
            previous->next = malloc(sizeof(Resource));
            previous->next->name = name;
            previous->next->goods = 0;
            previous->next->next = current;
            current = previous->next;
        }
        update_goods(goods, current);
    }
    return 0;
}

/* checks if ":" seperated sub strings have formart of resources, 
 * station name, add(,,) or invalid. returns 1, 2, 3 or 0 respectively.*/
int carriage_type(char *carriage) {
    char *copy = strdup(carriage);
    if (all_letters(copy) {//station name
        free(copy);
        return 2;
    } else if (is_add_resources(copy)) {// formart 
        free(copy);
        return 1;
    } else if(is_add_stations(copy)) {
        free(copy);
        return 3;
    }
    free(copy);
    return 0;
}

void check_goods(char *goods) {
    char *token, *carriage, *copy;
    copy = strdup(goods);
    token = strtok(copy, ",");
    while (token != NULL) {
        carriage = strdup(token);
        token = strtok(NULL, ",")
        if(is_add_resources(carriage) && token == NULL) {
            free(carriage);
            return 1
        }
        free(carriage);
    }
    free(copy);
    copy = strdup(goods);
    token = strtok(copy, ",");
    while (token != NULL) {
        carriage = strdup(token);
        token = strtok(NULL, ",")
        if(is_add_stations(carriage) && token == NULL) {
            free(carriage);
            return 2
        }
        free(carriage);
    }
    free(copy);
}



/* Takes in exit code of player, prints corresponding exit messages 
 * and returns the exit status number*/
void station_exit_message(enum ExitStatus status) {
    const char *message;
    switch (status) {
        case EXIT_ARGS:
            message = "Usage: station name authfile log logfile"
                    "[port] [host]";
            break;                                                    
        case EXIT_NAME:
            message = "Invalid name/auth";
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
        case EXIT_UNSPECIFIED:
            message = "Unspecified system call failure";
            break;
        default:
            exit(status);
    }
    fprintf(stderr, "%s\n", message);
    exit(status);
}   

enum TrainCheck {
    PROCESSED = 0,
    WRONG_STATION = 1,
    WRONG_FORMAT = 2,
    INVALID_NEXT = 3
};

/* checks string is alphabetic only, returning 1 on success */
int all_letters(char *word) {
    int count = 0;
    while(count < strlen(word)) {
        if (!isalpha(word[count])) {
            return 0;
        }
        count++;
    }
    return 1;
}

/* checks string is digits only, returning 1 on success */
int all_numbers(char *word) {
    int count;
    while(count < strlen(word)) {
        if (!isdigit(word[count])) {
            return 0;
        }
        count++;
    }
    return 1;
}


/* Checks if formart is for adding resources
 * returns 0 if False and 1 if True */
int is_add_resources(char *resource) {
    char *verify, *resource, *key = "+";
    char *token = strbrk(carriage, ",");
    verify = strtok(resource, key);
    if (!strcmp(verify, carriage)) {//check if it has +
        key = "-";
        resource = strtok(resource, key);
    }
    if (strcmp(verify, carriage)) { //check if it has -
        if (!all_letters(verify)) {//valid resource name ?
            return 0; //means not resources
        }
        verify = strtok(NULL, key);
        if (!all_numbers(verify)) {
            return 0;
        }
    } else { 
        return 0;
    }
    return 1;
}

/* Checks if formart is for adding stations
 * returns 0 if False and 1 if True */
int is_add_stations(char *carriage) {
    int check;
    char *copy, *token;
    char *name;//CHECK for strmpN 
    if (strcmp(station, "add")) {
        return 0;
    }
    station = strbrk(carriage,",)");
    station++;
    while (station != NULL) {
        copy = strdup(station);
        token = strtok(copy, "@"); 
        check = all_numbers(token);//check port is a number 
        token = strtok(NULL, ","); // check hostname given
        if(check == 0 || *token == '\0') {
            free(copy);
            return 0;
        }
        free(copy);
        station = strtok(NULL, ",)"); //station = next one to add
        station++;
    }
    return 1;
}
