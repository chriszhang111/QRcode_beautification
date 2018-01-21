#ifndef _CPP_ART_QRCODE_QRCODE_DATA_AND_ERR_H_
#define _CPP_ART_QRCODE_QRCODE_DATA_AND_ERR_H_

#include <vector>
#include "EncodeManager.h"
#include <string>
#include <utility>
#include "../qrencode/qrencode.h"

using uchar = unsigned char;

class QRCodeDataAndErr {
public:

    QRCodeDataAndErr(int version, QRecLevel level);

    
    bool addStringMode8Section(const std::string & s);

    
    int size();

    
    uchar get(unsigned int i);

    void setFitVector(const std::vector<std::pair<uchar, float>> & dataAndWeight);
private:
    void splitAndSort(); 
    void fitTarget();
    bool ecc(const std::vector<uchar> &origData,const std::pair<int, int> dataIndex, std::vector<uchar> &origErr, const std::pair<int, int> errIndex);
private:

    int m_version;                                      
    QRecLevel m_level;                                  
    std::vector<uchar> m_origData;                      
    std::vector<uchar> m_origErr;                       
    std::vector<uchar> m_sortedDataErr;                 
    std::vector<std::pair<int, int>> m_dataSectionIndex;
    std::vector<std::pair<int, int>> m_eccSectionIndex; 

    bool m_needFit;                                     
    std::vector<uchar> m_targetData;                    
    std::vector<float> m_dataWeigth;                    
    std::vector<uchar> m_targetErr;                     
    std::vector<float> m_errWeigth;                     
    std::vector<uchar> m_targetRemainder;               

    int m_dataAllCodes;                                 
    int m_errAllCodes;                                  
    int m_remainderLength;                              
    int m_blockNum1;                                    
    int m_blockNum2;                                    
    int m_dataCodes1;                                   
    int m_dataCodes2;                                   
    int m_eccCodes1;                                    
    int m_eccCodes2;                                    

    bool m_isNewData;                                   
    int m_actualDataSize;                               
    int m_deadDataSize;                                 
};

#endif

