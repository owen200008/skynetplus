#ifndef SKYNETPLUS_COROUTINESTACKDATA_H
#define SKYNETPLUS_COROUTINESTACKDATA_H

#include <basic.h>
#include "../../kernel_head.h"
#include "../../../skynetplusprotocal/skynetplusprotocal_define.h"

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
	const char* GetData(){
		if(m_bSelf){
			return m_szBuf;
		}
		return m_smBuf.GetDataBuffer();
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
template<class SprotoStruct, class SprotoStruct2>
bool InsToSprotoStructDefaultStack(SprotoStruct& data, CCorutineStackDataDefault*& pDefaultStack, SprotoStruct2& response) {
	int nStackDataLength = 0;
	char* pStackData = (char*)pDefaultStack->GetData(nStackDataLength);
	//这边赋值为nullptr，因为这个内部的数据结构以及被改变不能再使用
	pDefaultStack = nullptr;
	basiclib::CBasicBitstream os;
	os.BindOutData(pStackData, nStackDataLength);
	os >> data;
	if (os.IsReadError()) {
		os.ResetReadError();
		response.m_head.m_nSession = data.m_head.m_nSession;
		return false;
	}
	response.m_head.m_nSession = data.m_head.m_nSession;
#ifdef _DEBUG
	ASSERT(response.m_head.m_nMethod == (data.m_head.m_nMethod | SPROTO_METHOD_TYPE_RESPONSE));
#endif
	return true;
}
template<class SprotoStruct>
bool InsToSprotoStructOnly(SprotoStruct& data, CCorutineStackDataDefault*& pDefaultStack) {
	int nStackDataLength = 0;
	char* pStackData = (char*)pDefaultStack->GetData(nStackDataLength);
	//这边赋值为nullptr，因为这个内部的数据结构以及被改变不能再使用
	pDefaultStack = nullptr;
	basiclib::CBasicBitstream os;
	os.BindOutData(pStackData, nStackDataLength);
	os >> data;
	if (os.IsReadError()) {
		os.ResetReadError();
		return false;
	}
	return true;
}

template<class SprotoStruct>
bool InsToSprotoStructSMBuffer(SprotoStruct& data, basiclib::CBasicBitstream& os) {
	os >> data;
	if (os.IsReadError()) {
		os.ResetReadError();
		return false;
	}
	return true;
}
template<class SprotoStruct>
void AddToInsWithSprotoStruct(SprotoStruct& data, basiclib::CBasicBitstream& ins) {
	ins.SetDataLength(0);
	ins << data;
}
template<class SendSession, class SprotoStruct>
void SendWithSprotoStruct(SendSession& pSession, SprotoStruct& data, basiclib::CBasicBitstream& ins) {
	ins.SetDataLength(0);
	ins << data;
	pSession->Send(ins);
}

_SKYNET_KERNEL_DLL_API Net_UInt GetDefaultStackMethod(CCorutineStackDataDefault* pDefaultStack);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//预分配对象
template<class T>
class CPreCreateObjectMgr : public basiclib::CBasicObject{
public:
	CPreCreateObjectMgr(int nDefaultSize = 128){
		m_nNextCreateCount = nDefaultSize;
		m_vtGetObject.reserve(nDefaultSize);
	}
	virtual ~CPreCreateObjectMgr(){
		for(auto& data : m_vtCreate){
			delete[] data;
		}
	}
	T* GetCreateObject(){
		int nSize = m_vtGetObject.size();
		if(nSize > 0){
			T* pRet = m_vtGetObject[nSize - 1];
			m_vtGetObject.pop_back();
			return pRet;
		}
		CreateObject();
		return GetCreateObject();
	}
protected:
	void CreateObject(){
		T* pRet = new T[m_nNextCreateCount];
		m_vtCreate.push_back(pRet);
		for(int i = 0; i < m_nNextCreateCount; i++){
			m_vtGetObject.push_back(&pRet[i]);
		}
		m_nNextCreateCount *= 2;
	}
protected:
	basiclib::basic_vector<T*>	m_vtGetObject;
	basiclib::basic_vector<T*>	m_vtCreate;
	uint32_t					m_nNextCreateCount;
};

#endif