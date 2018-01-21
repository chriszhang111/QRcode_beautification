#include "ArtQRCodePlan.h"
#include <glog/logging.h>
#include "ArtQRCodeUtils.h"
#include "QRCodeCV.h"
#include "QRCodeDataAndErr.h"
#include "EncodeManager.h"

extern "C"
{
#include "../qrencode/qrspec.h"
#include "../qrencode/mask.h"
}

ArtQRCodePlan::ArtQRCodePlan(int version, int mask, QRecLevel level)
: m_version(version),
m_mask(mask),
m_level(level),
m_qrwidth(QRspec_getWidth(version)),
m_maskImage(static_cast<uchar*>(nullptr), free)
{
    DLOG_IF(ERROR, version < 1 || version > 40) << "version error!" << std::endl;
    DLOG_IF(ERROR, mask < 0 || mask > 7) << "mask error!" << std::endl;
    DLOG_IF(ERROR, level < QR_ECLEVEL_L || level > QR_ECLEVEL_H) << "level error!" << std::endl;
    initMaskImage();
    DLOG_IF(ERROR, m_maskImage.get() == nullptr) << "mask Image build error!" << std::endl;
    m_fillerPosition = getSequencePosition();
}


ArtQRCodePlan::~ArtQRCodePlan()
{
}


std::unique_ptr<ArtQRCode> ArtQRCodePlan::createArtQRCode(const std::string & content, const cv::Mat &tImg, const cv::Mat &tWgt) const
{
    if (tImg.rows != tImg.cols || tWgt.rows != tWgt.cols || tImg.rows != tWgt.rows || tImg.rows != m_qrwidth)
    {
        DLOG(INFO) << "ArtQRCodePlan::createArtQRCode info: the tImg and tWget have a wrong size!" << std::endl;
        return std::unique_ptr<ArtQRCode>();
    }

    QRCodeDataAndErr dataAndErr(m_version, m_level);
    dataAndErr.addStringMode8Section(content);

    auto target = getTargetSequence(tImg, tWgt);

    dataAndErr.setFitVector(target);

    auto size = dataAndErr.size();
    assert(size == m_fillerPosition.size());

    std::unique_ptr<ArtQRCode> qr(new  ArtQRCode(m_version, m_level, m_mask));
    qr->setData(m_maskImage.get(), m_qrwidth*m_qrwidth);
    std::shared_ptr<uchar> qrdata = qr->getData();
    uchar *p;
    for (auto i = 0; i < size; ++i)
    {
        p = qrdata.get() + m_fillerPosition[i].second * m_qrwidth + m_fillerPosition[i].first;
        *p = dataAndErr.get(i) ^ (*p);
    }
    return qr;
}


std::unique_ptr<ArtQRCode> ArtQRCodePlan::createGeneralQRCode(const std::string & content) const
{
    QRCodeDataAndErr dataAndErr(m_version, m_level);
    dataAndErr.addStringMode8Section(content);
    auto size = dataAndErr.size();

    std::unique_ptr<ArtQRCode> qr(new  ArtQRCode(m_version, m_level, m_mask));
    qr->setData(m_maskImage.get(), m_qrwidth*m_qrwidth);
    std::shared_ptr<uchar> qrdata = qr->getData();
    uchar *p;
    for (auto i = 0; i < size; ++i)
    {
        p = qrdata.get() + m_fillerPosition[i].second * m_qrwidth + m_fillerPosition[i].first;
        *p = dataAndErr.get(i) ^ (*p);
    }
    return qr;
}


void ArtQRCodePlan::initMaskImage()
{
    std::unique_ptr<uchar, decltype(free)*> frame(QRspec_newFrame(m_version), free);

    int size = m_qrwidth * m_qrwidth;
    uchar * p = frame.get();
    for (int i = 0; i < size; ++i, ++p) {
        if (!(*p & 0x80))
            *p = 0x02;
    }

    m_maskImage.reset(Mask_makeMask(m_qrwidth, frame.get(), m_mask, m_level));
}


std::vector<infoXY> ArtQRCodePlan::getSequencePosition() const
{
    std::vector<infoXY> ret;

    ret.reserve(m_qrwidth * m_qrwidth - 200);

    unsigned char *p;
    int x, y, w;

    p = m_maskImage.get();
    x = m_qrwidth - 2;
    y = m_qrwidth;
    w = m_qrwidth;
    int bit = 1;
    int dir = -1;

    while (true) {
        if (bit == 0) {
            x--;
            bit++;
        }
        else {
            x++;
            y += dir;
            bit--;
        }

        if (dir < 0) {
            if (y < 0) {
                y = 0;
                x -= 2;
                dir = 1;
                if (x == 6) {
                    x--;
                    y = 9;
                }
            }
        }
        else {
            if (y == w) {
                y = w - 1;
                x -= 2;
                dir = -1;
                if (x == 6) {
                    x--;
                    y -= 8;
                }
            }
        }
        if (x < 0 || y < 0)
            break;
        if (!(p[y * w + x] & 0x80)) {
            ret.push_back(std::make_pair(x, y));
        }
    }
    return ret;
}


std::vector<targetWeight> ArtQRCodePlan::getTargetSequence(const cv::Mat &img, const cv::Mat & weight) const
{
    assert(img.rows == img.cols && img.rows == m_qrwidth);
    assert(img.rows == weight.rows && img.cols == weight.cols);
    assert(m_maskImage.get() != nullptr);
    std::vector<targetWeight> dataAndErrTarget;
    dataAndErrTarget.reserve(m_fillerPosition.size());
    for (auto & i : m_fillerPosition) {
        dataAndErrTarget.push_back(
            std::make_pair(
            (img.at<uchar>(i.second, i.first) ^ 0x01) ^ (*(m_maskImage.get() + i.second*m_qrwidth + i.first) & 0x01),
            weight.at<float>(i.second, i.first)
            )
            );
    }
    return dataAndErrTarget;
}

