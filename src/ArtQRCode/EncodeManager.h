#ifndef _CPP_ART_QRCODE_ENCODE_MANAGER_H_
#define _CPP_ART_QRCODE_ENCODE_MANAGER_H_

#include <string>
#include <vector>

class EncodeManager
{
public:
	EncodeManager();
	~EncodeManager();

public:
	
	static int addStringMode8Section(const std::string& s, int indicatorLength, std::vector<unsigned char>& code);
	
	static int EncodeManager::estimateAddLengthMode8Section(const std::string &s, int indicatorLength);
	
	static int addTerminationSign(int maxDataCodes, std::vector<unsigned char>& code);

	

private:
	static int appendNum(std::vector<unsigned char> & s, int bits, unsigned int num);
	static int appendByte(std::vector<unsigned char> & s, int size, const unsigned char *data);
};

#endif

