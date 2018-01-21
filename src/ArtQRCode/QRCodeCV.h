#ifndef _CPP_ART_QRCODE_QRCODE_CV_H_
#define _CPP_ART_QRCODE_QRCODE_CV_H_

#include "../qrencode/qrencode.h"
#include "ArtQRCode.h"
#include <opencv2/opencv.hpp>
#include <assert.h>

class QRCodeCV
{
public:

    
    QRCodeCV(const QRcode * qr);


    
    QRCodeCV(const ArtQRCode * qr);



    
    ~QRCodeCV();
    

    
    cv::Mat getImage();

    
    cv::Mat getDataAndEccMask();

    
    cv::Mat getFormatMask();

    
    cv::Mat getVersionMask();

    
    cv::Mat getTimeMask();

    
    cv::Mat getAlignmentMask();

    
    cv::Mat getFinderAndSeparatorMask();

    
    cv::Mat getNoDataMask();

    
    inline int getVersion()
    {
        assert(m_qrcodeData.cols == m_qrcodeData.rows && (m_qrcodeData.cols - 17) % 4 == 0);
        assert(((m_qrcodeData.cols - 17) / 4) <= 40 && ((m_qrcodeData.cols - 17) / 4) >= 1);
        return (m_qrcodeData.cols - 17) / 4;
    };

private:

    
    cv::Mat m_qrcodeData;
};

#endif

