#ifndef QS_TCPSERVER_H
#define QS_TCPSERVER_H

#include "qs_tcpconnection.hpp"

class QS_TcpServer;

typedef boost::function<void(boost::shared_ptr<QS_TcpConnection>, const boost::system::error_code &)> TFuncAcceptHandler;

class Callback_AcceptHandler
{
public:
    Callback_AcceptHandler(TFuncAcceptHandler fnHandleAccept);
    void HandleAccept(boost::shared_ptr<QS_TcpServer> ptrTcpServer, boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code& ec);
private:
    TFuncAcceptHandler m_fnHandleAccept;
};

class QS_TcpServer : public boost::enable_shared_from_this<QS_TcpServer>
{
public:
    static boost::shared_ptr<QS_TcpServer> Create(boost::asio::io_service& ioService, unsigned short usPort)
    {
        return boost::shared_ptr<QS_TcpServer>(new QS_TcpServer(ioService, usPort));
    }

    friend class Callback_AcceptHandler;

    void AsyncAccept(TFuncAcceptHandler fnHandleAccept)
    {
        boost::shared_ptr<QS_TcpConnection> ptrConnection = QS_TcpConnection::Create(m_acceptor.get_io_service());
        boost::shared_ptr<Callback_AcceptHandler> acceptHandler(new Callback_AcceptHandler(fnHandleAccept));
        m_acceptor.async_accept(
            ptrConnection->Socket(),
            boost::bind(&Callback_AcceptHandler::HandleAccept,
                acceptHandler,
                shared_from_this(),
                ptrConnection,
                boost::asio::placeholders::error));
    }

private:
    QS_TcpServer(boost::asio::io_service& ioService, unsigned short usPort)
        : m_acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), usPort))
    {
    }

private:
    boost::asio::ip::tcp::acceptor m_acceptor;
};

#endif // QS_TCPSERVER_H
