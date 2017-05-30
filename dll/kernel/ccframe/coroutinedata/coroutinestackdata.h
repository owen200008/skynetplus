#ifndef SKYNETPLUS_COROUTINESTACKDATA_H
#define SKYNETPLUS_COROUTINESTACKDATA_H

#include <basic.h>

template<size_t defaultSize = 4096>
class CCorutineStackData : public basiclib::CBasicObject
{
public:
    CCorutineStackData(){
        m_bSelf = true;
        m_nLength = 0;
    }
    virtual ~CCorutineStackData(){
    }
    void CopyData(const char* pData, int nLength){
        if (nLength > defaultSize){
            m_bSelf = false;
            m_smBuf.SetDataLength(0);
            m_smBuf.AppendData(pData, nLength);
        }
        else{
            m_nLength = nLength;
            memset(m_szBuf, 0, defaultSize);
            memcpy(m_szBuf, pData, nLength);
        }
    }
    const char* GetData(int& nLength){
        if (m_bSelf){
            nLength = m_nLength;
            return m_szBuf;
        }
        nLength = m_smBuf.GetDataLength();
        return m_smBuf.GetDataBuffer();
    }
protected:
    char                        m_szBuf[defaultSize];
    int                         m_nLength;
    basiclib::CBasicSmartBuffer m_smBuf;
    bool                        m_bSelf;
};
typedef CCorutineStackData<16384>   CCorutineStackDataDefault;
/////////////////////////////////////////////////////////////////////////////////////////////////////


#endif