## SINGLE CLIENT AND MULTITHREADED CLIENT REVERSE PROXY
I am learning how to make different types of **reverse proxies** :)

Followed along this article as inspiration: [Writing a reverse proxy](https://www.gilesthomas.com/2013/08/writing-a-reverse-proxyloadbalancer-from-the-ground-up-in-c-part-1)

The above link uses a good example of wildcard in a make file. More info on that here: [wildcard](https://www.gnu.org/software/make/manual/html_node/Wildcard-Function.html)

SINGLEPROXY NOTES:

As a part of this learning experience, I wanted to run the server, client, and reverse proxy on respective threads. You can run all of the intended transmissions by running "bash run_tests.sh" this way.
The client should attempt to send a message to the server but immediately gets relayed to the proxy. The proxy then handles the message. Finally, the server will send a closing message to the client
because why not?
