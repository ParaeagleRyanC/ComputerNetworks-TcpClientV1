#include "log.h"
#include "tcp_client.h"
#include "ctype.h"
#include "string.h"

#define REQUIRED_NUMBER_OF_ARGUMENTS_OFFSET 2
#define NUMBER_OF_ACTIONS 5
#define ALL_OPTIONS_PARSED -1
#define SHORT_OPTIONS "vh:p:"
#define HELP_MESSAGE "\n\
    Usage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\
    \n\
    Arguments:\n\
    ACTION   Must be uppercase, lowercase, reverse,\n\
             shuffle, or random.\n\
    MESSAGE  Message to send to the server in \"double quotes\"\n\
    \n\
    Options:\n\
    --help\n\
    -v, --verbose\n\
    --host HOSTNAME, -h HOSTNAME\n\
    --port PORT, -p PORT\n"

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {

    // for debug
    log_debug("There are %d arguments and these are the arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        log_debug("%s\n", argv[i]);
    }

    // set default port and host
    config->port = TCP_CLIENT_DEFAULT_PORT;
    config->host = TCP_CLIENT_DEFAULT_HOST;

    static struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {"verbose", no_argument, 0, 'v'},
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    
    char *actions[NUMBER_OF_ACTIONS] = {"uppercase", "lowercase", "reverse", "shuffle", "random"};
    int getopt_return_value;

    while (1) {
        int option_index = 0;
        getopt_return_value = getopt_long(argc, argv, SHORT_OPTIONS, long_options, &option_index);

        // exit loop if all arguments are parsed
        if (getopt_return_value == ALL_OPTIONS_PARSED) break;

        // loop through optional arguments
        switch (getopt_return_value) {
        case 0:
            printf(HELP_MESSAGE);
            exit(EXIT_SUCCESS);

        case 'v':
            log_info("Verbose is ON\n");
            log_set_level(LOG_TRACE);
            break;

        case 'h':
            config->host = optarg;
            log_info("Host is set to '%s'\n", optarg);
            break;

        case 'p':
            // loop through input port number
            for (size_t i = 0; i < strlen(optarg); i++) {
                // check if input is digit
                if (!isdigit(optarg[i])) {
                    log_error("'%s' is not a valid port\n", optarg);
                    printf(HELP_MESSAGE);
                    exit(EXIT_FAILURE);
                }
            }
            config->port = optarg;
            log_info("Port is set to '%s'\n", optarg);
            break;

        case '?':
            printf(HELP_MESSAGE);
            exit(EXIT_FAILURE);
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", getopt_return_value);
            break;
        }
    }
    
    // check if there is not enough arguments
    if ((argc - optind) < REQUIRED_NUMBER_OF_ARGUMENTS_OFFSET) {
        log_error("Missing argument(s)!\n");
        printf(HELP_MESSAGE);
        exit(EXIT_FAILURE);
    }

    // check if there are too many arguments
    if ((argc - optind) > REQUIRED_NUMBER_OF_ARGUMENTS_OFFSET) {
        log_error("Too many arguments!\n");
        printf(HELP_MESSAGE);
        exit(EXIT_FAILURE);
    }

    // Will be nice to convert input action to lowercase
    char *action = argv[argc - REQUIRED_NUMBER_OF_ARGUMENTS_OFFSET];
    // loop through 5 available actions
    for (int i = 0; i < NUMBER_OF_ACTIONS; i++) {
        // check if input action is a match to one of the available actions
        if (!strcmp(action, actions[i])) {
            config->action = action;
            break;
        }
        // at end of the available action and still no match
        if (i == NUMBER_OF_ACTIONS - 1) {
            log_error("Invalid action!\n");
            printf(HELP_MESSAGE);
            exit(EXIT_FAILURE);
        }
    }

    // set message
    config->message = argv[argc - 1];
    char message[strlen(argv[argc - 1])];
    strcpy(message, argv[argc - 1]);
    sprintf(config->message, "%s %ld %s", config->action, strlen(argv[argc - 1]), message);
    
    // for debug
    if (optind < argc) {
        log_debug("non-option ARGV-elements: ");
        while (optind < argc)
            log_debug("%s ", argv[optind++]);
        log_debug("\n");
    }

    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Creates a TCP socket and connects it to the specified host and port.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_client_connect(Config config) {

    int sockfd; 
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // see if host and port are valid
    if ((rv = getaddrinfo(config.host, config.port, &hints, &servinfo)) != 0) {
        log_error("%s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // error has occurred
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("");
            continue;
        }

        // error has occurred
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("");
            continue;
        }
        break;
    }

    // no connection made
    if (p == NULL) {
        log_error("Failed to connect\n");
        return -1;
    }

    return sockfd;
}



/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    Config config: A config struct with the necessary information.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, Config config) {
    int message_length = strlen(config.message);
    int bytes_sent = 0;
    int total_bytes_sent = 0;
    
    // send until all message is sent
    while (total_bytes_sent < message_length) {
        bytes_sent = send(sockfd, config.message, message_length - total_bytes_sent, 0);
        // check if an error has occurred
        if (bytes_sent == -1) {
            log_error("Send failed!\n");
            exit(EXIT_FAILURE);
        }
        total_bytes_sent += bytes_sent;
    }
    return EXIT_SUCCESS;
}

/*
Description:
    Receives the response from the server. The caller must provide an already allocated buffer.
Arguments:
    int sockfd: Socket file descriptor
    char *buf: An already allocated buffer to receive the response in
    int buf_size: The size of the allocated buffer
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, char *buf, int buf_size) {
    int total_bytes_received = 0;
    int bytes_received = 0;

    // receive until buffer is filled
    while (total_bytes_received < buf_size) {
        bytes_received = recv(sockfd, buf + total_bytes_received, buf_size, 0);
        // check if an error has occurred
        if (bytes_received == -1) {
            log_error("Receive failed!\n");
            exit(EXIT_FAILURE);
        }
        // exit loop if connection is closed
        if (bytes_received == 0) {
            break;
        }
        total_bytes_received += bytes_received;
    }
    buf[total_bytes_received] = '\0';
    return EXIT_SUCCESS;
}

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd) {
    close(sockfd);
    return EXIT_SUCCESS;
}
