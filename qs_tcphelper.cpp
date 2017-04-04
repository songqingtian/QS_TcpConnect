#include "qs_tcphelper.h"

// 短整型大小端互换
#define BigLittleSwap16(A)  ((((unsigned short)(A) & 0xff00) >> 8) | \
                            (((unsigned short)(A) & 0x00ff) << 8))
// 长整型大小端互换
#define BigLittleSwap32(A)  ((((unsigned int)(A) & 0xff000000) >> 24) | \
                            (((unsigned int)(A) & 0x00ff0000) >> 8) | \
                            (((unsigned int)(A) & 0x0000ff00) << 8) | \
                            (((unsigned int)(A) & 0x000000ff) << 24))
// 64位长整型大小端互换
#define BigLittleSwap64(A) ((((unsigned long long)(A) & 0xff00000000000000) >> 56) | \
                            (((unsigned long long)(A) & 0x00ff000000000000) >> 40) | \
                            (((unsigned long long)(A) & 0x0000ff0000000000) >> 24) | \
                            (((unsigned long long)(A) & 0x000000ff00000000) >> 8) | \
                            (((unsigned long long)(A) & 0x00000000ff000000) << 8) | \
                            (((unsigned long long)(A) & 0x0000000000ff0000) << 24) | \
                            (((unsigned long long)(A) & 0x000000000000ff00) << 40) | \
                            (((unsigned long long)(A) & 0x00000000000000ff) << 56))

// 本机大端返回1，小端返回0
int checkCPUendian()
{
    union{
        unsigned long int i;
        unsigned char s[4];
    }c;

    c.i = 0x12345678;
    return (0x12 == c.s[0]);
}

// 模拟htons函数，本机字节序转网络字节序
unsigned short int t_htons(unsigned short int h)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，转换成大端再返回
    return checkCPUendian() ? h : BigLittleSwap16(h);
}

// 模拟ntohs函数，网络字节序转本机字节序
unsigned short int t_ntohs(unsigned short int n)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，网络数据转换成小端再返回
    return checkCPUendian() ? n : BigLittleSwap16(n);
}

// 模拟htonl函数，本机字节序转网络字节序
unsigned long int t_htonl(unsigned int h)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，转换成大端再返回
    return checkCPUendian() ? h : BigLittleSwap32(h);
}

// 模拟ntohl函数，网络字节序转本机字节序
unsigned long int t_ntohl(unsigned int n)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，网络数据转换成小端再返回
    return checkCPUendian() ? n : BigLittleSwap32(n);
}

// 本机字节序转网络字节序
unsigned long long t_htonl64(unsigned long long h)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，转换成大端再返回
    return checkCPUendian() ? h : BigLittleSwap64(h);
}

// 网络字节序转本机字节序
unsigned long long t_ntohl64(unsigned long long n)
{
    // 若本机为大端，与网络字节序同，直接返回
    // 若本机为小端，网络数据转换成小端再返回
    return checkCPUendian() ? n : BigLittleSwap64(n);
}

QS_TcpBuffer::QS_TcpBuffer(unsigned int uiBufLen)
    :m_uiBufLen(uiBufLen), m_uiTransferred(0), m_uiReadOffset(0)
{
    if (m_uiBufLen == 0){
        m_ucDataBuf = nullptr;
    } else{
        m_ucDataBuf = new unsigned char[uiBufLen];
    }
}

QS_TcpBuffer::~QS_TcpBuffer()
{
    if (m_ucDataBuf){
        delete[] m_ucDataBuf;
        m_ucDataBuf = nullptr;
    }
    if (m_uiBufLen != 0){
        m_uiBufLen = 0;
    }
    if (m_uiTransferred != 0){
        m_uiTransferred = 0;
    }
    if(m_uiReadOffset != 0){
        m_uiReadOffset = 0;
    }
}

unsigned char * QS_TcpBuffer::GetDataPtr()
{
    return m_ucDataBuf;
}

unsigned int QS_TcpBuffer::GetBufLen()
{
    return m_uiBufLen;
}

void QS_TcpBuffer::SetTransferred(unsigned int uiTransferred)
{
    m_uiTransferred = uiTransferred;
}

unsigned int QS_TcpBuffer::GetTransferred()
{
    return m_uiTransferred;
}

void QS_TcpBuffer::SetReadOffset(unsigned int uiReadOffset)
{
    m_uiReadOffset = uiReadOffset;
}

unsigned int QS_TcpBuffer::GetReadOffset()
{
    return m_uiReadOffset;
}

#include <boost/system/system_error.hpp>
bool GetErrMsg(const boost::system::error_code & ec, std::string & stdstrMsg)//返回true表示有错误，错误信息为stdstrMsg， 返回false表示无错误
{
    if (ec) {
        stdstrMsg = std::string(boost::system::system_error(ec).what());
        return true;
    }
    return false;
}
