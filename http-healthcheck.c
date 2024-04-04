#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

static char *const USAGE = "Usage: %s [ -v ] [ -p <port> ] [ <path> ]\n";

static const size_t MAX_PATH_LENGTH = 2048;
static const char REQUEST_PREFIX[] = "GET /";
static const char REQUEST_SUFFIX[] = " HTTP/1.1\r\nHost: localhost\r\n\r\n";

int verbose = 0;
int port = 80;
const char* path = "/";
size_t path_len = 1;


void parse_args(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "vp:h?")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 'p': {
                char *endptr;
                long p = strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0' || p <= 0 || p > 65535) {
                    fprintf(stderr, "Invalid port number \"%s\"\n", optarg);
                    exit(EXIT_FAILURE);
                }
                port = (int) p;
            }
                break;
            case 'h':
            case '?':
                fprintf(stdout, USAGE, argv[0]);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, USAGE, argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        path = argv[optind];
        path_len = strlen(path);
        if (path_len > MAX_PATH_LENGTH) {
            fprintf(stderr, "Path too long (%zu > max %zu): \"%s\"\n", path_len, MAX_PATH_LENGTH, path);
            exit(EXIT_FAILURE);
        }
    }

    if ('/' == path[0]) {
        path++;
        path_len--;
    }
}

int create_socket_and_connect(const char *ip) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return -1;
    }

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

ssize_t send_request(int sock) {
    size_t req_len = sizeof(REQUEST_PREFIX) - 1 + path_len + sizeof(REQUEST_SUFFIX) - 1;
    char req[req_len];
    snprintf(req, req_len, "%s%s%s", REQUEST_PREFIX, path, REQUEST_SUFFIX);

    return send(sock, req, req_len, 0);
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    if (verbose) fprintf(stdout, "GET http://localhost:%d/%s\n", port, path);

    int sock = create_socket_and_connect("127.0.0.1");
    if (sock < 0) exit(EXIT_FAILURE);

    if (send_request(sock) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char res[256];
    ssize_t bytes_received;

    bytes_received = recv(sock, res, sizeof(res) - 1, 0);
    if (bytes_received < 0) {
        perror("Recv failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    close(sock);
    res[bytes_received] = '\0';

    size_t eol = strcspn(res, "\r\n");
    // "HTTP/1.x 200 "
    if (eol < 13
        || 0 != strncmp("HTTP/1.", res, 7)
        || ('0' != res[7] && '1' != res[7])
        || (' ' != res[8])) {
        fprintf(stderr, "Unexpected response %zu\n", eol);
        fwrite(res, 1, bytes_received, stderr);
        exit(EXIT_FAILURE);
    }

    if (0 != strncmp("200", res + 9, 3) || !isspace(res[12])) {
        fwrite(res, 1, eol, stderr);
        exit(EXIT_FAILURE);
    }
    if (verbose) fwrite(res, 1, eol, stdout);
    return 0;
}
