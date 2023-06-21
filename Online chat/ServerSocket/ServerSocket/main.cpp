#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <cstdlib> // Для выполнения арифметических операций

#pragma comment (lib, "ws2_32.lib")

using namespace std;

double evaluateExpression(const string& expression)
{
    double result = 0.0;

    try
    {
        // Поток для разбора арифметического выражения
        istringstream iss(expression);
        double operand1, operand2;
        char oper;
        iss >> operand1 >> oper >> operand2;

        if (iss.fail())
            throw invalid_argument("Invalid expression");

        // Проверка деления на ноль
        if (oper == '/' && operand2 == 0)
            throw runtime_error("Division by zero");

        // Выполнение арифметической операции
        switch (oper)
        {
        case '+':
            result = operand1 + operand2;
            break;
        case '-':
            result = operand1 - operand2;
            break;
        case '*':
            result = operand1 * operand2;
            break;
        case '/':
            result = operand1 / operand2;
            break;
        default:
            throw invalid_argument("Invalid operator");
        }
    }
    catch (const exception& e)
    {
        cerr << "Error evaluating expression: " << e.what() << endl;
        return numeric_limits<double>::quiet_NaN(); // Возвращаем NaN в случае ошибки
    }

    return result;
}


int main()
{
    // Initialze winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOk = WSAStartup(ver, &wsData);
    if (wsOk != 0)
    {
        cerr << "Can't Initialize winsock! Quitting" << endl;
        return 99;
    }

    // Create a socket
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET)
    {
        cerr << "Can't create a socket! Quitting" << endl;
        return 99;
    }

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(8855);
    hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

    bind(listening, (sockaddr*)&hint, sizeof(hint));

    // Tell Winsock the socket is for listening 
    listen(listening, SOMAXCONN);

    // Create the master file descriptor set and zero it
    fd_set master;
    FD_ZERO(&master);

    // Add our first socket that we're interested in interacting with; the listening socket!
    // It's important that this socket is added for our server or else we won't 'hear' incoming
    // connections 
    FD_SET(listening, &master);

    // this will be changed by the \quit command (see below, bonus not in video!)
    bool running = true;

    while (running)
    {
        // Make a copy of the master file descriptor set, this is SUPER important because
        // the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
        // are accepting inbound connection requests OR messages. 

        // E.g. You have a server and it's master file descriptor set contains 5 items;
        // the listening socket and four clients. When you pass this set into select(), 
        // only the sockets that are interacting with the server are returned. Let's say
        // only one client is sending a message at that time. The contents of 'copy' will
        // be one socket. You will have LOST all the other sockets.

        // SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

        fd_set copy = master;

        // See who's talking to us
        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        // Loop through all the current connections / potential connect
        for (int i = 0; i < socketCount; i++)
        {
            // Makes things easy for us doing this assignment
            SOCKET sock = copy.fd_array[i];

            // Is it an inbound communication?
            if (sock == listening)
            {
                // Accept a new connection
                SOCKET client = accept(listening, nullptr, nullptr);

                // Add the new connection to the list of connected clients
                FD_SET(client, &master);

                // Send a welcome message to the connected client
                string welcomeMsg = "SERVER: Server is working stable!";
                send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
            }
            else // It's an inbound message
            {
                char buf[4096];
                ZeroMemory(buf, 4096);

                // Receive message
                int bytesIn = recv(sock, buf, 4096, 0);
                if (bytesIn <= 0)
                {
                    // Drop the client
                    closesocket(sock);
                    FD_CLR(sock, &master);
                }
                else
                {
                    // Check to see if it's a command. \quit kills the server
                    if (buf[0] == '\\')
                    {
                        // Is the command quit? 
                        string cmd = string(buf, bytesIn);
                        if (cmd == "\\quit")
                        {
                            running = false;
                            break;
                        }

                        // Unknown command
                        continue;
                    }

                    // Check if it's an arithmetic expression
                    string message = string(buf, bytesIn);
                    if (message.substr(0, 8) == "SERVER: ")
                    {
                        string expression = message.substr(8);
                        double result = evaluateExpression(expression);

                        ostringstream ss;
                        ss << "SERVER: " << expression << " = " << result << "\r\n";
                        string strOut = ss.str();

                        // Send the result to the client who sent the expression
                        send(sock, strOut.c_str(), strOut.size() + 1, 0);
                    }
                    else
                    {
                        // Send message to other clients, and definitely NOT the listening socket
                        for (u_int i = 0; i < master.fd_count; i++)
                        {
                            SOCKET outSock = master.fd_array[i];
                            if (outSock == listening || outSock == sock)
                            {
                                continue;
                            }

                            ostringstream ss;
                            ss << "User #" << sock << ": " << buf << "\r\n";

                            string strOut = ss.str();
                            send(outSock, strOut.c_str(), strOut.size() + 1, 0);
                        }

                        // Send message back to the client who sent it
                        ostringstream ss;
                        ss << "ME: " << buf << "\r\n";
                        string strOut = ss.str();
                        send(sock, strOut.c_str(), strOut.size() + 1, 0);
                    }
                }
            }
        }
    }

    // Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
    FD_CLR(listening, &master);
    closesocket(listening);

    // Message to let users know what's happening.
    string msg = "SERVER: Server is shutting down. Goodbye\r\n";

    while (master.fd_count > 0)
    {
        // Get the socket number
        SOCKET sock = master.fd_array[0];

        // Send the goodbye message
        send(sock, msg.c_str(), msg.size() + 1, 0);

        // Remove it from the master file list and close the socket
        FD_CLR(sock, &master);
        closesocket(sock);
    }

    // Cleanup winsock
    WSACleanup();

    system("pause");
    return 0;
}



