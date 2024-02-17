#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/thread.hpp>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/array.hpp>

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
using boost::asio::ip::tcp;
using boost::asio::ip::address;
using boost::asio::io_service;
using boost::asio::buffer;

//Let's write a function that will start a process that runs our C program
int startMainServer() {
   int pid, status;
   // first we fork the process
   if (pid = fork()) {
       // pid != 0: this is the parent process (i.e. the process of the test)
       waitpid(pid, &status, 0); // wait for the child to exit.
   } else {
       /* pid == 0: this is the child process. */
       const char executable[] = "./../src/singleProxy";
       // Run 'man 3 exec' to read about the below command
       // execl takes the arguments as parameters. execv takes them as an array
       // Below let's pass in the needed parameters: the path, what gets passed in as the main argv[0], then argv[1-3]...

       //   exec        argv[0], argv[1]=server_port_str, argv[2]=reverseProxy_addr, argv[3]=reverseProxy_port_str
       cout << "Inside child process of test, let's run our main server!" << endl;
       execl(executable, executable, "55005", "127.0.0.1", "56000", (char*)0);

       /* exec does not return unless the program couldn't be started. 
          If we made it to this point, that means something went wrong.
          when the child process stops, the waitpid() above will return.
       */
       cout << "FAILED TO RUN COMMAND TO START SERVER" << endl;


   }
   return status; // this should be the parent process again if there were no issues
}

void clientServer(){
    boost::asio::io_service io_service;
    using boost::asio::ip::tcp;
    boost::asio::io_context io_context;
    
    // we need a socket and a resolver
    tcp::socket client(io_context);
    tcp::resolver resolver(io_context);
    cout << "[Client] Waiting for connection" << endl;
    //while (true)
    //{
        // now we can use connect(..)
    client.connect( tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 55005));
    cout << "[Client Server] Made a connection" << endl;
    string const& message = "Hi from the Client!";
    client.send(boost::asio::buffer(message));

    boost::asio::streambuf sb;
    boost::system::error_code ec;
    while (boost::asio::read(client, sb, ec)) //This is not printing? Idk why :(
        std::cout  << &sb << "\n";

    cout << "CLOSING DOWN CLIENT" << endl;
    client.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    client.close();
}


void proxyServer(){
    boost::asio::io_service service;
    using namespace boost::asio::ip;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 56000);
    tcp::acceptor acceptor(service, endpoint); 
    tcp::socket socket(service);
    cout << "[Proxy Server] Waiting for connection" << endl;
    acceptor.accept(socket);
    cout << "[Proxy Server] Accepted a connection" << endl;
    service.run(); //This should stop this thread after finishing up all of the services for io_service
}


int main() {
    cout << "STARTING MAIN SERVER" << endl;
    //cout << "STATUS: " << startMainServer() << endl;
    boost::thread s(&startMainServer);
    sleep(5); //For some reason, the "bind()" takes a long time to fully execute. As a temp solution, let's make the other threads wait a bit.
    boost::thread p(&proxyServer);
    boost::thread c(&clientServer);
    c.join();
    p.join();
    s.detach(); //The other threads will end on its own once we send a message. Let's gracefully shut this one down.
    return 0;
}