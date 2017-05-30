#ifndef SKYNETPLUS_SPROTO_PROTOCAL_DEFINE_H
#define SKYNETPLUS_SPROTO_PROTOCAL_DEFINE_H

/////////////////////////////////////////////////////////////////////////////////////////
/*method 定义*/
//类型定义
#define SPROTO_METHOD_TYPE_MASK			    0x80000000
#define SPROTO_METHOD_TYPE_RESPONSE		    0x80000000          //代表是否返回包
//域的划分
#define SPROTO_METHOD_FIELD_MASK		    0x7F000000
//系统的域(0x70000000 - 0x7F000000)
#define SPROTO_METHOD_FIELD_SKYNETCHILD     0x7E000000          //服务器传递域
#define SPROTO_METHOD_FIELD_SKYNET          0x7F000000          //服务器通信域
//用户自定义域
#define SPROTO_USERDEFINE_FIELD_MASK        0x00FF0000          //用户保留自定义域
//消息ID的定义
#define SPROTO_METHOD_ID_MASK               0x0000FFFF          //消息ID定义区

//!全局域消息定义

//!系统域SPROTO_METHOD_FIELD_SKYNETCHILD 消息定义
#define SPROTO_METHOD_SKYNETCHILD_Request           (SPROTO_METHOD_FIELD_SKYNETCHILD | 0x00000000)      //服务请求
#define SPROTO_METHOD_SKYNETCHILD_ServerRequest     (SPROTO_METHOD_FIELD_SKYNETCHILD | 0x00000001)      //服务请求返回

//!系统域SPROTO_METHOD_FIELD_SKYNET 消息定义
#define SPROTO_METHOD_SKYNET_Ping                   (SPROTO_METHOD_FIELD_SKYNET | 0x00000000)
#define SPROTO_METHOD_SKYNET_Register               (SPROTO_METHOD_FIELD_SKYNET | 0x00000001)           //注册
#define SPROTO_METHOD_SKYNET_CreateNameToCtxID      (SPROTO_METHOD_FIELD_SKYNET | 0x00000002)           //创建名字到id的对应
#define SPROTO_METHOD_SKYNET_DeleteNameToCtxID      (SPROTO_METHOD_FIELD_SKYNET | 0x00000003)           //删除名字到id的对应
#define SPROTO_METHOD_SKYNET_GetCtxIDByName         (SPROTO_METHOD_FIELD_SKYNET | 0x00000004)           //查找对应
/////////////////////////////////////////////////////////////////////////////////////////

#endif