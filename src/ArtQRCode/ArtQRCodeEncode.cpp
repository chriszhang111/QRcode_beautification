#include "../qrencode/qrencode.h"
#include <glog/logging.h>
#include <opencv2/opencv.hpp>



typedef struct TargetImg
{
    TargetImg(const cv::Mat & m, const cv::Mat & w) :target(m), weight(w){};
    cv::Mat target;
    cv::Mat weight;
}TargetImg;


class ArtQRBitBlock
{
    
};


cv::Size getFitSizeToDest(const cv::Size& orign, int maxEdgeLength)
{
    cv::Size dst(maxEdgeLength, maxEdgeLength);
    if (orign.width > orign.height)
    {
        dst.height = orign.height * dst.width / orign.width;
    }
    else {
        dst.width = orign.width * dst.height / orign.height;
    }
    assert(std::max(dst.width, dst.height) == maxEdgeLength);
    return dst;
}



TargetImg getBinaryAndWeight(const cv::Mat &image, int maxEdgeLength, int deltaX, int deltaY)
{
    
    cv::Size graySize = getFitSizeToDest(image.size(), maxEdgeLength);
    cv::Mat grayImg;
    cv::resize(image, grayImg, graySize, 0, 0, CV_INTER_NN);
    if (image.channels() != 1)
    {
        cv::cvtColor(grayImg, grayImg, CV_RGB2GRAY, 1);
    }

    
    const int delta = 5;
    int tempPixel = 0;
    cv::Mat dstbinary(maxEdgeLength, maxEdgeLength,CV_8UC1);
    cv::Mat weight(maxEdgeLength, maxEdgeLength, CV_32FC1);
    for (auto j = 0; j < maxEdgeLength;++j) 
    {
        for (auto i = 0; i < maxEdgeLength; ++i)
        {
            
            int imgi = i + deltaX;
            int imgj = j + deltaY;
            if (imgi < 0 || imgi >= graySize.width || imgj < 0 || imgj >= graySize.height)
            {
                dstbinary.at<uchar>(j, i) = 255;
                weight.at<float>(j, i) = -1;
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
            dstbinary.at<uchar>(j, i) = grayImg.at<uchar>(imgj, imgi) >= 128 ? 255 : 0;
            weight.at<float>(j, i) = sumsq/n - (sum*1.0f/n) * (sum*1.0f/n);
        }
    }
    return TargetImg(dstbinary, weight);
}



QRcode * artQRcodeCreate(std::string url, cv::Mat &image, int version, int mask, QRecLevel level, int deltaX, int deltaY)
{
    
    LOG_IF(ERROR, version<1 || version > 40) << "version error!" << std::endl;
    LOG_IF(ERROR, mask < 0 || mask > 7) << "mask error!" << std::endl;
    LOG_IF(ERROR, level < QR_ECLEVEL_L || level > QR_ECLEVEL_H) << "level error!" << std::endl;
    
    
    int qrwidth = version * 4 + 17;
    TargetImg targetImg = getBinaryAndWeight(image, qrwidth, deltaX, deltaY);
    
    


    
    
    
    return nullptr;
}





int main()
{
    cv::Mat m = cv::imread("202301.jpg");
    cv::imshow("m", m);
    cv::waitKey(0);
    TargetImg t = getBinaryAndWeight(m, 17 + 4 * 10, 0, 0);
    cv::imshow("t", t.target);
    cv::waitKey(0);
    return 0;
}



