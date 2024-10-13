# Homework 4 PCOM

## Mara Fichios 321CA

* For this homeowrk I used 2 sleepdays

* For this homework I used the skel that was given to us at the lab, meaning,
the buffer, helpers and requests files. I solved TODOs in the requests.c file
during the 9th laboratory. Also, I used the parson library that was given to us
in the homework description.

* This homework was really interesting, as I had to implement a client that
gets basic commands regarding an online library, while it communicates with a
REST API, using the HTTP protocol. I chose the given parson library as it was
quite intuitive to use, making all the work for this project much easier.


* The commands I have implemented are the following:
    
    - `register` - registers a new user by reading the credentials from stdin
    with fgets. The input data is used to initialize a JSON object, which is
    then sent to the server, but not before computing a POST request. The
    server will respond with a status code and a message.
        - error messages appear when: someone is already logged in, the user
        is already registered, the password is too short, the username is too
        long, the password and username are empty, they contain spaces, or if
        username doesnt have alphanumeric characters.
    
    - `login` - same as before, but this time the user is set as logged in,
    and the session is kept alive by storing the cookie received from the
    response of the server. The cookie is sent back to the server in the next
    requests.
        - error messages appear when: someone is already logged in, credentials
        are wrong.

    - `logout` - sends a GET request to the server, which will respond with a
    status code and a message. Based on that code, there is checked wether the
    user logged out or not. The logged in and lib access flags are set to 0.
    The cookie is used for the get request and then freed.

    - `enter_library` - sends a GET request to the server, which will send back
    a response. The message contains the JWT token, which is stored in the
    response. It is extracted, and if not NULL, the user has library access.
    Used the cookie.
        - error messages appear when: someone is not logged in, the JWT token
        is NULL, thr JSON object could not be parsed.

    - `get_books` - sends a GET request to the server, which will respond with
    a status code and a message. The message contains the books in the library,
    which are printed to stdout. Used the cookie and token.
        - error messages appear when: someone is not logged in, the user is not
        in the library, the JSON object could not be parsed.

    - `get_book` - sends a GET request to the server, which will respond with a
    status code and a message. The message contains the book with the given id,
    which is printed to stdout, after the user inputs the required id, and the
    route is formed. I tried to get the final response as the command before,
    by using the json functions, but I have encountered an error and I found it
    easier to simply extract the book data by searching it in the server
    response. Used the cookie and token.
        - error messages appear when: someone is not logged in, the user is not
        in the library, ID is not a positive number, there is no book with the
        input ID

    - `add_book` - sends a POST request to the server, which will respond with
    a status code and a message. Depending on the server response, there is
    checked whether the book was added or not. The user inputs the title,
    author, genre, publisher and page count of the book, which are used to
    initialize a JSON object. Used the cookie and token.
        - error messages appear when: someone is not logged in, the user is
        not in the library, the book fields are empty, page count is not a
        positive number.

    - `delete_book` - sends a DELETE request to the server, which will respond
    with a status code and a message. Depending on the server response, there
    is checked whether the book was deleted or not. The user inputs the id of
    the book to be deleted, which is used to form the route. Used the cookie
    and token.
        - error messages appear when: someone is not logged in, the user is
        not in the library, ID is not a positive number, there is no book with
        the input ID.

    - `exit` - exits the program and closes the connection with the server.
