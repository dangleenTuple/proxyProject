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
using namespace boost::asio::ip;

atomic<int> counter = 0; //This counter (a semaphore) is atomic - meaning that the counter will either be finished or unstarted (remember, to change the value of this counter, we must load, add then store its value. Three steps.). 
    //There is no "in between" state. This is achieved by mutual exclusion.
    //Atomic operations in C++ are performed with std::memory_order_seq_cst, which means it guarantees sequential consistency (total global ordedring).
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
//Pass a message so that we can confirm our proxy can connect with multiple clients at a time.
void clientServer(string message, int clientNum){
    sleep(0); //Same reason as the other sleep(0), we want to have our print statements actually make sense when we start all of our client threads.
    boost::asio::io_service io_service;
    using boost::asio::ip::tcp;
    boost::asio::io_context io_context;
    
    // we need a socket and a resolver
    tcp::socket client(io_context);
    tcp::resolver resolver(io_context);

    client.connect( tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 55005));
    cout << "[Client " << clientNum << "] Made a connection" << endl;

    client.send(boost::asio::buffer(message));
    cout << "[Client " << clientNum << "] Sent a message" << endl;
}

void backendReceive(tcp::socket* backend)
{
    boost::array<char, 21> buf;
    backend->receive(boost::asio::buffer(buf)); 
    sleep(0); //Adding this here in case if client thread print statements are faster - otherwise, sometimes the print statements happen out of order.
    //NOTE: Sleep(0) seems like it does nothing, but lowers this thread's priority by letting other threads that need to run to go first.
    cout << "[Backend] Received this message: " << endl;
    for(char c : buf)
        cout << c;
    cout << endl;
    counter--;
}

void backendServer(){
    boost::asio::io_context io_context;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 56000);
    tcp::acceptor acceptor(io_context, endpoint); 
    acceptor.listen();
    while(true)
    {
        tcp::socket backend(io_context);
        acceptor.accept(backend);
        counter++;
        cout << "[Backend] Accepted a connection" << endl;
        boost::thread s(&backendReceive, &backend);
        s.join(); //We want to make sure our print statements actually finish to see if this is all successful. So let's ensure it by joining every backend receive thread.
    }
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
    boost::thread c1(&clientServer, "Hi from first client!", 1);
    boost::thread c2(&clientServer, "Hi from second client!", 2);
    boost::thread c3(&clientServer, "Hi from third client!", 3);
    c1.join(); //NOTE: the order of these joins don't really matter.
    c2.join();
    c3.join();
    sleep(5); //Let's give some time for these clients to actually connect to a backend before we actually have to lock our main thread.

    unique_lock<decltype(m)> l(m); //We are using this condition variable like a semaphore. The semaphore will count all of the threads that are running being processed, and when they finish they will subtract themselves from the semaphore
    //When the semaphore reaches 0, we detach our backend thread. Otherwise it will run forever.
    cv.wait(l, []{ return counter == 0; });
    b.detach();
    return 0;
    //NOTE: I tried my best to get the print statements to make sense when you run the bash script in this project. However, it still might be a little wacky. The global mutex, semaphore and utilizing the c++ sleep/join functions help a ton!
    //but it's not perfect xD
    //If you run my bash script over and over again, everything always comes out in a different order. This is a nice little exercise for learning about multithreading! :)
}
