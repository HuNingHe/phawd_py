/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file SocketConnect.cpp
 * @brief definition of socket communication in phawd
 */

#include "SharedParameter.h"
#include "SocketConnect.h"
using namespace phawd;

#if _WIN32
template<typename SendData, typename ReadData>
SocketConnect<SendData, ReadData>::SocketConnect() : socket_fd(INVALID_SOCKET), connected_fd(INVALID_SOCKET) {
    _sendSize = 0;
    _readSize = 0;
    memset(&_clientAddr, 0, sizeof(_clientAddr));
}

/*!
 * Check the version of Socket and Create socket.
 * Considering soft array, we don't use sizeof(SendData) and sizeof(ReadData).
 * @param sendSize: the bytes need to send
 * @param readSize: the bytes need to read
 * @param is_server: This determines whether this instance of SocketConnect is a server or not
 * @return: true if the init success
 */
template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::Init(size_t sendSize, size_t readSize, bool is_server){
    isServer = is_server;
    // we use non-block socket here
    if (sendSize <= 0 || readSize <= 0){
        printf("[Socket Connect] Init error, please input positive sendSize and readSize!\n");
        return;
    }

    WSADATA wsaData;
    WORD wVersionRet; // the version of socket
    wVersionRet = MAKEWORD(2,2);
    // Open socket library
    if (WSAStartup(wVersionRet, &wsaData) != 0){
        throw std::runtime_error("[Socket Connect] Open Socket Library failed!\n");
    }
    // Check that if the system support version of 2.2
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
        WSACleanup();
        throw std::runtime_error("[Socket Connect] The system does not support socket for version 2.2\n");
    }
    // Based on TCP protocol, we use SOCK_STREAM other than SOCK_DGRAM
    // Let the system decide for itself what protocol to use
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == INVALID_SOCKET){
        WSACleanup();
        throw std::runtime_error("[Socket Connect] Create socket failed!\n");
    }
    // If iMode = 0, blocking is enabled;
    // If iMode != 0, non-blocking mode is enabled.
    // see https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ioctlsocket
    unsigned long iMode = 1;
    int iResult = ioctlsocket(socket_fd, FIONBIO, (unsigned long *)&iMode);//设置成非阻塞模式
    if (iResult != NO_ERROR){
        throw std::runtime_error("Failed to set socket to non-block state");
    }

    _sendSize = sendSize;
    _readSize = readSize;
    _sendData = (SendData *)malloc(_sendSize);
    _readData = (ReadData *)malloc(_readSize);
    memset(_sendData, 0, _sendSize);
    memset(_readData, 0, _readSize);
    printf("[Socket Connect] Init Success!\n");
}

/*!
 * Used for client to connect a internet address
 * @param ipAddr: IP address of this server
 * @param port: port of this server
 * @param milliseconds: time limit for connect
 * @return: true if connect success
 */
template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::connectToServer(const std::string& ipAddr, unsigned short port, long int milliseconds){
    if (port <= 0) {
        printf("SocketConnect::connectToServer : invalid port!\n");
        return;
    }

    // 非阻塞式Socket使用
    memset(&_clientAddr, 0, sizeof(SOCKADDR_IN)); // init address
    _clientAddr.sin_family = AF_INET; // Declare that the address format follows TCP/IP
    _clientAddr.sin_port = htons(port);
    _clientAddr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
    connect(socket_fd, (LPSOCKADDR)&_clientAddr, sizeof(SOCKADDR_IN));

    // 由于服务端发数据时间是不定的, 因此我们不能使用阻塞socket, 否则一直阻塞在写数据或者读数据过程
    // 因此采用select模型来确保connect完成, 如果需要一对多通信则需要新开辟线程, 死循环中利用select对socket进行实时检测, 每检测到就accept对其进行读写, 最多管理64个socket
    //@detail:https://www.cnblogs.com/alantu2018/p/8612722.html
    // 可读和可写的socket I/O集合, select对加入集合的socket进行轮询检查, 检查集合的socket可读/可写状态，如变为可读/可写则退出等待
    /*! select模型中常见的集合操作有FD_ZERO、FD_SET、FD_ISSET、FD_CLR:
     *@FD_ZERO(*set): 清空IO集合set
     *@FD_SET(s, *set): 添加socket或者其他IO口s到IO集合set中
     *@FD_SETSIZE(*set): 设置集合最大元素个数
     *@FD_ISSET(s, *set): 判断IO口s是否在IO集合set中, 若是则返回非零
     *@FD_CLR(s, *set): 从IO集合set中删除IO口s, 并将描述符个数减一
     *@detail: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
     */

    /*!
     * 说明:
     * 1、处于监听状态的socket不会被select视为可读, 直到收到客户端的connect请求才会被视为可读
     * 2、客户端发出connect请求后, 必须在完成三次握手后, 客户端socket才会被select认为可写
     */
    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(socket_fd, &rset);
    FD_SET(socket_fd, &wset);

    struct timeval interval{};
    interval.tv_sec = milliseconds;
    interval.tv_usec = 0;
    // Windows端select函数第一个参数可忽略, 在linux端应该为socket_fd + 1
    // 第三个参数为异常的集合, 我们不考虑socket异常的情况
    // 第四个参数为最多阻塞的时间, 如果等待超时都没有检测到则放弃
    // 返回值为检测到符合要求的socket个数, 返回0表示超时, -1表示发生错误

    int n = select(0, &rset, &wset, nullptr, &interval);
    if(n == 0){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("connect timeout\n");
    }else if(n < 0){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("connect error, please check the ipv4 address format!\n");
    }else{
        printf("[Socket Connect] Connect success!\n");
    }
}

/*!
 * Used for server to bind a internet address, don't suggest to use here, because we use non-block socket
 * @param maxConnect: The maximum number of connections allowed
 * @param port: port of this program
 * @param milliseconds: time limit for accept
 * @return: true if bind success
 */
template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::listenToClient(unsigned short port, int listenQueueLength, long int milliseconds){
    if (port <= 0) {
        printf("SocketConnect::listenToClient : invalid port!\n");
        return;
    }
    memset(&_clientAddr, 0, sizeof(SOCKADDR_IN)); // init address
    _clientAddr.sin_family = AF_INET; // Declare that the address format follows TCP/IP
    _clientAddr.sin_port = htons(port);
    _clientAddr.sin_addr.s_addr = ADDR_ANY;

    if(bind(socket_fd, (LPSOCKADDR)&_clientAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("[Socket Connect] Bind faild!\n");
    }

    if(listen(socket_fd, listenQueueLength) == SOCKET_ERROR){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("[Socket Connect] Listen faild!\n");
    }
    printf("Enter listening state!");
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(socket_fd, &rset);

    struct timeval interval{};
    interval.tv_sec = milliseconds;
    interval.tv_usec = 0;
    // Windows端select函数第一个参数可忽略, 在linux端应该为socket_fd + 1
    // 第三个参数为异常的集合, 我们不考虑socket异常的情况
    // 第四个参数为最多阻塞的时间, 如果等待超时都没有检测到则放弃
    // 返回值为检测到符合要求的socket个数, 返回0表示超时, -1表示发生错误

    int n = select(0, &rset, nullptr, nullptr, &interval);
    if(n == 0){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("listen timeout\n");
    }else if(n < 0){
        closesocket(socket_fd);
        WSACleanup();
        throw std::runtime_error("listen error\n");
    }else{
        int nAddrlen = sizeof(_clientAddr);
        /*！accept函数说明:
         * 系统侦听listen到connect后, 把这个连接放入到接受accept队列中, 然后我们调用accept告诉客户端我们有空闲处理
         * 这将返回新的套接字, 用于读写操作, 而原来的套接字还在继续侦听
         */
        connected_fd = accept(socket_fd, (LPSOCKADDR)&_clientAddr, &nAddrlen);
        if(connected_fd == INVALID_SOCKET){
            throw std::runtime_error("accept error\n");
        }
    }
    printf("[Socket Connect] Bind and Listen success!\n");
}

template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::Close() {
    if(socket_fd != INVALID_SOCKET) {
        closesocket(socket_fd);
        socket_fd = INVALID_SOCKET;
        WSACleanup();
    }

    if(connected_fd != INVALID_SOCKET){
        closesocket(connected_fd);
        connected_fd = INVALID_SOCKET;
        WSACleanup();
    }

    if(_sendData){
        free(_sendData);
        _sendData = nullptr;
    }

    if(_readData){
        free(_readData);
        _readData = nullptr;
    }
    printf("[Socket Connect] Close Success\n");
}

#elif __linux__
template<typename SendData, typename ReadData>
SocketConnect<SendData, ReadData>::SocketConnect():socket_fd(-1), connected_fd(-1) {
    _sendSize = -1;
    _readSize = -1;
    memset(&_clientAddr, 0, sizeof(_clientAddr));
}

template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::Init(size_t sendSize, size_t readSize, bool is_server) {
    isServer = is_server;

    // we use non-block socket here
    if (sendSize <= 0 || readSize <= 0){
        printf("[Socket Connect] Init error, please input positive sendSize and readSize!\n");
        return;
    }
    socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(socket_fd < 0) {
        throw std::runtime_error("[Socket Connect] Create socket failed!\n");
    }

    _sendSize = sendSize;
    _readSize = readSize;
    free(_sendData);
    free(_readData);
    _sendData = (SendData *)malloc(_sendSize);
    _readData = (ReadData *)malloc(_readSize);
    memset(_sendData, 0, _sendSize);
    memset(_readData, 0, _readSize);
    printf("[Socket Connect] Init Success!\n");
}

template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::connectToServer(const std::string& serverIP, unsigned short port, long int milliseconds) {
    if (port <= 0) {
        printf("SocketConnect::connectToServer : invalid port!\n");
        return;
    }
    memset(&_clientAddr, 0, sizeof(_clientAddr));
    _clientAddr.sin_family = AF_INET;
    _clientAddr.sin_port = htons(port);
    // inet_pton return 1:success，0: invalid expression，-1: error
    if(inet_pton(AF_INET, serverIP.c_str(), &_clientAddr.sin_addr) <= 0) {
        throw std::runtime_error("SocketConnect::connectToServer : Format error in IP address");
    }
    connect(socket_fd, (struct sockaddr*)&_clientAddr, sizeof(_clientAddr));

    fd_set r_set, w_set;
    FD_ZERO(&r_set);
    FD_ZERO(&w_set);
    FD_SET(socket_fd, &r_set);
    FD_SET(socket_fd, &w_set);

    struct timeval interval{};
    interval.tv_sec = milliseconds;
    interval.tv_usec = 0;

    int n = select(socket_fd + 1, &r_set, &w_set, nullptr, &interval);
    if(n == 0){
        close(socket_fd);
        throw std::runtime_error("connect timeout\n");
    }else if(n < 0){
        close(socket_fd);
        throw std::runtime_error("connect error, please check the ipv4 address format!\n");
    }else{
        printf("[Socket Connect] Connect success!\n");
    }
}

template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::listenToClient(unsigned short port, int listenQueueLength, long int milliseconds) {
    memset(&_clientAddr, 0, sizeof(_clientAddr));
    sockaddr_in serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);// Set the IP address to INADDR_ANY to automatically obtain the IP address of the machine
    serverAddr.sin_port = htons(port);

    if(bind(socket_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
        close(socket_fd);
        throw std::runtime_error("bind socket error\n");
    }

    if(listen(socket_fd, listenQueueLength) == -1){
        close(socket_fd);
        throw std::runtime_error("listen socket error\n");
    }
    printf("Enter listening state!\n");

    fd_set r_set;
    FD_ZERO(&r_set);
    FD_SET(socket_fd, &r_set);
    struct timeval interval{};
    interval.tv_sec = milliseconds;
    interval.tv_usec = 0;

    int n = select(socket_fd + 1, &r_set, nullptr, nullptr, &interval);
    if(n == 0) {
        close(socket_fd);
        throw std::runtime_error("listen timeout\n");
    } else if(n < 0) {
        close(socket_fd);
        throw std::runtime_error("listen error\n");
    } else {
        socklen_t nAddrlen = sizeof(_clientAddr);
        // 第二个参数为返回的客户端地址, 第三个返回客户端发回的数据长度, 设置为NULL表示不关心
        connected_fd = accept(socket_fd, (struct sockaddr*)&_clientAddr, &nAddrlen);
        if(connected_fd < 0){
            throw std::runtime_error("accept error\n");
        }
    }
    printf("[Socket Connect] Bind and Listen success!\n");
}

template<typename SendData, typename ReadData>
void SocketConnect<SendData, ReadData>::Close(){
    if(socket_fd > 0) {
        close(socket_fd);
        socket_fd = -1;
    }

    if(connected_fd > 0){
        close(connected_fd);
        socket_fd = -1;
    }

    if(_sendData){
        free(_sendData);
        _sendData = nullptr;
    }

    if(_readData){
        free(_readData);
        _readData = nullptr;
    }
    printf("[Socket Connect] Close Success\n");
}
#endif

template<typename SendData, typename ReadData>
SocketConnect<SendData, ReadData>::~SocketConnect() {
    Close();
}

template<typename SendData, typename ReadData>
int SocketConnect<SendData, ReadData>::Send(bool verbose){
    if ( _sendSize <= 0 || _sendData == nullptr ) {
        printf("[Socket Connect] Send failed, Init first \n");
        return -1;
    }
    char *sendBuff = new char[_sendSize];
    memcpy(sendBuff ,_sendData, _sendSize);
    int nRet = 0;
    bool judge1 = false;
    bool judge2 = false;
#if _WIN32
    judge1 = connected_fd != INVALID_SOCKET;
    judge2 = socket_fd != INVALID_SOCKET;

#elif __linux__
    judge1 = connected_fd > 0;
    judge2 = socket_fd > 0;
#endif

    if (isServer) {
        if(judge1) {
            nRet = send(connected_fd, sendBuff, _sendSize, 0);
            if (nRet <= 0){
                if (verbose){
                    printf("[Socket Connect] Send failed! \n");
                }
                nRet = -1;
            }
        }
    } else {
        if(judge2){
            nRet = send(socket_fd, sendBuff, _sendSize, 0);
            if (nRet <= 0){
                if (verbose){
                    printf("[Socket Connect] Send failed! \n");
                }
                nRet = -1;
            }
        }
    }

    delete []sendBuff;
    sendBuff = nullptr;
    if (verbose){
        printf("[Socket Connect] Send Finished! \n");
    }
    return nRet;
}

template<typename SendData, typename ReadData>
int SocketConnect<SendData, ReadData>::Read(bool verbose){
    if (_readSize <= 0 || _readData == nullptr){
        printf("[Socket Connect] Read failed, Init first \n");
        return -1;
    }

    char *readBuff = new char[_readSize];
    int nRet = 0;
    bool judge1 = false;
    bool judge2 = false;
#if _WIN32
    judge1 = connected_fd != INVALID_SOCKET;
    judge2 = socket_fd != INVALID_SOCKET;

#elif __linux__
    judge1 = connected_fd > 0;
    judge2 = socket_fd > 0;
#endif
    if (isServer) {
        if(judge1){
            nRet = recv(connected_fd, readBuff, _readSize, 0);
            if (nRet <= 0){
                if (verbose){
                    printf("[Socket Connect] Read failed! \n");
                }
                nRet = -1;
            }
        }
    } else {
        if(judge2){
            nRet = recv(socket_fd, readBuff, _readSize, 0);
            if (nRet <= 0){
                if (verbose){
                    printf("[Socket Connect] Read failed! \n");
                }
                nRet = -1;
            }
        }
    }
    if (nRet > 0){
        memcpy(_readData, readBuff, _readSize);
    }
    delete []readBuff;
    readBuff = nullptr;
    if (verbose){
        printf("[Socket Connect] Read Finished! \n");
    }
    return nRet;
}

template<typename SendData, typename ReadData>
SendData *SocketConnect<SendData, ReadData>::getSend(){
    if (_sendData == nullptr){
        throw std::runtime_error("Init first!");
    }
    return _sendData;
}

template<typename SendData, typename ReadData>
ReadData *SocketConnect<SendData, ReadData>::getRead() {
    if (_readData == nullptr) {
        throw std::runtime_error("Init first!");
    }
    return _readData;
}

template class phawd::SocketConnect<SocketToPhawd, SocketFromPhawd>;