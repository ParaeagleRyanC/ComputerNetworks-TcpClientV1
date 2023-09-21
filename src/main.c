#include <stdio.h>

#include "tcp_client.h"
#include "log.h"

int main(int argc, char *argv[]) {
    
    log_set_level(LOG_ERROR);

    Config config;
    char buffer[TCP_CLIENT_MAX_INPUT_SIZE];

    tcp_client_parse_arguments(argc, argv, &config);
    int sockfd = tcp_client_connect(config);
    tcp_client_send_request(sockfd, config);
    tcp_client_receive_response(sockfd, buffer, TCP_CLIENT_MAX_INPUT_SIZE);

    printf("%s\n", buffer);

    return tcp_client_close(sockfd);
}
