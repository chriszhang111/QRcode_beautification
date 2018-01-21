#ifndef _CPP_ART_QRCODE_COLOR_ART_QRCODE_PLAN_H_
#define _CPP_ART_QRCODE_COLOR_ART_QRCODE__PLAN_H_
#include <memory>
#include <string>
#include "ArtQRCode.h"
#include "ArtQRCodePlan.h"
#include <opencv2/opencv.hpp>

class ColorArtQRCodePlan
{
public:
	/**
	 * @param version 二维码版本号
	 * @param mask 二维码的掩码策略编号
	 * @param level 二维码的纠错等级
	 */    
    ColorArtQRCodePlan(int version,int mask, QRecLevel level);
    
    ~ColorArtQRCodePlan();

    /**
     * @brief 生成彩色艺术二维码图像
     * 
     * @param content 该二维码图像所包含的内容
     * @param image 该彩色艺术二维码拟合的图像。(BGR图像)
     * @param destQrWidth 所生成图像的目标宽度。如果为0，则与输入图像的宽度相同
     * @return 生成的彩色艺术二维码图像（注意，不边框）
     */
    cv::Mat createColorArtQRCode(const std::string & content, const cv::Mat &image, int destQrWidth = 0) const;

private:
	ArtQRCodePlan m_artPlan;
    int m_qrwidth;
};


#endif

