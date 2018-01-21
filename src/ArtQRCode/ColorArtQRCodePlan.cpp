#include "ColorArtQRCodePlan.h"
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include "../CmLib.h"
#include "QRCodeCV.h"
#include "ArtQRCodeUtils.h"

ColorArtQRCodePlan::ColorArtQRCodePlan(int version, int mask, QRecLevel level)
: m_artPlan(version, mask, level), m_qrwidth(version * 4 + 17)
{
}

ColorArtQRCodePlan::~ColorArtQRCodePlan()
{
}




cv::Mat ColorArtQRCodePlan::createColorArtQRCode(const std::string & content, const cv::Mat &image, int destWidth ) const
{
    
    int deltaColor = 90;
    
    int top = 128 + deltaColor;
    int bottom = 128 - deltaColor;

    if (image.cols != image.rows)
    {
        return cv::Mat();
    }

    if (destWidth <= 0)
    {
        destWidth = image.rows;
    }

    int m = destWidth / m_qrwidth; 
    m = m % 2 == 0 ? m + 1 : m;         
    int midWidth = m_qrwidth * m;

    cv::Mat midImg;
    cv::resize(image, midImg, cv::Size(midWidth, midWidth), 0.0, 0.0, cv::INTER_AREA);
    int nChannels = midImg.channels();
    cv::Mat grayImg;
    
    if (nChannels == 3)
    {
        cv::cvtColor(midImg, grayImg, CV_BGR2GRAY, 1);
    }
    else if (nChannels == 1)
    {
        grayImg = midImg;
    }
    else
    {
        return cv::Mat();
    }

    Mat gauKernel = cv::getGaussianKernel(m, (m - 1) / 6);
    gauKernel = gauKernel*gauKernel.t(); 
    
    cv::Mat dstBinary(m_qrwidth, m_qrwidth, CV_8UC1);
    double sum = 0.0f;
    for (int j = 0; j < m_qrwidth; j++)
    {
        uchar * pdst = dstBinary.ptr<uchar>(j);
        uchar * pgray = grayImg.ptr<uchar>(j*m); 
        uchar * pgtemp = nullptr;
        for (int i = 0; i < m_qrwidth; i++)
        {
            sum = 0.0f;
            pgtemp = pgray + m*i; 
            for (int tl = 0; tl < m; ++tl)
            {
                double * pgk = gauKernel.ptr<double>(tl); 
                for (int tw = 0; tw < m; ++tw)
                {
                    sum += pgk[tw] * pgtemp[tw];
                }
                pgtemp = pgtemp + grayImg.step;      
            }
            pdst[i] = sum >= 128 ? 1 : 0;
        }
    }

    Mat sal, img3f;
    midImg.convertTo(img3f, CV_32FC3, 1.0 / 255);
    sal = CmSaliencyRC::GetRC(img3f);

    cv::Mat edges;
    cv::Canny(grayImg, edges, 100, 150);
    
    edges.convertTo(edges, CV_32FC1, 1.0 / 255);
    
    sal = 0.75*sal + 0.25*edges;
    
    Mat dstWeight(m_qrwidth, m_qrwidth, CV_32FC1, cv::Scalar(0.0f));
    
    for (decltype(m_qrwidth) j = 0; j < m_qrwidth; ++j) 
    {
        float * pwgt = dstWeight.ptr<float>(j);
        for (int tl = 0; tl < m; ++tl)
        {
            float * psal = sal.ptr<float>(j*m + tl);
            int ind = 0;
            for (decltype(m_qrwidth) i = 0; i < m_qrwidth; ++i)
            {
                for (int tw = 0; tw < m; ++tw)
                {
                    pwgt[i] += psal[ind++];
                }
            }
        }
    }
    
    std::unique_ptr<ArtQRCode> artqr = m_artPlan.createArtQRCode(content, dstBinary, dstWeight);
    assert(artqr->getWidth() == m_qrwidth);
    uchar* qr = artqr->getData().get();

#ifdef _DEBUG
    QRCodeCV cv(artqr.get());
    cv::Mat myqr = cv.getImage();
    cv::imwrite("blockQR.bmp", myqr);
#endif

    double kwg2all = 0.0;
    for (int tl = 0; tl < m; ++tl)
    {
        double * pgk = gauKernel.ptr<double>(tl);
        for (int tw = 0; tw < m; ++tw)
        {
            kwg2all += pgk[tw] * pgk[tw];
        }
    }
    kwg2all = 1.0 / kwg2all;

    
    uchar *pqr = qr;
    for (decltype(m_qrwidth) j = 0; j < m_qrwidth; ++j) 
    {
        cv::Vec3b * pmidImg = midImg.ptr<cv::Vec3b>(j * m); 
        uchar * pgray = grayImg.ptr<uchar>(j * m);          
        cv::Vec3b * ptemp = nullptr;
        uchar * pgtemp = nullptr;
        for (decltype(m_qrwidth) i = 0; i < m_qrwidth; ++i)
        {
            if ((*pqr & 0x60) != 0) 
            {
                ptemp = pmidImg + m*i;      
                for (int tl = 0; tl < m; ++tl)
                {
                    for (int tw = 0; tw < m; ++tw)
                    {
                        ptemp[tw][0] = ptemp[tw][1] = ptemp[tw][2] = ((*pqr & 0x01) ^ 0x01) * 255;
                    }
                    ptemp = (cv::Vec3b*)((uchar*)ptemp + midImg.step);
                }
            }
            else
            {
                pgtemp = pgray + m*i;   
                double sum = 0.0f;
                for (int tl = 0; tl < m; ++tl)
                {
                    double * pgk = gauKernel.ptr<double>(tl);
                    for (int tw = 0; tw < m; ++tw)
                    {
                        sum += pgk[tw] * pgtemp[tw];
                    }
                    pgtemp = pgtemp + grayImg.step;
                }

                
                if ((*pqr & 0x01) != 0) 
                {
                    if (sum > bottom)
                    {
                        ptemp = pmidImg + m*i;      
                        double delta = bottom - sum;
                        for (int tl = 0; tl < m; ++tl)
                        {
                            double * pgk = gauKernel.ptr<double>(tl);
                            for (int tw = 0; tw < m; ++tw)
                            {
                                ptemp[tw][0] = saturate_cast<uchar>(cvRound(ptemp[tw][0] + pgk[tw] * delta * kwg2all));
                                ptemp[tw][1] = saturate_cast<uchar>(cvRound(ptemp[tw][1] + pgk[tw] * delta * kwg2all));
                                ptemp[tw][2] = saturate_cast<uchar>(cvRound(ptemp[tw][2] + pgk[tw] * delta * kwg2all));
                            }
                            ptemp = (cv::Vec3b*)((uchar*)ptemp + midImg.step);
                        }
                    }
                }
                else
                {
                    if (sum < top)
                    {
                        ptemp = pmidImg + m*i;      
                        double delta = top - sum;
                        for (int tl = 0; tl < m; ++tl)
                        {
                            double * pgk = gauKernel.ptr<double>(tl);
                            for (int tw = 0; tw < m; ++tw)
                            {
                                ptemp[tw][0] = saturate_cast<uchar>(cvRound(ptemp[tw][0] + pgk[tw] * delta * kwg2all));
                                ptemp[tw][1] = saturate_cast<uchar>(cvRound(ptemp[tw][1] + pgk[tw] * delta * kwg2all));
                                ptemp[tw][2] = saturate_cast<uchar>(cvRound(ptemp[tw][2] + pgk[tw] * delta * kwg2all));
                            }
                            ptemp = (cv::Vec3b*)((uchar*)ptemp + midImg.step);
                        }
                    }
                }
            }
            ++pqr;
        }
    }

    
    int first = m / 2;
    pqr = qr;
    for (decltype(m_qrwidth) j = 0; j < m_qrwidth; ++j) 
    {
        cv::Vec3b * pmidImg = midImg.ptr<cv::Vec3b>(j*m + first);
        int x = first;  
        for (decltype(m_qrwidth) i = 0; i < m_qrwidth; ++i)
        {
            if ((*pqr & 0x60) == 0) 
            {
                if ((*pqr & 0x01) != 0) 
                {
                    pmidImg[x][0] = pmidImg[x][0] > bottom ? bottom : pmidImg[x][0];
                    pmidImg[x][1] = pmidImg[x][1] > bottom ? bottom : pmidImg[x][1];
                    pmidImg[x][2] = pmidImg[x][2] > bottom ? bottom : pmidImg[x][2];
                }
                else  
                {
                    
                    pmidImg[x][0] = pmidImg[x][0] < top ? top : pmidImg[x][0];
                    pmidImg[x][1] = pmidImg[x][1] < top ? top : pmidImg[x][1];
                    pmidImg[x][2] = pmidImg[x][2] < top ? top : pmidImg[x][2];
                }
            }
            x += m;
            ++pqr;
        }
    }

    cv::resize(midImg, midImg, cv::Size(destWidth, destWidth));
    return midImg;
}

