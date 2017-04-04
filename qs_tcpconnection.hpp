#ifndef QS_TCPCONNECTION_HPP
#define QS_TCPCONNECTION_HPP

#define BOOST_ASIO_ENABLE_CANCELIO
#define SERVER_SIDE 1
#if SERVER_SIDE
#define CLIENT_SIDE 0
#elif
#define CLIENT_SIDE 1
#endif
#if CLIENT_SIDE
#ifdef _WIN32
#define BOOST_ASIO_DISABLE_IOCP
#endif
#endif

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include "qs_tcphelper.h"

class QS_TcpConnection;

typedef boost::function<void(const boost::system::error_code &)> TFuncConnectHandler;
#if SERVER_SIDE
typedef boost::function<void(const boost::system::error_code &)> TFuncDisconnectHandler;
#endif
typedef boost::function<void(const boost::system::error_code &, size_t)> TFuncWriteHandler;
typedef boost::function<void(boost::shared_ptr<QS_TcpConnection>, boost::shared_ptr<QS_TcpBuffer>, const boost::system::error_code &)> TFuncReadLenHandler;
typedef boost::function<void(boost::shared_ptr<QS_TcpConnection>, boost::shared_ptr<QS_TcpBuffer>, const boost::system::error_code &, size_t)> TFuncReadSomeHandler;

struct TaskWrite{
    TaskWrite(boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, TFuncWriteHandler fnHandleWrite){
        this->ptrTcpBuf = ptrTcpBuf;
        this->fnHandleWrite = fnHandleWrite;
    }
    boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf;
    TFuncWriteHandler fnHandleWrite;
};

class Callback_ConnectHandler
{
public:
    Callback_ConnectHandler(TFuncConnectHandler fnHandleConnect);
    void HandleConnect(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code & ec);
private:
    TFuncConnectHandler m_fnHandleConnect;
};

#if SERVER_SIDE
class Callback_DisconnectHandler
{
public:
    Callback_DisconnectHandler(TFuncDisconnectHandler fnHandleDisconnect);
    void HandleDisconnect(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code & ec);
private:
    TFuncDisconnectHandler m_fnHandleDisconnect;
};
#endif

class Callback_WriteHandler
{
public:
    Callback_WriteHandler(TFuncWriteHandler fnHandleWrite);
    void HandleWrite(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code& ec, size_t szBytesTransferred);
private:
    TFuncWriteHandler m_fnHandleWrite;
};

class Callback_ReadLenHandler
{
public:
    Callback_ReadLenHandler(TFuncReadLenHandler fnHandleReadLen);
    void HandleReadLen(boost::shared_ptr<QS_TcpConnection> ptrConnection, boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, const boost::system::error_code & ec);
private:
    TFuncReadLenHandler m_fnHandleReadLen;
};

class Callback_ReadSomeHandler
{
public:
    Callback_ReadSomeHandler(TFuncReadSomeHandler fnHandleReadSome);
    void HandleReadSome(boost::shared_ptr<QS_TcpConnection> ptrConnection, boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, const boost::system::error_code & ec, size_t szTransferred);
private:
    TFuncReadSomeHandler m_fnHandleReadSome;
};

class QS_TcpConnection : public boost::enable_shared_from_this<QS_TcpConnection>
{
public:
    static boost::shared_ptr<QS_TcpConnection> Create(boost::asio::io_service& ioService)
    {
        return boost::shared_ptr<QS_TcpConnection>(new QS_TcpConnection(ioService));
    }

    friend class Callback_ConnectHandler;
#if SERVER_SIDE
    friend class Callback_DisconnectHandler;
#endif
    friend class Callback_WriteHandler;
    friend class Callback_ReadLenHandler;
    friend class Callback_ReadSomeHandler;

    boost::asio::ip::tcp::socket& Socket()
    {
        return boost::ref(m_socket);
    }

    void AsyncConnect(const char * chAddr, const unsigned short usPort, TFuncConnectHandler fnHandleConnect)
    {
        boost::asio::ip::tcp::endpoint epServer(boost::asio::ip::address_v4::from_string(chAddr), usPort);
        boost::shared_ptr<Callback_ConnectHandler> connectHandler(new Callback_ConnectHandler(fnHandleConnect));
        m_socket.async_connect(epServer, boost::bind(&Callback_ConnectHandler::HandleConnect, connectHandler, shared_from_this(), boost::asio::placeholders::error));
    }

#if SERVER_SIDE
    void Disconnect(TFuncDisconnectHandler fnHandleDisconnect)
    {
        while(true){
            boost::mutex::scoped_lock lock(m_mutexWriteQueue);
            if(m_queueTaskWrite.empty()){
                break;
            }
            SleepMilliSeconds(1);
        }
        m_socket.shutdown(boost::asio::socket_base::shutdown_send);
        boost::shared_ptr<Callback_DisconnectHandler> ptrDisconnectHandler(new Callback_DisconnectHandler(fnHandleDisconnect));
        boost::asio::async_read(m_socket, boost::asio::null_buffers(),
            boost::bind(&Callback_DisconnectHandler::HandleDisconnect,
            ptrDisconnectHandler,
            shared_from_this(),
            boost::asio::placeholders::error));
    }
#elif CLIENT_SIDE
    void Disconnect()
    {
        boost::asio::strand strand_(m_socket.get_io_service());
        boost::system::error_code ec;
        strand_.post(boost::bind(&boost::asio::ip::tcp::socket::cancel, &m_socket, boost::ref(ec)));
        //strand_.post(boost::bind(&QS_TcpConnection::SleepMilliSeconds, shared_from_this(), 1000));
        strand_.post(boost::bind(&boost::asio::ip::tcp::socket::shutdown, &m_socket, boost::asio::socket_base::shutdown_both, boost::ref(ec)));
        //strand_.post(boost::bind(&QS_TcpConnection::SleepMilliSeconds, shared_from_this(), 1000));
        strand_.post(boost::bind(&boost::asio::ip::tcp::socket::close, &m_socket, boost::ref(ec)));
    }
#endif

    void AsyncSend(boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, TFuncWriteHandler fnHandleWrite)
    {
        boost::shared_ptr<TaskWrite> ptrTaskWrite(new TaskWrite(ptrTcpBuf, fnHandleWrite));
        boost::mutex::scoped_lock lock(m_mutexWriteQueue);
        if (m_queueTaskWrite.empty()){
            m_queueTaskWrite.push_back(ptrTaskWrite);
            boost::shared_ptr<TaskWrite> ptrTaskWriteFront = m_queueTaskWrite.front();
            AsyncWrite(ptrTaskWriteFront->ptrTcpBuf, ptrTaskWriteFront->fnHandleWrite);
        } else{
            m_queueTaskWrite.push_back(ptrTaskWrite);
        }
    }

    /*asio::async_read通常用户读取指定长度的数据，读完或出错才返回。
    *而socket的async_read_some读取到数据或出错就返回，不一定读完了整个包。
    */
    void AsyncRecvLen(boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, TFuncReadLenHandler fnHandleReadLen)
    {
        boost::shared_ptr<Callback_ReadLenHandler> readLenHandler(new Callback_ReadLenHandler(fnHandleReadLen));
        boost::asio::async_read(
            m_socket,
            boost::asio::buffer(ptrTcpBuf->GetDataPtr(), ptrTcpBuf->GetBufLen()),
            boost::bind(&Callback_ReadLenHandler::HandleReadLen,
                readLenHandler,
                shared_from_this(),
                ptrTcpBuf,
                boost::asio::placeholders::error));
    }

    void AsyncRecvSome(boost::shared_ptr<QS_TcpBuffer> ptrBuf, TFuncReadSomeHandler fnHandleReadSome)
    {
        boost::shared_ptr<Callback_ReadSomeHandler> readSomeHandler(new Callback_ReadSomeHandler(fnHandleReadSome));
        m_socket.async_read_some(
            boost::asio::buffer(ptrBuf->GetDataPtr(), ptrBuf->GetBufLen()),
            boost::bind(&Callback_ReadSomeHandler::HandleReadSome,
                readSomeHandler,
                shared_from_this(),
                ptrBuf,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:
    QS_TcpConnection(boost::asio::io_service& ioService)
        :m_socket(ioService)
    {
    }

    void AsyncWrite(boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, TFuncWriteHandler fnHandleWrite)
    {
        boost::shared_ptr<Callback_WriteHandler> writeHandler(new Callback_WriteHandler(fnHandleWrite));
        boost::asio::async_write(
            m_socket,
            boost::asio::buffer(ptrTcpBuf->GetDataPtr(), ptrTcpBuf->GetBufLen()),
            boost::asio::transfer_at_least(ptrTcpBuf->GetBufLen()),
            boost::bind(&Callback_WriteHandler::HandleWrite,
                writeHandler,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void SleepMilliSeconds(long lMilliSec)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(lMilliSec));
    }

private:
    boost::asio::ip::tcp::socket m_socket;
    boost::mutex m_mutexWriteQueue;
    std::list<boost::shared_ptr<TaskWrite>> m_queueTaskWrite;
};

#endif // QS_TCPCONNECTION_HPP
