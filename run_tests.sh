#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/src
make all
cd $DIR/tests
make all
trap "" SIGINT #We disallow an interrupt signal in our current shell
(trap SIGINT; ./multiProxyTest) #we make a subshell where we run our program. We allow an interrupt signal in this subshell
#TODO: We can avoid doing this subshell method by simply writing 'trap true SIGINT' before invoking our test program, not sure why this is?
make cleanTests
cd ../src
make clean
