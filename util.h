#pragma once//头文件仅包含一次
#ifdef _WIN32
//win头文件
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<IPHlpApi.h>//获取网卡信息接口的头文件
#include<vector>
#include<stdint.h>
#include<sstream>
#include<fstream>
#include<boost\filesystem.hpp>
#pragma comment(lib, "IPHlpApi.LIB")//获取网卡信息接口的库文件包含
#pragma comment(lib,"ws2_32.lib")

#else
//Linux头文件

#endif

class FileUtil{
public:
	static int64_t GetFileSize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}

	static bool Write(const std::string &name, const std::string &body, int64_t offset=0){
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "ab");//二进制方式打开文件/文本操作的不同
		if (fp == NULL){
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size()){
			std::cerr << "向文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	//指针参数表示这是一个输出型参数
	//const& 表示这是一个输入性参数
	//&表示这是一个输入输出型参数
	static bool Read(const std::string &name, std::string *body){
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据：" << name << "size:" << filesize << std::endl;
		FILE *fp = NULL;
		fopen_s(&fp,name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "打开文件失败" << std::endl;
			return false;
		}
		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize){
			std::cerr << "向文件写入数据失败" << std::endl;
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	static bool ReadRange(const std::string &name, std::string *body, int64_t len, int64_t offset=0){
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "打开文件失败" << std::endl;
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len){
			std::cerr << "向文件写入数据失败" << std::endl;
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	static bool GetRange(const std::string& range_str, int* start, int* end)
	{
		size_t  pos1 = range_str.find('-');
		size_t pos2 = range_str.find('=');
		*start = std::atol(range_str.substr(pos2 + 1, pos1 - pos2 - 1).c_str());
		std::cout << "range_str.substr(pos1 + 1, pos1 - pos2 - 1):" << range_str.substr(pos1 + 1, pos1 - pos2 - 1) << std::endl;
		*end = std::atol(range_str.substr(pos1 + 1).c_str());
		std::cout << "range_str.substr(pos1 + 1):" << range_str.substr(pos1 + 1) << std::endl;
		return true;
	}
};
class Adapter{
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

class AdapterUtil{
#ifdef _WIN32
	//windows下的获取网卡信息实现
public:
	static bool GetAllAdapter(std::vector<Adapter>*list){
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//开辟一块网卡信息结构的空间
		//GetAdaptersInfo win下获取网卡及信息的借口--网卡信息有可能有多个，因此传入一个指针
		//获取网卡信息可能会失败，因此空间不足，因此有一个输出型参数，用于向用户返回到底有多少网卡
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);//用于获取所有网卡信息所占有的大小
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW)//当前缓冲区溢出，缓冲区空间不足，因此重新给指针申请空间
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		}
		while (p_adapters!=nullptr){
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0){//因为有些网卡系并没有启动，导致IP==0
				list->push_back(adapter);//将网卡信息添加到vector中返回给用户
				//std::cout << "网卡名称" << p_adapters->AdapterName << std::endl;
				//std::cout << "网卡描述" << p_adapters->Description << std::endl;
				//std::cout << "IP地址" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				//std::cout << "子网掩码" << p_adapters->IpAddressList.IpMask.String << std::endl;
				//std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}

#else
	bool GetAllAdapter(std::vector<Adapter>*list)
	{
		return true;
	}
#endif
};
