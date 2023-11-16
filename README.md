# TCP Client v1

This program is a simple TCP client that sends data to a server to be transformed. 
The client is a command-line tool that follows standard Unix norms.

After `make`-ing the program, in a terminal, a request can be sent in the following format: `./tcp_client ACTION LENGTH MESSAGE`.

The ACTION can be one of the following 5 options:
* Uppercase
* Lowercase
* Reverse
* Shuffle
* Random

The LENGTH is the length of the message. The message is the text to be transformed.

For example, if you want “Hello World” to be reversed, you would send
`reverse 11 Hello World`
The response from the server would be
`dlroW olleH`


This program demonstrates the knowledge of parsing command-line arguments using `getopt_long` in C, familiarity with building a good command-line too, and using the the socket API.
