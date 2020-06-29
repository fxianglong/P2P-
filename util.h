#pragma once//ͷ�ļ�������һ��
#ifdef _WIN32
//winͷ�ļ�
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<IPHlpApi.h>//��ȡ������Ϣ�ӿڵ�ͷ�ļ�
#include<vector>
#include<stdint.h>
#include<sstream>
#include<fstream>
#include<boost\filesystem.hpp>
#pragma comment(lib, "IPHlpApi.LIB")//��ȡ������Ϣ�ӿڵĿ��ļ�����
#pragma comment(lib,"ws2_32.lib")

#else
//Linuxͷ�ļ�

#endif

class FileUtil{
public:
	static int64_t GetFileSize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}

	static bool Write(const std::string &name, const std::string &body, int64_t offset=0){
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "ab");//�����Ʒ�ʽ���ļ�/�ı������Ĳ�ͬ
		if (fp == NULL){
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size()){
			std::cerr << "���ļ�д������ʧ��\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	//ָ�������ʾ����һ������Ͳ���
	//const& ��ʾ����һ�������Բ���
	//&��ʾ����һ����������Ͳ���
	static bool Read(const std::string &name, std::string *body){
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "��ȡ�ļ����ݣ�" << name << "size:" << filesize << std::endl;
		FILE *fp = NULL;
		fopen_s(&fp,name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "���ļ�ʧ��" << std::endl;
			return false;
		}
		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize){
			std::cerr << "���ļ�д������ʧ��" << std::endl;
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
			std::cerr << "���ļ�ʧ��" << std::endl;
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len){
			std::cerr << "���ļ�д������ʧ��" << std::endl;
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
	uint32_t _ip_addr;//�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ���������
};

class AdapterUtil{
#ifdef _WIN32
	//windows�µĻ�ȡ������Ϣʵ��
public:
	static bool GetAllAdapter(std::vector<Adapter>*list){
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//����һ��������Ϣ�ṹ�Ŀռ�
		//GetAdaptersInfo win�»�ȡ��������Ϣ�Ľ��--������Ϣ�п����ж������˴���һ��ָ��
		//��ȡ������Ϣ���ܻ�ʧ�ܣ���˿ռ䲻�㣬�����һ������Ͳ������������û����ص����ж�������
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);//���ڻ�ȡ����������Ϣ��ռ�еĴ�С
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW)//��ǰ������������������ռ䲻�㣬������¸�ָ������ռ�
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		}
		while (p_adapters!=nullptr){
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0){//��Ϊ��Щ����ϵ��û������������IP==0
				list->push_back(adapter);//��������Ϣ��ӵ�vector�з��ظ��û�
				//std::cout << "��������" << p_adapters->AdapterName << std::endl;
				//std::cout << "��������" << p_adapters->Description << std::endl;
				//std::cout << "IP��ַ" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				//std::cout << "��������" << p_adapters->IpAddressList.IpMask.String << std::endl;
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
