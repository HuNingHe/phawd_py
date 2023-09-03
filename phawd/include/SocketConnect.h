/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file SocketConnect.h
 * @brief definition of socket communication in phawd
 */
#if _WIN32
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
#elif __linux__
#include<sys/select.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#endif

#pragma once
#include <string>
namespace phawd {
/*!
 * This class is used for socket communication.
 * Server should call Init() first to create a socket and open internet library, then call bind() to bind the socket with an specified ip address.
 * After calling these two functions, start calling listen() to listen to the socket.
 * Then, call accept() to wait a connect request.(listenToClient do all of these)
 * Before the program ends, it is necessary to call close() to free heap memory to prevent memory leakage
 *
 * Client should call Init() first to create a socket and open internet library, then call connectToServer() to create a connection with the specified ip address.
 * Then the connection between server and client finished.
 *
 * Now we can use read()/send() to exchange data between server and client.
 * getRead()/setSent()
 * This member of message is used to provide internal errors and build information to the outside.
 *
 * We can even use this class to implement communication between Linux and Windows
 * !!!!Init() and Close should use in pairs, and only call one time, otherwise program crashed.!!!!
 */

template<typename SendData, typename ReadData>
class SocketConnect {
private:
    bool isServer = false;
#if _WIN32
    SOCKET socket_fd, connected_fd;
    SOCKADDR_IN  _clientAddr{};
#elif __linux
    int socket_fd, connected_fd;
    sockaddr_in  _clientAddr{};
#endif
    size_t _sendSize;
    size_t _readSize;
    SendData *_sendData = nullptr;
    ReadData *_readData = nullptr;
public:
    SocketConnect();

    ~SocketConnect();

    void Init(size_t sendSize, size_t readSize, bool is_server = false);

    void connectToServer(const std::string& serverIP, unsigned short port, long int milliseconds = 30);

    void listenToClient(unsigned short port, int listenQueueLength = 2, long int milliseconds = 60);

    /*!
     * @return : -1 send failed , else send success
     */
    int Send(bool verbose = false);

    /*!
     * @return : -1 read failed , else read success
     */
    int Read(bool verbose = false);

    void Close();

    SendData *getSend();
    /*!
     * Call this after you call init(), otherwise program crashed
     */
    ReadData *getRead();
};

}
