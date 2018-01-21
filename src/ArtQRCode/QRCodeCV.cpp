#include "QRCodeCV.h"


QRCodeCV::QRCodeCV(const QRcode * qr) :m_qrcodeData(qr->width,qr->width,CV_8UC1)
{
    for (decltype(m_qrcodeData.rows) i = 0; i < m_qrcodeData.rows; ++i)
    {
        for (decltype(m_qrcodeData.cols) j = 0; j < m_qrcodeData.cols; ++j)
        {
            m_qrcodeData.at<uchar>(i, j) = *(qr->data + i*qr->width + j);
        }
    }
}


QRCodeCV::QRCodeCV(const ArtQRCode * qr) :m_qrcodeData(qr->getVersion() * 4 + 17, qr->getVersion() * 4 + 17, CV_8UC1)
{
    int qrw = qr->getVersion() * 4 + 17;
    
    for (decltype(m_qrcodeData.rows) i = 0; i < m_qrcodeData.rows; ++i)
    {
        for (decltype(m_qrcodeData.cols) j = 0; j < m_qrcodeData.cols; ++j)
        {
            m_qrcodeData.at<uchar>(i, j) = *(qr->getData().get() + i*qrw + j);
        }
    }
}

QRCodeCV::~QRCodeCV()
{
}


cv::Mat QRCodeCV::getImage()
{
    return ((m_qrcodeData & 0x01) ^ 0x01) * 255;
}


cv::Mat QRCodeCV::getDataAndEccMask()
{
    return (m_qrcodeData & 0x02) / 2;
}


cv::Mat QRCodeCV::getFormatMask()
{
    return (m_qrcodeData & 0x04) / 4;
}


cv::Mat QRCodeCV::getVersionMask()
{
    return (m_qrcodeData & 0x08) / 8;
}


cv::Mat QRCodeCV::getTimeMask()
{
    return (m_qrcodeData & 0x10) / 16;
}


cv::Mat QRCodeCV::getAlignmentMask()
{
    return (m_qrcodeData & 0x20) / 32;
}


cv::Mat QRCodeCV::getFinderAndSeparatorMask()
{
    return (m_qrcodeData & 0x40) / 64;
}


cv::Mat QRCodeCV::getNoDataMask()
{
    return (m_qrcodeData & 0x80) / 128;
}

