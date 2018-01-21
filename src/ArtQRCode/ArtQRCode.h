#ifndef _CPP_ART_QRCODE_ART_QRCODE_H_
#define _CPP_ART_QRCODE_ART_QRCODE_H_

#include <memory>

extern "C"
{
#include "../qrencode/qrencode.h"
}

using uchar = unsigned char;


class ArtQRCode
{
public:
    
    inline ArtQRCode(const int version, const QRecLevel level, const int mask);
    inline ~ArtQRCode();

    
    inline int getVersion() const;

    
    inline QRecLevel getRecLevel() const;

    
    inline int getMask() const;

    
    inline int getWidth() const;

    
    inline bool setData(const uchar * src, const int size);

    
    inline std::shared_ptr<uchar> getData() const;
private:
    int m_version;
    QRecLevel m_level;
    int m_mask;
    std::shared_ptr<uchar> m_data;
};


inline ArtQRCode::ArtQRCode(const int version, const QRecLevel level, const int mask)
{
    if (1 <= version && version <= 40)
    {
        m_version = version;
    }
    else
    {
        m_version = 5;  
    }

    if (QR_ECLEVEL_L <= level && level <= QR_ECLEVEL_H)
    {
        m_level = level;
    }
    else
    {
        m_level = QR_ECLEVEL_M;
    }

    if (0 <= mask && mask <= 7)
    {
        m_mask = mask;
    }
    else
    {
        m_mask = 2;
    }
    
    m_data.reset(new uchar[(m_version * 4 + 17)*(m_version * 4 + 17)], [](uchar * p){delete[]p; });
}


inline ArtQRCode::~ArtQRCode()
{
}


inline int ArtQRCode::getVersion() const
{
    return m_version;
}


inline QRecLevel ArtQRCode::getRecLevel() const
{
    return m_level;
}


inline int ArtQRCode::getMask() const
{
    return m_mask;
}


inline int ArtQRCode::getWidth() const
{
    return m_version * 4 + 17;
}


inline bool ArtQRCode::setData(const uchar * src, const int size)
{
    if (size == (m_version * 4 + 17)*(m_version * 4 + 17))
    {
        memcpy(m_data.get(), src, size);
        return true;
    }
    else
    {
        return false;
    }
}


inline std::shared_ptr<uchar> ArtQRCode::getData() const
{
    return m_data;
}


#endif 

