#ifndef IP_UTIL_H__
#define IP_UTIL_H__

#include <windows.h>
#include <IPHlpApi.h>
#include <memory>

namespace iputil {

// �ж�һ��ip��ַ�Ƿ�ռ��
bool is_ip_available(const in_addr &dest);

// ��ȡ����ָ��ip���������ӿ�
std::shared_ptr<IP_ADAPTER_INFO> getAdapterInfo(const in_addr &ip, IP_ADDR_STRING *&pOutAddr);

// ��inaddr��ʼ������һ�����õ�ip
in_addr find_available_ip(in_addr inaddr);

// �滻ip
DWORD replace_ip(std::shared_ptr<IP_ADAPTER_INFO> &adapterInfo, const in_addr &oldIp, const in_addr &newIp);
DWORD replace_ip(DWORD adapterIndex, DWORD oldContext, const in_addr &newIp, const IPMask &newMask);
class ip_manager{
	
};
}

#endif