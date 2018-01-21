#ifndef _CPP_ART_QRCODE_ART_QRCODE_PLAN_H_
#define _CPP_ART_QRCODE_ART_QRCODE_PLAN_H_

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <opencv2/opencv.hpp>
#include "ArtQRCode.h"

extern "C"
{
#include "../qrencode/qrencode.h"
}


using uchar = unsigned char;
using infoXY = std::pair<int, int>;
using targetWeight = std::pair<uchar, float>;

class ArtQRCodePlan
{
public:
    
    ArtQRCodePlan(int version, int mask, QRecLevel level);

    
    ~ArtQRCodePlan();

    
    std::unique_ptr<ArtQRCode> createArtQRCode(const std::string & content, const cv::Mat &tImg, const cv::Mat &tWgt) const;

    
    std::unique_ptr<ArtQRCode> createGeneralQRCode(const std::string & conent) const;

private:
 
    
    void initMaskImage();

    
    std::vector<targetWeight> getTargetSequence(const cv::Mat &img, const cv::Mat & weight) const;

    
    std::vector<infoXY> getSequencePosition() const;

private:
	int m_version;                       			
	int m_mask;                          			
	int m_qrwidth;                       			
	QRecLevel m_level;                   			
	std::vector<infoXY> m_fillerPosition;			
	
	
    std::unique_ptr<uchar[],decltype(free)*> m_maskImage;

};

#endif

