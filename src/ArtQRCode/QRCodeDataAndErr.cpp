#include "QRCodeDataAndErr.h"
#include <cassert>
#include <glog/logging.h>
#include <utility>
#include <memory>
#include <iostream>
#include <iomanip>
extern "C"
{
#include "../qrencode/qrspec.h"
#include "../qrencode/rscode.h"
}


QRCodeDataAndErr::QRCodeDataAndErr(int version, QRecLevel level)
	: m_version(version),
	  m_actualDataSize(0),
	  m_level(level),
	  m_needFit(false),
	  m_isNewData(true),
	  m_deadDataSize(0)
{
	
	int spec[5];
	QRspec_getEccSpec(m_version, m_level, spec);
	m_dataAllCodes    = QRspec_getDataLength(m_version, m_level);
	m_errAllCodes     = QRspec_getECCLength(m_version, m_level);
	m_remainderLength = QRspec_getRemainder(m_version);
	m_blockNum1       = QRspec_rsBlockNum1(spec);
	m_blockNum2       = QRspec_rsBlockNum2(spec);
	m_dataCodes1      = QRspec_rsDataCodes1(spec);
	m_dataCodes2      = QRspec_rsDataCodes2(spec);
	m_eccCodes1       = QRspec_rsEccCodes1(spec);
	m_eccCodes2       = QRspec_rsEccCodes2(spec);
	
	assert(m_dataAllCodes == (m_dataCodes1 * m_blockNum1 + m_dataCodes2 * m_blockNum2));
	assert(m_errAllCodes == (m_eccCodes1 * m_blockNum1 + m_eccCodes2 * m_blockNum2));

	
	m_origData.reserve(m_dataAllCodes * 8);
	m_origErr.reserve(m_errAllCodes * 8);

	
	m_dataSectionIndex.clear();
	m_dataSectionIndex.reserve(m_blockNum1 + m_blockNum2);
	int dataBaseIndex = 0;
	for (int i = 0; i < m_blockNum1; i++) 	
	{
		m_dataSectionIndex.push_back(std::make_pair(dataBaseIndex, dataBaseIndex + m_dataCodes1 * 8)); 
		dataBaseIndex += m_dataCodes1 * 8;
	}
	for (int i = 0; i < m_blockNum2; i++)	
	{
		m_dataSectionIndex.push_back(std::make_pair(dataBaseIndex, dataBaseIndex + m_dataCodes2 * 8));
		dataBaseIndex += m_dataCodes2 * 8;
	}
	m_eccSectionIndex.clear();
	m_eccSectionIndex.reserve(m_blockNum1 + m_blockNum2);
	int eccBaseIndex = 0;
	for (int i = 0; i < m_blockNum1; i++)	
	{
		m_eccSectionIndex.push_back(std::make_pair(eccBaseIndex, eccBaseIndex + m_eccCodes1 * 8)); 
		eccBaseIndex += m_eccCodes1 * 8;
	}
	for (int i = 0; i < m_blockNum2; i++)	
	{
		m_eccSectionIndex.push_back(std::make_pair(eccBaseIndex, eccBaseIndex + m_eccCodes2 * 8));
		eccBaseIndex += m_eccCodes2 * 8;
	}
}


bool QRCodeDataAndErr::addStringMode8Section(const std::string & s)
{
	int indicator = QRspec_lengthIndicator(QR_MODE_8, m_version);
	int estimateAddLength = EncodeManager::estimateAddLengthMode8Section(s, indicator);
	
	if ((estimateAddLength + m_actualDataSize) > m_dataAllCodes * 8)
	{
		DLOG(WARNING) << "编码数据字数超出限制:" << s << std::endl;
		return false;
	}
	else
	{
		
		if (m_isNewData)
		{
			auto tailsz = m_origData.size() - m_actualDataSize;
			for (decltype(tailsz) i = 0; i < tailsz; ++i)
			{
				m_origData.pop_back();
			}
			m_actualDataSize = m_actualDataSize +  EncodeManager::addStringMode8Section(s, indicator, m_origData);
		}
		else
		{
			
			m_actualDataSize = m_actualDataSize + EncodeManager::addStringMode8Section(s, indicator, m_origData);
		}
		m_isNewData = false;
		return true;
	}
}


int QRCodeDataAndErr::size()
{
	if (!m_isNewData)
	{
		splitAndSort();
		m_isNewData = true;
	}
	return static_cast<int>(m_sortedDataErr.size());
}


uchar QRCodeDataAndErr::get(unsigned int i)
{
	
	if (!m_isNewData)
	{
		splitAndSort();
		m_isNewData = true;
	}

	
	if (i >= m_sortedDataErr.size())
	{
		return 0;
	}
	return m_sortedDataErr[i];
}


void QRCodeDataAndErr::splitAndSort()
{
	
	EncodeManager::addTerminationSign(m_dataAllCodes, m_origData);
	m_deadDataSize = static_cast<int>(m_origData.size()); 
	
	for (int i = 0; i < m_dataAllCodes * 8 - m_deadDataSize; ++i)
	{
		m_origData.push_back(0);
	}
	DLOG(INFO) << "actual data:(" << m_actualDataSize << ") "
	          << "data after terminate:(" << m_deadDataSize << ") "
	          << "max data size:(" << m_dataAllCodes * 8 << ")" << std::endl;

	
	m_origErr.clear();
	for (int i = 0; i < m_blockNum1 + m_blockNum2; ++i)
	{
		ecc(m_origData, m_dataSectionIndex[i], m_origErr, m_eccSectionIndex[i]);
	}

	
	
	if (m_needFit)
	{
		
		fitTarget();
    }

	
	m_sortedDataErr.clear();
	if (m_blockNum2 == 0)	
	{
		for (int i = 0; i < m_dataCodes1; ++i)
		{
			for (int bn = 0; bn < m_blockNum1; ++bn)
			{
				for (int j = 0; j < 8; j++)
				{
					m_sortedDataErr.push_back(m_origData[m_dataSectionIndex[bn].first + i * 8 + j]);
				}
			}
		}
	}
	else
	{
		assert(m_dataCodes1 + 1 == m_dataCodes2);
		for (int i = 0; i < m_dataCodes1; ++i) 	
		{
			for (int bn = 0; bn < m_blockNum1 + m_blockNum2; ++bn)
			{
				for (int j = 0; j < 8; j++)
				{
					m_sortedDataErr.push_back(m_origData[m_dataSectionIndex[bn].first + i * 8 + j]);
				}
			}
		}
		

		for (int bn = m_blockNum1; bn < m_blockNum1 + m_blockNum2; ++bn)
		{
			for (int j = 0; j < 8; j++)
			{
				m_sortedDataErr.push_back(m_origData[m_dataSectionIndex[bn].first + m_dataCodes1 * 8 + j]);
			}
		}
	}

	
	assert(m_eccCodes1 == m_eccCodes2);
	for (int i = 0; i < m_eccCodes1; ++i)
	{
		for (int bn = 0; bn < m_blockNum1 + m_blockNum2; ++bn)
		{
			for (int j = 0; j < 8; j++)
			{
				m_sortedDataErr.push_back(m_origErr[m_eccSectionIndex[bn].first + i * 8 + j]);
			}
		}
	}

	
	if (m_needFit)
	{
		for (int i = 0; i < m_remainderLength; ++i)
		{
			m_sortedDataErr.push_back(m_targetRemainder[i]);
		}
	}
	else
	{
		for (int i = 0; i < m_remainderLength; ++i)
		{
			m_sortedDataErr.push_back(0);
		}
	}
}


bool QRCodeDataAndErr::ecc(
    const std::vector<uchar> &origData, 
    const std::pair<int, int> dataIndex, 
    std::vector<uchar> &origErr, 
    const std::pair<int, int> errIndex)
{
	assert((dataIndex.second - dataIndex.first) % 8 == 0);
	assert((errIndex.second - errIndex.first) % 8 == 0);
	assert(dataIndex.second >= dataIndex.first);
	assert(errIndex.second >= errIndex.first);
	assert(origData.size() >= dataIndex.second);
	if (origErr.size() < errIndex.second)
	{
		auto tsz = errIndex.second - origErr.size();
		for (decltype(tsz) i = 0; i < tsz; i++)
		{
			origErr.push_back(0);
		}
	}
	
	std::vector<uchar>::const_iterator p = origData.cbegin() + dataIndex.first;
	auto dataSize = dataIndex.second - dataIndex.first;
	auto dl = dataSize / 8;
	std::unique_ptr<uchar[]> data(new uchar[dl]);
	if (data.get() == nullptr) return false;
	for (decltype(dl) i = 0; i < dl; ++i)
	{
		uchar v = 0;
		for (int j = 0; j < 8; j++)
		{
			v <<= 1;
			v |= *p;
			p++;
		}
		data[i] = v;
	}
	
	int el = (errIndex.second - errIndex.first) / 8;
	std::unique_ptr<uchar[]> err(new uchar[el]);
	if (err.get() == nullptr) return false;
	RS*rs = init_rs(8, 0x11d, 0, 1, el, 255 - dl - el);
	if (rs == nullptr) return false;
	encode_rs_char(rs, data.get(), err.get());
	
	int tmpind = errIndex.first;
	for (int i = 0; i < el; i++)
	{
		uchar t = 0x80;
		for (int j = 0; j < 8; j++)
		{
			
			origErr[tmpind] = ((t & err[i]) != 0);
			tmpind++;
			t >>= 1;
		}
	}
	
	return true;
}


void QRCodeDataAndErr::setFitVector(const std::vector<std::pair<uchar, float>> & dataAndWeight)
{
	
	
	m_targetData.clear();
	m_targetData.reserve(m_dataAllCodes * 8);
	m_dataWeigth.clear();
	m_dataWeigth.reserve(m_dataAllCodes * 8);
	assert(dataAndWeight.size() == m_dataAllCodes * 8 + m_errAllCodes * 8 + m_remainderLength);
	assert(m_blockNum2 == 0 || (m_blockNum2 != 0 && m_dataCodes1 + 1 == m_dataCodes2));
	for (int bn = 0; bn < m_blockNum1; ++bn)
	{
		for (int i = 0; i < m_dataCodes1; ++i)
		{
			int indexdc = (i * (m_blockNum1 + m_blockNum2) + bn) * 8;
			for (int j = 0; j < 8; ++j)
			{
				m_targetData.push_back(dataAndWeight[indexdc + j].first);
				m_dataWeigth.push_back(dataAndWeight[indexdc + j].second);
			}
		}
	}
	for (int bn = m_blockNum1; bn < m_blockNum1 + m_blockNum2; ++bn) 
	{
		for (int i = 0; i < m_dataCodes1; ++i)
		{
			int indexdc = (i * (m_blockNum1 + m_blockNum2) + bn) * 8;
			for (int j = 0; j < 8; ++j)
			{
				m_targetData.push_back(dataAndWeight[indexdc + j].first);
				m_dataWeigth.push_back(dataAndWeight[indexdc + j].second);
			}
		}
		int indexdc = (m_dataCodes1 * (m_blockNum1 + m_blockNum2) + bn - m_blockNum1) * 8;
		for (int j = 0; j < 8; ++j)
		{
			m_targetData.push_back(dataAndWeight[indexdc + j].first);
			m_dataWeigth.push_back(dataAndWeight[indexdc + j].second);
		}
	}
	assert(m_targetData.size() == m_dataAllCodes * 8);
	assert(m_dataWeigth.size() == m_dataAllCodes * 8);

	
	m_targetErr.clear();
	m_targetErr.reserve(m_errAllCodes * 8);
	m_errWeigth.clear();
	m_errWeigth.reserve(m_errAllCodes * 8);
	assert(m_eccCodes1 == m_eccCodes2);
	for (int bn = 0; bn < m_blockNum1 + m_blockNum2; ++bn)
	{
		for (int i = 0; i < m_eccCodes1; ++i)
		{
			int indexdc = (i * (m_blockNum1 + m_blockNum2) + bn + m_dataAllCodes) * 8;
			for (int j = 0; j < 8; ++j)
			{
				m_targetErr.push_back(dataAndWeight[indexdc + j].first);
				m_errWeigth.push_back(dataAndWeight[indexdc + j].second);
			}
		}
	}
	assert(m_targetErr.size() == m_errAllCodes * 8);
	assert(m_errWeigth.size() == m_errAllCodes * 8);
	
	m_targetRemainder.clear();
	m_targetRemainder.reserve(m_remainderLength);
	for (int i = 0; i < m_remainderLength; ++i)
	{
		m_targetRemainder.push_back(dataAndWeight[(m_dataAllCodes + m_errAllCodes) * 8 + i].first);
	}

	
	m_needFit = true;
}



void QRCodeDataAndErr::fitTarget()
{
    
    
    for (int bn = 0; bn < m_blockNum1+m_blockNum2; ++bn)
    {
        
        
        int deadSize = 0;
        assert(m_dataSectionIndex[bn].second >= m_dataSectionIndex[bn].first);
        if (m_deadDataSize >= m_dataSectionIndex[bn].second)
        {
            
            
            continue;
        }
        else if (m_deadDataSize >= m_dataSectionIndex[bn].first)
        {
            deadSize = m_deadDataSize - m_dataSectionIndex[bn].first;
        }
        else
        {
            deadSize = 0;
        }

        
        std::vector<uchar>::const_iterator pd = m_origData.cbegin() + m_dataSectionIndex[bn].first;
        std::vector<uchar>::const_iterator ptd = m_targetData.cbegin() + m_dataSectionIndex[bn].first;
        auto dataSize = m_dataSectionIndex[bn].second - m_dataSectionIndex[bn].first;
        assert(dataSize % 8 == 0);
        auto dl = dataSize / 8;
        std::vector<uchar>::const_iterator pe = m_origErr.cbegin() + m_eccSectionIndex[bn].first;
        std::vector<uchar>::const_iterator pte = m_targetErr.cbegin() + m_eccSectionIndex[bn].first;
        auto errSize = m_eccSectionIndex[bn].second - m_eccSectionIndex[bn].first;
        assert(errSize % 8 == 0);
        auto el = errSize / 8;
        std::unique_ptr<uchar[]> dataErr(new uchar[dl + el]);
        std::unique_ptr<uchar[]> targetDataErr(new uchar[dl + el]);
        if (dataErr.get() == nullptr) return;
        if (targetDataErr.get() == nullptr) return;
        for (int i = 0; i < dl; ++i)   
        {
            uchar v = 0;
            uchar tv = 0;
            for (int j = 0; j < 8; j++)
            {
                v <<= 1;
                tv <<= 1;
                v |= *pd;
                tv |= *ptd;
                pd++;
                ptd++;
            }
            dataErr[i] = v;
            targetDataErr[i] = tv;
        }
        for (int i = dl; i < dl + el; ++i)   
        {
            uchar v = 0;
            uchar tv = 0;
            for (int j = 0; j < 8; j++)
            {
                v <<= 1;
                tv <<= 1;
                v |= *pe;
                tv |= *pte;
                pe++;
                pte++;
            }
            dataErr[i] = v;
            targetDataErr[i] = tv;
        }

        
        std::vector<std::pair<float, int>> weightWithIdx; 
        weightWithIdx.reserve(dataSize+errSize);
        for (decltype(dataSize) i = 0; i < dataSize; ++i)
        {
            weightWithIdx.push_back(std::make_pair(m_dataWeigth[m_dataSectionIndex[bn].first+i], i));
        }
        for (decltype(errSize) i = 0; i < errSize;++i)
        {
            weightWithIdx.push_back(std::make_pair(m_errWeigth[m_eccSectionIndex[bn].first+i], i+dataSize));
        }
        assert(deadSize <= dataSize);
        for (decltype(deadSize) i = 0; i < deadSize;++i)
        {
            weightWithIdx[i].first = std::numeric_limits<float>::max();
        }
        assert(weightWithIdx.size() == dataSize + errSize);
        qsort(&weightWithIdx[0], dataSize + errSize, sizeof(std::pair<float, int>), 
            [](const void *x,const void *y){
            return (((*(std::pair<float, int>*)x).first - (*(std::pair<float, int>*)y).first) > 0 ? -1 : 1);
        });
        
        
        
        std::vector<std::unique_ptr<uchar[]>> vbase;
        vbase.reserve(dataSize);
        for (int i = 0; i < dataSize;i++)
        {
            vbase.push_back(std::unique_ptr<uchar[]>(new uchar[dl + el]()));
        }
        assert(dl * 8 == dataSize);
        for (int i = 0; i < dl;i++)
        {
            int x = 0x80;
            for (int j = 0; j < 8;j++)
            {
                vbase[i * 8 + j][i] = x;
                x >>= 1;
            }
        }
        RS*rs = init_rs(8, 0x11d, 0, 1, el, 255 - dl - el);
        if (rs == nullptr) return;
        for (auto i = vbase.cbegin(); i < vbase.cend();i++)
        {
            encode_rs_char(rs, i->get(), i->get()+dl);
        }
        assert(vbase.size() == dataSize);

        
        int success = deadSize; 
        int bi = deadSize;      
        std::unique_ptr<uchar[]> temp;
        std::vector<int> historyIdx; 
        historyIdx.reserve(dataSize);
        for (int i = 0; i < success; i++) historyIdx.push_back(i);
        while (success < dataSize && bi < dataSize+errSize)
        {
            
            
            
            
            
            
            
            
            
            
            
            int colIdx = weightWithIdx[bi].second/8; 
            uchar vbiValue = (0x80 >> (weightWithIdx[bi].second%8));
            int searchIndex = -1;
            
            for (int rowIdx = success; rowIdx < dataSize; ++rowIdx)
            {
                if ((vbase[rowIdx][colIdx] & vbiValue) != 0)	
            	{
            		searchIndex = rowIdx;
            		break;
            	}
            }
            if (searchIndex!=-1) 
            {
                assert(vbase[searchIndex][colIdx] != 0);
                
                
                for (int rowIdx = 0; rowIdx < success; ++rowIdx)
                {
                    if ((vbase[rowIdx][colIdx] & vbiValue) != 0)	
                    {
                        for (int i = 0; i < dl + el; ++i)
                        {
                            vbase[rowIdx][i] ^= vbase[searchIndex][i];
                        }
                    }
                }
                for (int rowIdx = searchIndex + 1; rowIdx < dataSize; ++rowIdx)
                {
                    if ((vbase[rowIdx][colIdx] & vbiValue) != 0)	
                    {
                        for (int i = 0; i < dl + el; ++i)
                        {
                            vbase[rowIdx][i] ^= vbase[searchIndex][i];
                        }
                    }
                }
                
                vbase[success].swap(vbase[searchIndex]);
                
                historyIdx.push_back(weightWithIdx[bi].second);
                success++;
            }
            bi++;
        }

        
        
        assert(success == dataSize); 
        assert(historyIdx.size() == success);
        

        for (int i = deadSize; i < success; ++i) 
        {
            int j = historyIdx[i] / 8;
            uchar jv = (0x80 >> (historyIdx[i] % 8));
            assert( (vbase[i][j] & jv) != 0);
            if ( (dataErr[j] & jv) != (targetDataErr[j] & jv) ) 
            {
                for (int k = 0; k < dl + el; ++k)
                {
                    dataErr[k] ^= vbase[i][k];
                }
            }
            assert((dataErr[j] & jv) == (targetDataErr[j] & jv));
        }

        
        
        int tmpInd = m_dataSectionIndex[bn].first;
        for (int i = 0; i < dl; i++)
        {
            uchar t = 0x80;
            for (int j = 0; j < 8;j++)
            {
                m_origData[tmpInd] = ((t&dataErr[i]) != 0);
                tmpInd++;
                t >>= 1;
            }
        }
        tmpInd = m_eccSectionIndex[bn].first;
        for (int i = 0; i < el; i++)
        {
            uchar t = 0x80;
            for (int j = 0; j < 8; j++)
            {
                m_origErr[tmpInd] = ((t&dataErr[i + dl]) != 0);
                tmpInd++;
                t >>= 1;
            }
        }
    }
}

