#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <json-c/json.h>

#define PORT 18081
#define BUFFER_SIZE 1024

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(client_socket);
        return;
    }

    // Null-terminate the received data
    buffer[bytes_received] = '\0';

    // Print the received request (for debugging)
    printf("Received request:\n%s\n", buffer);

    // Check if the request is a POST to /startup_sidecar
    if (strstr(buffer, "POST /startup_sidecar") != NULL) {
        // Find the start of the JSON body
        char *json_body = strstr(buffer, "\r\n\r\n");
        if (json_body != NULL) {
            json_body += 4; // Move past the headers

            // Parse the JSON body
            struct json_object *parsed_json;
            struct json_object *service_config;
            struct json_object *control_port;
            struct json_object *ntp_address;
            struct json_object *redis_address;
            struct json_object *redis_port;
            parsed_json = json_tokener_parse(json_body);
            if (parsed_json == NULL) {
                fprintf(stderr, "Failed to parse JSON\n");
                const char *http_response = "HTTP/1.1 400 Bad Request\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "Connection: close\r\n"
                                            "\r\n"
                                            "Invalid JSON format\n";
                send(client_socket, http_response, strlen(http_response), 0);
                close(client_socket);
                return;
            }

            // Extract the required fields
            json_object_object_get_ex(parsed_json, "service_config", &service_config);
            json_object_object_get_ex(parsed_json, "control_port", &control_port);
            json_object_object_get_ex(parsed_json, "ntp_address", &ntp_address);
            json_object_object_get_ex(parsed_json, "redis_address", &redis_address);
            json_object_object_get_ex(parsed_json, "redis_port", &redis_port);
            if (service_config == NULL || control_port == NULL || ntp_address == NULL ||
                redis_address == NULL || redis_port == NULL) {
                fprintf(stderr, "Missing required fields\n");
                const char *http_response = "HTTP/1.1 400 Bad Request\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "Connection: close\r\n"
                                            "\r\n"
                                            "Missing required fields\n";
                send(client_socket, http_response, strlen(http_response), 0);
                close(client_socket);
                return;
            }

            // Generate the command
            const char *service_config_str = json_object_get_string(service_config);
            int control_port_int = json_object_get_int(control_port);
            const char *ntp_address_str = json_object_get_string(ntp_address);
            const char *redis_address_str = json_object_get_string(redis_address);
            int redis_port_int = json_object_get_int(redis_port);
            char command[BUFFER_SIZE];
            
            snprintf(command, sizeof(command), "sudo bash /home/hpr/consul/sidecar_launcher/startup_sidecar.sh \"%s\" %d \"%s\" \"%s\" %d", 
                     service_config_str, control_port_int, ntp_address_str, redis_address_str, redis_port_int);

            // Change directory to where the program is located
            if (chdir("/home/hpr/consul/sidecar") != 0) {
                perror("chdir failed");
            }

            // Start another C program (e.g., "sidecar") without blocking
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                execlp("bash", "bash", "-c", command, NULL);
                // If exec fails
                perror("exec failed");
                exit(1);
            } else if (pid < 0) {
                perror("fork failed");
            }

            // Clean up JSON object
            json_object_put(parsed_json);

            // Respond with a success message
            const char *http_response = "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Connection: close\r\n"
                                        "\r\n"
                                        "Command executed successfully\n";
            send(client_socket, http_response, strlen(http_response), 0);
            close(client_socket);
        }
    } else {
        const char *http_response = "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Type: text/plain\r\n"
                                    "Connection: close\r\n"
                                    "\r\n"
                                    "Not Found\n";
        send(client_socket, http_response, strlen(http_response), 0);
        close(client_socket);
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    // Bind the socket to the port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(1);
    }

    // Listen for incoming connections
    listen(server_socket, 5);
    printf("HTTP server is running on port %d...\n", PORT);

    while (1) {
        // Accept a client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        // Handle the client request
        handle_request(client_socket);
    }

    close(server_socket);
    return 0;
}

