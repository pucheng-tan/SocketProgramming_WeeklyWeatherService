# SocketProgramming_WeeklyWeatherService
Implementing several servers and and clients for a simple weekly weather service, in C.

CMPT 434 Assignment 1 Part A

Name: Pucheng Tan

Instructions to run the program:

----------------------question1----------------------

files for this question: Makefile, server.c, client.c

Instruction to run the program:
The port number of the TCP-based server was set on line 18 in server.c 
   to be 33300, but you can change it to any idle port number between 30000 and 40000.
1. Open two console windows.
2. Use 'make' command to run the Makefile
3. In the first console window, use 'server' command to run the server
4. In the second console window, use 'client' command and command arguments
   the host name and port number of the server. for example 
   'client localhost 37200'
5. Flow the prompt to enter the day of a week. To be noticed, the valid arguments
   are: 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday',
   'Sunday', 'quit'(to quit the program).
   Days in all lower cases, such as 'monday' or 'friday', are not supported.
   
----------------------question2---------------------- 

files for this question: Makefile, server.c, proxy2.c client.c

Instruction to run the program:
The port number of the proxy server was set on line 18 in proxy2.c 
   to be 38500, but you can change it to any idle port number between 30000 and 40000.
1. Open three console windows.
2. Use 'make' command to run the Makefile
3. In the first console window, use 'server' command to run the server
4. In the second console window, use 'proxy2' command and command arguments
   the host name and port number of the server. for example 
   'proxy2 localhost 37200'
5. In the third console window, use 'client' command and command arguments
   the host name and port number of the proxy server. for example 
   'client localhost 37400'
6. Flow the prompt to enter the day of a week. To be noticed, the valid arguments
   are: 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday',
   'Sunday', 'quit'(to quit the program), 'all'(get the weather of 7 days of the week)
   Days in all lower cases, such as 'monday' or 'friday', are not supported.

----------------------question3----------------------

(a).
files for this question: Makefile, listener.c, talker.c

Instruction to run the program:
The port number of the UDP-based server was set on line 16 in listener.c 
   to be 39500, but you can change it to any idle port number between 30000 and 40000.
1. Open two console windows.
2. Use 'make' command to run the Makefile
3. In the first console window, use 'listener' command to run the server
4. In the second console window, use 'talker' command and command arguments
   the host name and port number of the server. for example 
   'talker localhost 38200'
5. Flow the prompt to enter the day of a week. To be noticed, the valid arguments
   are: 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday',
   'Sunday', 'quit'(to quit the program)
   Days in all lower cases, such as 'monday' or 'friday', are not supported.

(b).
files for this question: Makefile, listener.c, proxy3.c client.c

Instruction to run the program:
The port number of the proxy server was set on line 18 in proxy3.c 
   to be 39600, but you can change it to any idle port number between 30000 and 40000.
1. Open three console windows.
2. Use 'make' command to run the Makefile
3. In the first console window, use 'listener' command to run the server
4. In the second console window, use 'proxy3' command and command arguments
   the host name and port number of the server. for example 
   'proxy3 localhost 38400'
5. In the third console window, use 'client' command and command arguments
   the host name and port number of the proxy server. for example 
   'client localhost 37400'
6. Flow the prompt to enter the day of a week. To be noticed, the valid arguments
   are: 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday',
   'Sunday', 'quit'(to quit the program), 'all'(get the weather of 7 days of the week)
   Days in all lower cases, such as 'monday' or 'friday', are not supported.


----------------------Limitation----------------------

There is one limitation to the input 'all'. 'all' is supported as an valid input
by question 2 and 3, but should not be supported by question 1. However, there is 
only one client.c used for all questions. It's hard to solve this problem, because
client.c would not know if it is connecting to a proxy server or a real server.
When user types 'all' as the question 1 input, the following would display:

client side:
Enter the day of a week
all
client: connect: Connection refused
client: connecting to 127.0.0.1

Weather Summary: 

server side:
server: got connection from 127.0.0.1
server: received 'all'
send: Bad address

The good news is that although 'all' would be treated as an valid input by mistake,
the program would not crash when such situation happens. Users can continue play with
the program by typing a valid input.
