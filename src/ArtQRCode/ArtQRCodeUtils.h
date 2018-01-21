#ifndef _CPP_ART_QRCODE_ART_QRCODE_UTILS_H_
#define _CPP_ART_QRCODE_ART_QRCODE_UTILS_H_

#include <opencv2/opencv.hpp>

typedef struct TargetImg
{
    TargetImg(const cv::Mat & m, const cv::Mat & w) :target(m), weight(w){};
    cv::Mat target;
    cv::Mat weight;
}TargetImg;



cv::Size getFitSizeToDst(const cv::Size& orign, int maxEdgeLength);



TargetImg getBinaryAndWeight(const cv::Mat &image, int maxEdgeLength, int deltaX, int deltaY);


#endif

