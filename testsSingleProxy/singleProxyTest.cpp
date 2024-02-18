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
#include <condition_variable>

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
using boost::asio::ip::tcp;
using boost::asio::ip::address;
using boost::asio::io_service;
using boost::asio::buffer;

condition_variable cv;
mutex m;

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
       execl(executable, executable, "55005", "127.0.0.1", "56000", (char*)0); //NOTE: Try to use ports that are dynamic (49152 to 65535) since they are meant for client programs like this one

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
    client.connect( tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 55005));
    cout << "[Client] Made a connection" << endl;

    string message = "Hi from the Client!";
    client.send(boost::asio::buffer(message));
    cout << "CLIENT SENT MESSAGE" << endl;

    //Utilizing a lock here to make the print statements make sense. Otherwise, it will read out of order since the send/recvs might happen at different times for the proxy and client.
    unique_lock<decltype(m)> l(m);
    cv.wait(l);
    cout << "READING RESULTING BUFFER IN CLIENT: " << endl;
    //NOTE: Keep in mind, the singleProxy.c code is designed to take in only one message. 
    //Because it only will relay to the proxy once from the main server.
    boost::array<char, 43> buf;
    client.receive(boost::asio::buffer(buf));
    for(char c : buf)
        cout << c;
    cout << endl;
    boost::system::error_code ec;
    cout << "CLOSING DOWN CLIENT\n\n" << endl;
    client.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    client.close();
}


void proxyServer(){
    boost::asio::io_service service;
    using namespace boost::asio::ip;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 56000);
    tcp::acceptor acceptor(service, endpoint); 
    tcp::socket socket(service);
    cout << "[Proxy] Waiting for connection" << endl;
    acceptor.accept(socket);
    cout << "[Proxy] Accepted a connection" << endl;

    boost::array<char, 21> buf;
    boost::system::error_code ec;
    cout << "READING RESULTING BUFFER IN PROXY: " << endl;
    socket.receive(boost::asio::buffer(buf));
    for(char c : buf)
        cout << c;
    cout << endl;
    cv.notify_all();
    //unique_lock<decltype(m)> l(m);
    //cv.wait(l);
    cout << "CLOSING DOWN PROXY\n\n" << endl;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket.close();
}


int main() {
    cout << "STARTING MAIN SERVER" << endl;
    //cout << "STATUS: " << startMainServer() << endl;
    boost::thread s(&startMainServer);
    sleep(5); //For some reason, the "bind()" takes a long time to fully execute. As a temp solution, let's make the other threads wait a bit.
    boost::thread p(&proxyServer);
    boost::thread c(&clientServer);
    p.join();
    c.join(); //We have to join the client and proxy (it doesn't matter the order) first to make sure that these are completely done with their transmissions before we attempt to shut down the main thread.
    s.detach(); //The other threads will end on their own once we send a message. Let's stop the main server thread, otherwise it will hang.
    return 0;
}