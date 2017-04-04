#include "qs_tcpserver.hpp"

Callback_AcceptHandler::Callback_AcceptHandler(TFuncAcceptHandler fnHandleAccept) : m_fnHandleAccept(fnHandleAccept)
{
}

void Callback_AcceptHandler::HandleAccept(boost::shared_ptr<QS_TcpServer> ptrTcpServer, boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code& ec)
{
    ptrTcpServer->AsyncAccept(m_fnHandleAccept);

    if(!m_fnHandleAccept.empty()){
        m_fnHandleAccept(ptrConnection, boost::ref(ec));
    }
}
