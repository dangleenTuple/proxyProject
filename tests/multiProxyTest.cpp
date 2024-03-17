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
int startMultiProxy() {
   int pid, status;
   // first we fork the process
   if (pid = fork()) {
       // pid != 0: this is the parent process (i.e. the process of the test)
       waitpid(pid, &status, 0); // wait for the child to exit.
   } else {
       /* pid == 0: this is the child process. */
       const char executable[] = "./../src/multiProxy";
       // Run 'man 3 exec' to read about the below command
       // execl takes the arguments as parameters. execv takes them as an array
       // Below let's pass in the needed parameters: the path, what gets passed in as the main argv[0], then argv[1-3]...

       //   exec        argv[0], argv[1]=server_port_str, argv[2]=reverseProxy_addr, argv[3]=reverseProxy_port_str
       cout << "Inside child process of test, let's run our main process!" << endl;
       execl(executable, executable, "55005", "127.0.0.1", "56000", (char*)0); //NOTE: Try to use ports that are dynamic (49152 to 65535) since they are meant for client programs like this one

       /* exec does not return unless the program couldn't be started. 
          If we made it to this point, that means something went wrong.
          when the child process stops, the waitpid() above will return.
       */
       cout << "FAILED TO RUN COMMAND TO START MULTI PROXY PROCESS" << endl;


   }
   return status; // this should be the parent process again if there were no issues
}
//TODO: pass a message so that we can confirm our proxy can connect with multiple clients at a time.
void clientServer(string message){
    boost::asio::io_service io_service;
    using boost::asio::ip::tcp;
    boost::asio::io_context io_context;
    
    // we need a socket and a resolver
    tcp::socket client(io_context);
    tcp::resolver resolver(io_context);

    cout << "[Client] Waiting for connection" << endl;
    client.connect( tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 55005));
    cout << "[Client] Made a connection" << endl;

    //Let's not send anything until we make a connection to the backend
    unique_lock<decltype(m)> l(m);
    cv.wait(l);
    client.send(boost::asio::buffer(message));
    cout << "CLIENT SENT MESSAGE" << endl;
}

//TODO: probably have this run in a loop or until the main server closes? Use a mutex to do this?
void backendServer(){
    boost::asio::io_service service;
    using namespace boost::asio::ip;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 56000);
    tcp::acceptor acceptor(service, endpoint); 
    tcp::socket socket(service);

    cout << "[Proxy] Waiting for connection" << endl;
    acceptor.accept(socket);
    cout << "[Proxy] Accepted a connection" << endl;
    cv.notify_all();
    //We handle the shutdown/closing of the backend server in the backend_socket.c program
}


int main() {

    //In our test, we will make multiple client threads that will attempt to connect to the server (port 55005). When the clients ask to connect to the server
    //the server will add client instances to our epoll instance as events. Each client will connect to a backend (in our case, just our socket on port 56000)
    //and the backend will have an instance that will be added to our same epoll instance.
    //Then we are going to make our clients send some messages that we will print out from the backend to confirm our multiple clients are being connected.
    cout << "STARTING MAIN PROCESS" << endl;

    //Let's make a process for the multi proxy code where we will listen for client connections
    boost::thread s(&startMultiProxy);
    sleep(5); //For some reason, the "bind()" takes a long time to fully execute. As a temp solution, let's make the other threads wait a bit.

    //To keep things simple, let's just use one backend server to test this.
    boost::thread b(&backendServer);
    boost::thread c1(&clientServer, "Hi from first client!");
    //boost::thread c2(&clientServer, "Hi from second client!");

    c1.join();
    //c2.join();
    b.join();
    s.detach(); //The other threads will end on their own once we send a message. Let's stop the main server thread, otherwise it will hang.*/
    return 0;
}