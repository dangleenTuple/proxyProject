#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/thread.hpp>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

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
       cout << "Inside child process of test, let's run our main server: " << endl;
       execl(executable, executable, "8000", "127.0.0.1", "8888", (char*)0);

       /* exec does not return unless the program couldn't be started. 
          If we made it to this point, that means something went wrong.
          when the child process stops, the waitpid() above will return.
       */
       cout << "FAILED TO RUN COMMAND TO START SERVER" << endl;


   }
   return status; // this should be the parent process again if there were no issues
}

/*void handleRequest(http::request<http::string_body>& request, tcp::socket& socket) {
    // Prepare the response message
    http::response<http::string_body> response;
    response.version(request.version());
    response.result(http::status::ok);
    response.set(http::field::server, "My HTTP Server");
    response.set(http::field::content_type, "text/plain");
    response.body() = "Hello, World!";
    response.prepare_payload();

    // Send the response to the client
    boost::beast::http::write(socket, response);
}

void clientServer() {
    boost::asio::io_context io_context;
    //This is basically a "connection" socket. We need this for the client.
    tcp::acceptor acceptor(io_context, {tcp::v4(), 51000});

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        // Read the HTTP request
        boost::beast::flat_buffer buffer;
        http::request<http::string_body> request;
        boost::beast::http::read(socket, buffer, request);

        // Handle the request
        handleRequest(request, socket);

        // Close the socket
        socket.shutdown(tcp::socket::shutdown_send);
    }
}*/

void clientServer(){
    boost::asio::io_service io_service;
    using boost::asio::ip::tcp;
    boost::asio::io_context io_context;
    
    // we need a socket and a resolver
    tcp::socket client(io_context);
    tcp::resolver resolver(io_context);

    //while (true)
    //{
        // now we can use connect(..)
        boost::asio::connect(client, resolver.resolve( tcp::resolver::query("localhost","8000",tcp::resolver::query::canonical_name)));
       /*const int BUFLEN = 1024;
        vector<char> buf(BUFLEN);
        client.send(buffer(HTTP_REQUEST, HTTP_REQUEST.size()));
        error_code error;
        int len = client.receive(buffer(buf, BUFLEN));
        cout << "main(): buf.data()=";
        cout.write(buf.data(), len);
        cout << endl;*/

       // boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    //}

     // and use write(..) to send some data which is here just a string
   /* std::string data{"some client data ..."};
    auto result = boost::asio::write(socket, boost::asio::buffer(data));
    
    // the result represents the size of the sent data
    std::cout << "data sent: " << data.length() << '/' << result << std::endl;

    // and close the connection now
    boost::system::error_code ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket.close();*/
}

void SendHandler(boost::system::error_code ex){
    cout << "MADE IT TO HANDLER" << endl;
}

void proxyServer(){
    boost::asio::io_service service;
    using namespace boost::asio::ip;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8888);
    tcp::acceptor acceptor(service, endpoint); 
    tcp::socket socket(service);
    cout << "[Server] Waiting for connection" << endl;

    acceptor.accept(socket);
    cout << "[Server] Accepted a connection from client" << endl;

    std::string msg = "You made it to the proxy server!";
    socket.async_send(boost::asio::buffer(msg), SendHandler);
    service.run();
}


int main() {

    cout << "STARTING MAIN SERVER" << endl;
    //cout << "STATUS: " << startMainServer() << endl;
    boost::thread s(&startMainServer);
    boost::thread c(&clientServer);
    boost::thread p(&proxyServer);
    c.join();
    p.join();
    s.join();

    return 0;
}


/*
class MockServerRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write("Hello from the mock server\n")



class TestCanProxyHTTPRequestToBackend(unittest.TestCase):

    def test_simple_proxying(self):
        # We start up a backend
        httpd = HTTPServer(('127.0.0.1', 8888), MockServerRequestHandler)
        server_thread = Thread(target=httpd.serve_forever)
        server_thread.daemon = True
        server_thread.start()

        # ...and an rsp option that proxies everything to it.
        server = pexpect.spawn(RSP_BINARY, ["8000", "127.0.0.1", "8888"])
        server.expect("Started.  Listening on port 8000.")
        try:
            # Make a request and check it works.
            response = requests.get("http://127.0.0.1:8000")
            self.assertEqual(response.status_code, 200)
            self.assertEqual(response.text, "Hello from the mock server\n")

            # Make a second request and check it works too.
            response = requests.get("http://127.0.0.1:8000")
            self.assertEqual(response.status_code, 200)
            self.assertEqual(response.text, "Hello from the mock server\n")
        finally:
            server.kill(9)

            httpd.shutdown()
*/