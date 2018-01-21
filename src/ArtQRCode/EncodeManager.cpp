#include "EncodeManager.h"
extern "C"{
#include "../qrencode/qrspec.h"
}


EncodeManager::EncodeManager()
{
}


EncodeManager::~EncodeManager()
{
}


int EncodeManager::estimateAddLengthMode8Section(const std::string &s,int indicatorLength)
{
    return static_cast<int>(s.size() * 8 + indicatorLength + 4);  
}


int EncodeManager::addStringMode8Section(const std::string& s, int indicatorLength, std::vector<unsigned char>& code)
{

    int charSize = static_cast<int>(s.size());
    
    int addLength = 0;
    
    addLength = addLength + appendNum(code, 4, 0x04);
    
    addLength = addLength + appendNum(code, indicatorLength, charSize);
    
    addLength = addLength + appendByte(code, charSize,reinterpret_cast<const unsigned char*>(s.c_str()));
    return addLength;
}



int EncodeManager::addTerminationSign(int maxDataCodes, std::vector<unsigned char>& code)
{
    
    int tl = 0;
    if (maxDataCodes * 8 - code.size() >= 4)
    {
        tl = 4;
    }
    else
    {
        tl =static_cast<int>(maxDataCodes * 8 - code.size());
    }
    for (int i = 0; i < tl; ++i)
    {
        code.push_back(0);
    }
    return tl;
}



int EncodeManager::appendNum(std::vector<unsigned char> & s, int bits, unsigned int num)
{
    unsigned int mask;
    mask = 1 << (bits - 1);
    for (int i = 0; i < bits; i++) {
        if (num & mask) {
            s.push_back(1);
        }
        else {
            s.push_back(0);
        }
        mask = mask >> 1;
    }
    return bits;
}



int EncodeManager::appendByte(std::vector<unsigned char> & s, int size, const unsigned char *data)
{
    unsigned char mask;
    for (int i = 0; i < size; i++) {
        mask = 0x80; 
        for (int j = 0; j < 8; j++) {
            if (data[i] & mask) {
                s.push_back(1);
            }
            else {
                s.push_back(0);
            }
            mask = mask >> 1;
        }
    }
    return size*8;
}

