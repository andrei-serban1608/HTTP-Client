#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define SERVER_IP "34.254.242.81"
#define SERVER_PORT 8080

char *session_cookie = NULL;
char *JWT_token = NULL;

int isNumber(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (strchr("0123456789", str[i]) == NULL) {
            return 0;
        }
    }
    return 1;
}

int hasSpaces(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') {
            return 1;
        }
    }
    return 0;
}

void handle_register(int sockfd)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *data = NULL;
    char *buffer = calloc(LINELEN, sizeof(char));
    char *message;
    char *response;

    printf("username=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if (strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        return;
    }
    if (hasSpaces(buffer)) {
        printf("Invalid username. Exited\n");
        free(buffer);
        return;
    }
    json_object_set_string_with_len(root_object, "username", buffer, strlen(buffer));
    printf("password=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if (strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        return;
    }
    if (hasSpaces(buffer)) {
        printf("Invalid password. Exited\n");
        free(buffer);
        return;
    }
    json_object_set_string_with_len(root_object, "password", buffer, strlen(buffer));
    data = json_serialize_to_string_pretty(root_value);
    message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/register", "application/json", data, NULL, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // Verific prima cifra a codului de return din raspunsul serverului
    if (response[9] == '4') {
        printf("Username already taken\n");
    } else if (response[9] == '2') {
        printf("Successful account registration!\n");
    } else {
        printf("Something went wrong!\n");
    }
    json_value_free(root_value);
    free(buffer);
    free(data);
    free(message);
}

void handle_login(int sockfd)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *data = NULL;
    char *buffer = calloc(LINELEN, sizeof(char));
    char *message;
    char *response;

    printf("username=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if (strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        return;
    }
    if (hasSpaces(buffer)) {
        printf("Invalid username. Exited\n");
        free(buffer);
        return;
    }
    json_object_set_string_with_len(root_object, "username", buffer, strlen(buffer));
    printf("password=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        return;
    }
    if (hasSpaces(buffer)) {
        printf("Invalid password. Exited\n");
        free(buffer);
        return;
    }
    json_object_set_string_with_len(root_object, "password", buffer, strlen(buffer));
    data = json_serialize_to_string_pretty(root_value);
    message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/login", "application/json", data, NULL, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // Verific prima cifra a codului de return din raspunsul serverului
    if (response[9] == '4') {
        printf("Invalid Credentials\n");
        return;
    }
    session_cookie = calloc(LINELEN, sizeof(char));
    // incadrez cookie-ul de sesiune si il pun in session_cookie
    strcpy(buffer, strstr(response, "Set-Cookie: ") + strlen("Set-Cookie: "));
    buffer = strtok(buffer, ";");
    strcpy(session_cookie, buffer);
    if (response[9] == '2') {
        printf("Successful login!\n");
    } else {
        printf("Something went wrong!\n");
    }
    json_value_free(root_value);
    free(buffer);
    free(data);
    free(message);
}

void handle_enter_library(int sockfd)
{
    char *message;
    char *response;
    char *buffer = calloc(LINELEN, sizeof(char));
    message = compute_get_request(SERVER_IP, "/api/v1/tema/library/access", NULL, session_cookie, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (response[9] == '4') {
        printf("Not logged in\n");
        free(message);
        return;
    } else if (response[9] == '2') {
        printf("Authorization token acquired!\n");
    } else {
        printf("Something went wrong\n");
        free(message);
        return;
    }
    response = strstr(response, "{\"token\"");
    strcpy(buffer, response);
    JSON_Value *token = json_parse_string(buffer);
    JWT_token = (char *) json_object_get_string(json_object(token), "token");
    free(message);
}

void handle_get_books(int sockfd)
{
    char *message;
    char *response;
    message = compute_get_request(SERVER_IP, "/api/v1/tema/library/books", NULL, session_cookie, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (response[9] == '4') {
        printf("Authorization header is missing!\n");
        free(message);
        return;
    }
    if (response[9] != '2') {
        printf("Something went wrong\n");
        free(message);
        return;
    }
    response = strstr(response, "[");
    printf("%s\n", response);
    free(message);
}

void handle_get_book(int sockfd)
{
    char *message;
    char *response;
    char *url = calloc(LINELEN, sizeof(char));
    char *buffer = calloc(LINELEN, sizeof(char));

    strcpy(url, "/api/v1/tema/library/books/");
    printf("id=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(url);
        free(buffer);
        return;
    }
    if (!isNumber(buffer)) {
        printf("Invalid ID. Exited\n");
        free(url);
        free(buffer);
        return;
    }
    strcat(url, buffer);
    message = compute_get_request(SERVER_IP, url, NULL, session_cookie, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // 404 Not Found
    if (response[9] == '4' && response[11] == '4') {
        printf("No book was found\n");
        free(url);
        free(buffer);
        free(message);
        return;
    }
    // 403 Unauthorized Access
    if (response[9] == '4' && response[11] == '3') {
        printf("Authorization header is missing!\n");
        free(url);
        free(buffer);
        free(message);
        return;
    }
    if (response[9] != '2') {
        printf("Something went wrong\n");
        free(url);
        free(buffer);
        free(message);
        return;
    }
    response = strstr(response, "{");
    printf("%s\n", response);
    free(url);
    free(buffer);
    free(message);
}

void handle_add_book(int sockfd)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *message;
    char *response;
    char *data = calloc(LINELEN, sizeof(char));
    char *buffer = calloc(LINELEN, sizeof(char));

    printf("title=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    json_object_set_string_with_len(root_object, "title", buffer, strlen(buffer));
    printf("author=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    json_object_set_string_with_len(root_object, "author", buffer, strlen(buffer));
    printf("genre=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    json_object_set_string_with_len(root_object, "genre", buffer, strlen(buffer));
    printf("page_count=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    if (!isNumber(buffer)) {
        printf("Invalid page_count. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    json_object_set_number(root_object, "page_count", atoi(buffer));
    printf("publisher=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(buffer);
        free(data);
        return;
    }
    json_object_set_string_with_len(root_object, "publisher", buffer, strlen(buffer));
    data = json_serialize_to_string_pretty(root_value);
    message = compute_post_request(SERVER_IP, "/api/v1/tema/library/books", "application/json", data, session_cookie, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (response[9] == '4') {
        printf("Authorization header is missing!\n");
    } else if (response[9] == '2') {
        printf("Book added successfully!\n");
    } else {
        printf("Something went wrong\n");
    }
    free(buffer);
    free(data);
    free(message);
    json_value_free(root_value);
}

void handle_delete_book(int sockfd)
{
    char *message;
    char *response;
    char *url = calloc(LINELEN, sizeof(char));
    char *buffer = calloc(LINELEN, sizeof(char));

    strcpy(url, "/api/v1/tema/library/books/");
    printf("id=");
    fgets(buffer, LINELEN, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if(strlen(buffer) == 0) {
        printf("Incomplete field. Exited\n");
        free(url);
        free(buffer);
        return;
    }
    if (!isNumber(buffer)) {
        printf("Invalid ID. Exited\n");
        free(url);
        free(buffer);
        return;
    }
    strcat(url, buffer);
    message = compute_delete_request(SERVER_IP, url, NULL, session_cookie, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // 404 Not Found
    if (response[9] == '4' && response[11] == '4') {
        printf("No book was found\n");
    // 403 unauthorized access
    } else if (response[9] == '4' && response[11] == '3') {
        printf("Authorization header is missing!\n");
    } else if (response[9] == '2') {
        printf("Book deleted successfully!\n");
    } else {
        printf("Something went wrong!\n");
    }
    free(url);
    free(buffer);
    free(message);
}

void handle_logout(int sockfd)
{
    char *message;
    char *response;
    message = compute_get_request(SERVER_IP, "/api/v1/tema/auth/logout", NULL, session_cookie, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (response[9] == '4') {
        printf("You are not logged in\n");
    } else if (response[9] == '2') {
        printf("Logged out successfully!\n");
    } else {
        printf("Something went wrong\n");
    }
    // pierd valorile pentru cookie-ul de sesiune si pentru token-ul de acces pentru a evita accesul nedorit dupa logout
    if (session_cookie != NULL) {
        free(session_cookie);
        session_cookie = NULL;
    }
    if (JWT_token != NULL) {
        free(JWT_token);
        JWT_token = NULL;
    }
    free(message);
}

int main(int argc, char *argv[])
{
    char *command = calloc(LINELEN, sizeof(char));
    int sockfd;

    while (1) {
        // apel blocant pentru citirea urmatoarei comenzi de la tastatura
        // deschid o conexiune TCP abia dupa ce primesc un request, astfel
        // evit un posibil timeout al serverului
        scanf("%s", command);
        // sa scap de newline-ul din buffer
        getchar();
        sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
        if (!strcmp(command, "register")) {
            handle_register(sockfd);
        } else if (!strcmp(command, "login")) {
            handle_login(sockfd);
        } else if (!strcmp(command, "enter_library")) {
            handle_enter_library(sockfd);
        } else if (!strcmp(command, "get_books")) {
            handle_get_books(sockfd);
        } else if (!strcmp(command, "get_book")) {
            handle_get_book(sockfd);
        } else if (!strcmp(command, "add_book")) {
            handle_add_book(sockfd);
        } else if (!strcmp(command, "delete_book")) {
            handle_delete_book(sockfd);
        } else if (!strcmp(command, "logout")) {
            handle_logout(sockfd);
        } else if (!strcmp(command, "exit")) {
            free(command);
            // inchide conexiunea TCP si iese din program
            close_connection(sockfd);
            break;
        } else {
            printf("Unknown command: %s\n", command);
        }
        
        close_connection(sockfd);
    }

    return 0;
}
