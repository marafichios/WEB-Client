#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.c"
#include <ctype.h>

#define HOST "34.246.184.49"
#define PORT 8080

#define PAYLOAD "application/json"

#define REGISTER_ACC_ROUTE "/api/v1/tema/auth/register"
#define LOGIN_ACC_ROUTE "/api/v1/tema/auth/login"
#define LIBRARY_ACC_ROUTE "/api/v1/tema/library/access"
#define BOOKS_ACC_ROUTE "/api/v1/tema/library/books/"
#define LOGOUT_ACC_ROUTE "/api/v1/tema/auth/logout"

int logged_in = 0;
int library_access = 0;	
char  *cookie = NULL;
char *token = NULL;

int check_valid_credentials(char *username, char *password) {
	//check if username has characters any other that alphanumeric also it cannot be empty or if they are 
	
	if (strlen(username) == 0 || strlen(password) == 0) {
		printf("ERROR: Username and password should not be empty!\n");
		return 0;
	}

	if (strchr(username, ' ') != NULL || strchr(password, ' ') != NULL) {
		printf("ERROR: Username and password should not contain spaces!\n");
		return 0;
	}
	
	for (int i = 0; i < strlen(username); i++) {
		if (!isalnum(username[i])) {
			printf("ERROR: Username should contain only alphanumeric characters!\n");
			return 0;
		}
	}
	return 1;
}

char *JSON_obj_instantiation(char *username, char *password) {
	
	// create JSON object with username and password
	char *serialized_string;
	JSON_Value *value = json_value_init_object();
	JSON_Object *object = json_value_get_object(value);

	json_object_set_string(object, "username", username);
	json_object_set_string(object, "password", password);

	// serialize JSON object
	serialized_string = json_serialize_to_string_pretty(value);
	json_value_free(value);
	return serialized_string;
}

int verify_valid_book_input(char *title, char *author, char *genre, char *publisher, char *page_count) {
	
	if (strlen(title) == 0 || strlen(author) == 0 || strlen(genre) == 0 || strlen(publisher) == 0 || strlen(page_count) == 0) {
		printf("ERROR: All fields should be filled!\n");
		return 0;
	}

	// check if page count contains letters
	for (int i = 0; i < strlen(page_count); i++) {
		if (page_count[i] < '0' || page_count[i] > '9') {
			printf("ERROR: Page count should be a positive number!\n");
			return 0;
		}
	}

	return 1;
}

int register_cmd(int sockfd) {
	char username[LINELEN] = {0};
	char password[LINELEN] = {0};
	char *JSON_string, *post_request, *response;
	
	// user must be logged out to register
	if (logged_in == 1) {
		printf("ERROR: Log out before you register!\n");
		return 0;
	}
	// read username and password
	printf("username=");
	fgets(username, LINELEN, stdin);
	username[strcspn(username, "\n")] = '\0';

	printf("password=");
	fgets(password, LINELEN, stdin);
	password[strcspn(password, "\n")] = '\0';

	if (check_valid_credentials(username, password) == 0) {
		return 0;
	}

	// instantiate JSON object
	JSON_string = JSON_obj_instantiation(username, password);

	// create POST request
	post_request = compute_post_request(HOST, REGISTER_ACC_ROUTE, PAYLOAD, &JSON_string, 1, NULL, 0, 0);

	// send POST request to server
	send_to_server(sockfd, post_request);

	// receive response from server
	response = receive_from_server(sockfd);

	// check if registration was successful by analyzing the response
	if (strstr(response, "HTTP/1.1 4") != NULL || strstr(response, "\"error\"") != NULL) {
        printf("ERROR: Registration failed! Username is used by someone else.\n");
    } else {
        printf("SUCCESS: Registration done!\n");
    }

	free(response);
	free(post_request);
	free(JSON_string);
	return 0;

}

char *login_cmd (int sockfd) {
	char username[LINELEN];
	char password[LINELEN];
	char *JSON_string, *post_request, *response;
	
	// user must be logged out to login
	if (logged_in == 1) {
		printf("ERROR: Log out before logging in with another username!\n");
		return 0;
	}

	// read username and password
	printf("username=");
	fgets(username, LINELEN, stdin);
	username[strcspn(username, "\n")] = '\0';

	printf("password=");
	fgets(password, LINELEN, stdin);
	password[strcspn(password, "\n")] = '\0';


	// instantiate JSON object
	JSON_string = JSON_obj_instantiation(username, password);

	// create POST request
	post_request = compute_post_request(HOST, LOGIN_ACC_ROUTE, PAYLOAD, &JSON_string, 1, NULL, 0, 0);

	// send POST request to server
	send_to_server(sockfd, post_request);

	response = receive_from_server(sockfd);
	
	if (strstr(response, "HTTP/1.1 4") == NULL) {
    	printf("SUCCES: Login done!\n");
		logged_in = 1;

		// extract cookie in order to keep the session
    	char *start = strstr(response, "Set-Cookie: ");
    	if (start) {
			// move the start pointer to the beginning of the cookie value
    	    start += strlen("Set-Cookie: ");
			// find the end of the cookie value
    	    char *end = strchr(start, ';');
			if (end)
    	        *end = '\0';
			cookie = strdup(start); // save the cookie
    	}
	} else {
    	printf("ERROR: Invalid credentials!\n");
	}

	free(post_request);
	free(response);
	free(JSON_string);
	return cookie;
}

int logout_cmd(int sockfd) {
	char *response, *get_request;

	if (logged_in == 0) {
		printf("ERROR: You are not logged in!\n");
		return 0;
	}

	// create GET request
	get_request = compute_get_request(HOST, LOGOUT_ACC_ROUTE, NULL, &cookie, 1, 0);

	// send GET request to server
	send_to_server(sockfd, get_request);

	response = receive_from_server(sockfd);

	// check if logout was successful
	if(strstr(response, "HTTP/1.1 200 OK") != NULL) {
		printf("SUCCESS: Logout done!\n");
		logged_in = 0;
		library_access = 0;
	} else {
		printf("ERROR: Logout failed!\n");
	}

	free(cookie);
	free(get_request);
	free(response);

	return 0;
}

char *enter_library_cmd(int sockfd) {
    char *response, *get_request;

    if (!logged_in) {
        printf("ERROR: You are not logged in!\n");
        return 0;
    }

	// create GET request
    get_request = compute_get_request(HOST, LIBRARY_ACC_ROUTE, NULL, &cookie, 1, NULL);
    
	// send GET request to server
	send_to_server(sockfd, get_request);

    response = receive_from_server(sockfd);

    // Parse JSON response to extract the token
    JSON_Value *root_value = json_parse_string(basic_extract_json_response(response));
    if (json_value_get_type(root_value) != JSONObject) {
        printf("ERROR: Unable to parse response as JSON\n");
    } else {

		// extract token from JSON response
        JSON_Object *root_object = json_value_get_object(root_value);
        const char *extracted_token = json_object_get_string(root_object, "token");
        if (extracted_token) {
            token = strdup(extracted_token);
            printf("SUCCESS: You get access to the library!\n");
            library_access = 1;
        } else {
            printf("ERROR: Token not found!\n");
            library_access = 0;
        }
    }

    json_value_free(root_value);

    free(get_request);
    free(response);

    return token;
}

int add_book_cmd(int sockfd) {
	char title[LINELEN], author[LINELEN], genre[LINELEN], publisher[LINELEN], page_count[LINELEN];
	char *JSON_string, *post_request, *response;

	if (!logged_in) {
		printf("ERROR: You are not logged in!\n");
		return 0;
	}

	if (!library_access) {
		printf("ERROR: You do not have access to the library!\n");
		return 0;
	}

	// read book details
	printf("title=");
	fgets(title, LINELEN, stdin);
	title[strcspn(title, "\n")] = '\0';

	printf("author=");
	fgets(author, LINELEN, stdin);
	author[strcspn(author, "\n")] = '\0';

	printf("genre=");
	fgets(genre, LINELEN, stdin);
	genre[strcspn(genre, "\n")] = '\0';

	printf("publisher=");
	fgets(publisher, LINELEN, stdin);
	publisher[strcspn(publisher, "\n")] = '\0';

	printf("page_count=");
	fgets(page_count, LINELEN, stdin);
	page_count[strcspn(page_count, "\n")] = '\0';

	if (verify_valid_book_input(title, author, genre, publisher, page_count) == 0)
		return 0;

	// create JSON object with book details
	JSON_Value *value = json_value_init_object();
	JSON_Object *object = json_value_get_object(value);

	json_object_set_string(object, "title", title);
	json_object_set_string(object, "author", author);
	json_object_set_string(object, "genre", genre);
	json_object_set_string(object, "publisher", publisher);
	json_object_set_number(object, "page_count", atoi(page_count));

	// serialize JSON object
	JSON_string = json_serialize_to_string_pretty(value);

	// create POST request
	post_request = compute_post_request(HOST, BOOKS_ACC_ROUTE, PAYLOAD, &JSON_string, 1, &cookie, 1, token);

	// send POST request to server
	send_to_server(sockfd, post_request);

	response = receive_from_server(sockfd);

	// check response
	if (strstr(response, "HTTP/1.1 4") != NULL) {
		printf("ERROR: Adding book failed!\n");
	} else {
		printf("SUCCESS: Book added!\n");
	}

	json_free_serialized_string(JSON_string);
	json_value_free(value);

	free(post_request);
	free(response);

	return 0;
}

int get_books_cmd(int sockfd) {
    char *response, *get_request;

	// check if user is logged in and has access to the library
    if (!logged_in) {
        printf("ERROR: You are not logged in!\n");
        return -1;
    }

    if (!library_access) {
        printf("ERROR: You do not have access to the library!\n");
        return -1;
    }

	// create GET request
    get_request = compute_get_request(HOST, BOOKS_ACC_ROUTE, NULL, &cookie, 1, token);
    send_to_server(sockfd, get_request);

    response = receive_from_server(sockfd);
	char *final_response = strstr(response, "[{");

	// parse JSON response
    JSON_Value *root_value = json_parse_string(final_response);
	JSON_Array *root_array = json_value_get_array(root_value);
	size_t count = json_array_get_count(root_array);

    if (count == 0) {
        printf("[]\n");
    } else {
		// print books
        for (size_t i = 0; i < count; i++) {
            JSON_Object *book = json_array_get_object(root_array, i);
            int id = (int) json_object_get_number(book, "id");
            const char *title = json_object_get_string(book, "title");
            printf("id: %d\ntitle: %s\n", id, title);
        }
        
    }
	json_value_free(root_value);
    free(get_request);
    free(response);
    return 0;

	
}

int get_book_cmd(int sockfd) {
	char id[LINELEN], *response, *get_request, route[256] = {0};

	// check if user is logged in and has access to the library
	if (!logged_in) {
		printf("ERROR: You are not logged in!\n");
		return -1;
	}

	if (!library_access) {
		printf("ERROR: You do not have access to the library!\n");
		return -1;
	}

	// read book id
	printf("id=");
	fgets(id, LINELEN, stdin);
	id[strcspn(id, "\n")] = '\0';

	for (int i = 0; id[i] != '\0'; i++) {
    	if (!isdigit(id[i])) {
    	    printf("ERROR: ID should be a positive number!\n");
    	    return 0;
    	}
	}

	// create route
	strcat(route, "/api/v1/tema/library/books/");
	strcat(route, id);

	// create GET request
	get_request = compute_get_request(HOST, route, NULL, &cookie, 1, token);
	send_to_server(sockfd, get_request);

	response = receive_from_server(sockfd);

	// extract book details
	char *final_response = strstr(response, "{");

	// check server response and print the book details
	if (strstr(response, "error") != NULL) {
		printf("ERROR: There is no book with the required ID!\n");
    } else {
		printf("%s\n", final_response);
	}

	free(get_request);
	free(response);

	return 0;
}

int delete_book_cmd(int sockfd) {
	char id[LINELEN], *response, *delete_request, route[256] = {0};

	// check if user is logged in and has access to the library
	if (!logged_in) {
		printf("ERROR: You are not logged in!\n");
		return -1;
	}

	if (!library_access) {
		printf("ERROR: You do not have access to the library!\n");
		return -1;
	}

	// read book id
	printf("id=");
	fgets(id, LINELEN, stdin);
	id[strcspn(id, "\n")] = '\0';

	for (int i = 0; id[i] != '\0'; i++) {
		if (!isdigit(id[i])) {
		    printf("ERROR: ID should be a positive number!\n");
		    return 0;
		}
	}

	// create route
	strcat(route, "/api/v1/tema/library/books/");
	strcat(route, id);

	// create DELETE request
	delete_request = compute_delete_request(HOST, route, NULL, &cookie, 1, token);
	send_to_server(sockfd, delete_request);

	// receive response from server
	response = receive_from_server(sockfd);
	
	// check response
	if (strstr(response, "HTTP/1.1 2") != NULL) {
		printf("SUCCESS: Book deleted!\n");
	} else {
		printf("ERROR: Deleting book failed, wrong ID!\n");
	}

	free(delete_request);
	free(response);

	return 0;
}


int main() {

	int sockfd;
	char command[20];

	while (1) {
		// open connection with server fro each command
		sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
		fgets(command, LINELEN, stdin);

		if (strcmp(command, "register\n") == 0) 
			register_cmd(sockfd);
		else if (strcmp(command, "login\n") == 0)
			login_cmd(sockfd);
		else if (strcmp(command, "logout\n") == 0)
			logout_cmd(sockfd);
		else if (strcmp(command, "enter_library\n") == 0)
			enter_library_cmd(sockfd);
		else if (strcmp(command, "add_book\n") == 0)
			add_book_cmd(sockfd);
		else if (strcmp(command, "delete_book\n") == 0)
			delete_book_cmd(sockfd);
		else if (strcmp(command, "get_books\n") == 0)
			get_books_cmd(sockfd);
		else if(strcmp(command, "get_book\n") == 0)
			get_book_cmd(sockfd);
		else if (strcmp(command, "exit\n") == 0)
			break;
		else
			printf("Invalid command\n");
			
		close_connection(sockfd);
	}

	// close connection with server
	close_connection(sockfd);
	return 0;
}