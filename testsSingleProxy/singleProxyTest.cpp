#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

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
       //      exec         argv[0]  argv[1]  argv[2]      argv[3]
       cout << "Inside child process of test, let's run our main server: " << endl;
       execl(executable, executable, "8000", "127.0.0.1", "8888", (char*)0);

       /* exec does not return unless the program couldn't be started. 
          If we made it to this point, that means something went wrong.
          when the child process stops, the waitpid() above will return.
       */
       cout << "FAILED TO RUN COMMAND TO START SERVER" << endl;


   }
   return status; // this is the parent process again.
}


int main() {

   cout << "STARTING SERVER" << endl;
   cout << "STATUS: " << startMainServer() << endl;

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