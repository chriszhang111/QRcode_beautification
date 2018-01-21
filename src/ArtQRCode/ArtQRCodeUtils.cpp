#include "ArtQRCodeUtils.h"
#include "../qrencode/qrencode.h"
#include <glog/logging.h>


cv::Size getFitSizeToDst(const cv::Size& orign, int maxEdgeLength)
{
    cv::Size dst(maxEdgeLength, maxEdgeLength);
    if (orign.width > orign.height)
    {
        dst.height = cvRound((0.0 + orign.height * dst.width) / orign.width);
    }
    else {
        dst.width = cvRound((0.0 + orign.width * dst.height) / orign.height);
    }
    assert(std::max(dst.width, dst.height) == maxEdgeLength);
    return dst;
}


TargetImg getBinaryAndWeight(const cv::Mat &image, int maxEdgeLength, int deltaX, int deltaY)
{
    
    cv::Size graySize = getFitSizeToDst(image.size(), maxEdgeLength);
    cv::Mat grayImg;
    cv::resize(image, grayImg, graySize, 0, 0, cv::INTER_NEAREST);
    if (image.channels() != 1)
    {
        cv::cvtColor(grayImg, grayImg, CV_BGR2GRAY, 1);
    }

    
    const int delta = 5;
    int tempPixel = 0;
    cv::Mat dstbinary(maxEdgeLength, maxEdgeLength, CV_8UC1);
    cv::Mat weight(maxEdgeLength, maxEdgeLength, CV_32FC1);
    for (auto j = 0; j < maxEdgeLength; ++j) 
    {
        uchar* pdst = dstbinary.ptr<uchar>(j);
        float* pwgt = weight.ptr<float>(j);
        for (auto i = 0; i < maxEdgeLength; ++i)
        {
            
            int imgi = i + deltaX;
            int imgj = j + deltaY;
            if (imgi < 0 || imgi >= graySize.width || imgj < 0 || imgj >= graySize.height)
            {
                pdst[i] = 1;
                pwgt[i] = -1.f;
                continue;
            }

            int n = 0;          
            int sum = 0;        
            float sumsq = 0;    
            
            
            for (int dy = -delta; dy <= delta; ++dy)
            {
                for (int dx = -delta; dx <= delta; ++dx)
                {
                    if (0 <= imgi + dx && imgi + dx < graySize.width && 0 <= imgj + dy && imgj + dy < graySize.height)
                    {
                        tempPixel = grayImg.at<uchar>(imgj + dy, imgi + dx);
                        ++n;
                        sum += tempPixel;
                        sumsq += tempPixel*tempPixel;
                    }
                }
            }
            pdst[i] = grayImg.at<uchar>(imgj, imgi) >= 128 ? 1 : 0;
            
            
            
            pwgt[i] = sumsq / n + (sum*1.0f / n) * (sum*1.0f / n); 
        }
    }
    return TargetImg(dstbinary, weight);
}

