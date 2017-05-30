#ifndef SKYNETPLUS_SPROTO_PROTOCAL_ERROR_H
#define SKYNETPLUS_SPROTO_PROTOCAL_ERROR_H

//!错误的定义系统保留段0xF0000000-0xFFFFFFFF
#define SPROTO_ERRORID_SKYNETPLUS_Global_ParseError         0xF0000000          //传递包解析错误

#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_METHODIDERR      0xF0001001          //注册参数错误
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_KEYEXIST         0xF0001002          //key已经存在
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_CREATECTXERR     0xF0001003          //创建ctx失败
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_ERRORREGISTERKEY 0xF0001004          //不允许注册的key

#define SPROTO_ERRORID_SKYNETPLUS_GetNameToCtx_NoName       0xF0004001          //获取名字名字不存在

#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_RoadErr  0xF0005001          //到了错误的服务器
#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_NoSeria  0xF0005002          //没注册序列化函数
#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_SeriaErr 0xF0005003          //序列化失败

#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_NoServer       0xF1000001          //找不到服务器
#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ServerYieldErr 0xF1000002          //唤醒失败
#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ResponseErr    0xF1000003          //请求返回错误

#endif