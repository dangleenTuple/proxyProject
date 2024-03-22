## MULTIPLE CLIENT REVERSE PROXY
I am learning how to make different types of **reverse proxies** :)

Followed along the part two of the same how-to as the single client reverse proxy: [Writing a reverse proxy](https://www.gilesthomas.com/2013/09/writing-a-reverse-proxyloadbalancer-from-the-ground-up-in-c-part-2-handling-multiple-connections-with-epoll)

In the follow-along, they used a python script for testing the project. I used C++ to write a test program (I did the same thing for the single-client connection proxy).
In my project, I do NOT handle HTTP requests. I only communicate directly over TCP sockets.

Simply run the bash script run_tests.sh to see how my multithreaded client program tests out the  multi-client connected proxy (which does NOT use multithreading to connect the clients to the server.
It uses epoll).
