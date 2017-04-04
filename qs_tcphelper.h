#ifndef QS_TCPHELPER_H
#define QS_TCPHELPER_H

unsigned short t_htons(unsigned short h);
unsigned short t_ntohs(unsigned short n);
unsigned long int t_htonl(unsigned int h);
unsigned long int t_ntohl(unsigned int n);
unsigned long long t_htonl64(unsigned long long h);
unsigned long long t_ntohl64(unsigned long long n);

#include <boost/noncopyable.hpp>
class QS_TcpBuffer : private boost::noncopyable
{
public:
    QS_TcpBuffer(const unsigned int uiBufLen);
    ~QS_TcpBuffer();

    unsigned char * GetDataPtr();
    unsigned int GetBufLen();
    void SetTransferred(unsigned int uiTransferred);
    unsigned int GetTransferred();
    void SetReadOffset(unsigned int uiReadOffset);
    unsigned int GetReadOffset();

private:
    unsigned char * m_ucDataBuf;
    unsigned int m_uiBufLen;
    unsigned int m_uiTransferred;
    unsigned int m_uiReadOffset;
};

#include <boost/system/error_code.hpp>
bool GetErrMsg(const boost::system::error_code & ec, std::string & stdstrMsg);//返回true表示有错误，错误信息为stdstrMsg， 返回false表示无错误

#endif // QS_TCPHELPER_H
