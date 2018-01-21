#include "CmLib.h"
#include "ArtQRCode/ColorArtQRCodePlan.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <cassert>

using str = std::string;
int main()
{    
    str s("Beautiful QRCode");
    int version = 5;
    int mask = 1;
    cv::Mat qrimg = cv::imread("Test.bmp",CV_LOAD_IMAGE_COLOR);

    //生成二维码图像
    ColorArtQRCodePlan plan(version, mask, QRecLevel::QR_ECLEVEL_L);
    cv::Mat qr = plan.createColorArtQRCode(s, qrimg, 0);

    //增加边框，展示并保存二维码图像
    int m = cvRound(1.0f * qr.cols / (version*4+17));
    cv::copyMakeBorder(qr, qr, 4 * m, 4 * m, 4 * m, 4 * m, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    cv::imshow("test", qr);
    cv::imwrite("result.bmp", qr);
    cv::waitKey(0);
    return 0;
}
