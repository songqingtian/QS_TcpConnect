#include "qs_tcpconnection.hpp"

Callback_ConnectHandler::Callback_ConnectHandler(TFuncConnectHandler fnHandleConnect) : m_fnHandleConnect(fnHandleConnect)
{
}

void Callback_ConnectHandler::HandleConnect(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code & ec)
{
    std::string stdstrErrMsg;
    bool bRet = GetErrMsg(ec, boost::ref(stdstrErrMsg));
    if (!bRet){
        ptrConnection->m_socket.set_option(boost::asio::ip::tcp::no_delay(true));
    }
    if (!m_fnHandleConnect.empty()){
        m_fnHandleConnect(ec);
    }
}

#if SERVER_SIDE
Callback_DisconnectHandler::Callback_DisconnectHandler(TFuncDisconnectHandler fnHandleDisconnect) : m_fnHandleDisconnect(fnHandleDisconnect)
{
}

void Callback_DisconnectHandler::HandleDisconnect(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code & ec)
{
    if (!ec){
        boost::shared_ptr<Callback_DisconnectHandler> ptrDisconnectHandler(new Callback_DisconnectHandler(m_fnHandleDisconnect));
        boost::asio::async_read(ptrConnection->m_socket, boost::asio::null_buffers(),
            boost::bind(&Callback_DisconnectHandler::HandleDisconnect,
            ptrDisconnectHandler,
            ptrConnection,
            boost::asio::placeholders::error));
    } else if (ec == boost::asio::error::eof){
        ptrConnection->m_socket.shutdown(boost::asio::socket_base::shutdown_receive);
        ptrConnection->m_socket.close();
    } else{
        if(!m_fnHandleDisconnect.empty()){
            m_fnHandleDisconnect(ec);
        }
    }
}
#endif

Callback_WriteHandler::Callback_WriteHandler(TFuncWriteHandler fnHandleWrite) : m_fnHandleWrite(fnHandleWrite)
{
}

void Callback_WriteHandler::HandleWrite(boost::shared_ptr<QS_TcpConnection> ptrConnection, const boost::system::error_code& ec, size_t szBytesTransferred)
{
    if (!m_fnHandleWrite.empty()){
        m_fnHandleWrite(ec, szBytesTransferred);
        m_fnHandleWrite.clear();
    }

    boost::mutex::scoped_lock lock(ptrConnection->m_mutexWriteQueue);
    ptrConnection->m_queueTaskWrite.pop_front();
    if (!ptrConnection->m_queueTaskWrite.empty()){
        boost::shared_ptr<TaskWrite> ptrTaskWriteFront = ptrConnection->m_queueTaskWrite.front();
        ptrConnection->AsyncWrite(ptrTaskWriteFront->ptrTcpBuf, ptrTaskWriteFront->fnHandleWrite);
    }
}

Callback_ReadLenHandler::Callback_ReadLenHandler(TFuncReadLenHandler fnHandleReadLen) : m_fnHandleReadLen(fnHandleReadLen)
{
}

void Callback_ReadLenHandler::HandleReadLen(boost::shared_ptr<QS_TcpConnection> ptrConnection, boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, const boost::system::error_code & ec)
{
    if(!m_fnHandleReadLen.empty()){
        m_fnHandleReadLen(ptrConnection, ptrTcpBuf, ec);
        m_fnHandleReadLen.clear();
    }
}

Callback_ReadSomeHandler::Callback_ReadSomeHandler(TFuncReadSomeHandler fnHandleReadSome) : m_fnHandleReadSome(fnHandleReadSome)
{
}

void Callback_ReadSomeHandler::HandleReadSome(boost::shared_ptr<QS_TcpConnection> ptrConnection, boost::shared_ptr<QS_TcpBuffer> ptrTcpBuf, const boost::system::error_code & ec, size_t szTransferred)
{
    if(m_fnHandleReadSome.empty()){
        m_fnHandleReadSome(ptrConnection, ptrTcpBuf, ec, szTransferred);
        m_fnHandleReadSome.clear();
    }
}
