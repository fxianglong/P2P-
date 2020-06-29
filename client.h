#pragma once
#include"util.h"
#include<thread>
#include<boost\filesystem.hpp>
#include<httplib.h>
#include<string>
#define P2P_PORT 29999//�˿�
#define MAX_IPBUFFER  16
#define MAX_RANGE (100*1024*1024)	//100m
#define SHARED_PATH "./Shared/"

#define DOWNLOAD_PATH "./Download/"

class StringUtil{
public:
	static int64_t Str2Dig(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;
		return res;
	}
};

class Host{
public:
	uint32_t _ip_addr;
	bool _pair_ret;//���ڴ����Խ��
};
class Client{
public:
	bool Start()
	{
		//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ���ֻ��һ��
		//ѭ�����У�ÿ������һ���ļ�֮��ͻ���������ƥ�������������ǲ������
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
	//������Ե��߳���ں���
	void HostPair(Host* host){
		//1.��֯httpЭ���ʽ����������
		//2.�һ��tcp�ͻ��ˣ������ݷ���
		//3.�ȴ��������Ļظ��������н���
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);//ʵ����httplib�ͻ��˶���
		auto rsp = cli.Get("/hostpair");//�����˷�����ԴΪ/hostpair��Get����//�����ӽ���ʧ�ܻ᷵��NULL
		if (rsp && rsp->status == 200){//�ж���Ӧ����Ƿ���ȷ
			host->_pair_ret = true;//����������Խ��
		}
		return;
	}
	bool GetOnlineHost()//��ȡ��������
	{
		char ch='Y';//�Ƿ�����ƥ�䣬Ĭ���ǽ���ƥ�䣬���Ѿ�ƥ�䣬online������Ϊ�������û�ѡ��
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ����²鿴����������Y/N��";
			fflush(stdout);
			std::cin >> ch;
		}
		else
		{
			ch = 'Y';
		}
		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ�䡭��\n";
			//1.��ȡ������Ϣ�������õ������������е�IP��ַ�б�
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); i++){
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				uint32_t net = ntohl(ip&mask);
				//�������������
				uint32_t max_host = (~ntohl(mask));
				for (int j = 1; j < (int32_t)max_host; j++)
				{
					uint32_t host_ip = htonl(net + j);//�������IP�ļ���Ӧ��ʹ�������ֽ��������ź�������
					Host host;
					host._ip_addr = host_ip;
					host._pair_ret = false;
					host_list.push_back(host);

				}
			}
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); i++)
			{
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "��������ƥ���У����Ժ󡭡�\n";
			for (int i = 0; i < host_list.size(); i++){
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true){
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}

		//����������IP��ӡ���������û�ѡ��
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//3.���������õ���Ӧ���������λ������������IP��ӵ�_online_host�б���
		//4.��ӡ���������б�ʹ�û�ѡ��
		std::cout << "��ѡ�������������ȡ�ļ������б�";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		//select_ip->Host
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			if (buf == select_ip){
				//�û�ѡ������IP��ַ
				GetShareList(select_ip);//�û�ѡ������IP��ַ
				return true;
			}
		}
		std::cout << "����IP��ַ����" << std::endl;
		return false;
	}
	bool GetShareList(std::string &host_ip){
		//��ȡ�ļ��б�
		//�����˷���һ���ļ���ȡ����
		//1.��������
		//2.�õ���Ӧ֮�󣬽������ģ��ļ����ƣ�
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cout << rsp->status << std::endl;
			std::cerr << "��ȡ�ļ��б���Ӧ����\n";
			return false;
		}
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ���";
		fflush(stdout);
		std::string filename;
		while(1){
			std::cin >> filename;
			if (rsp->body.find(filename) == std::string::npos)
			{
				std::cout << "���벻��ȷ������������" << std::endl;
				continue;
			}
			else
			{
				RangeDownload1(host_ip, filename);
				return true;
			}
		}
	}
	bool DownloadFile(const std::string &host_ip,std::string &filename){
		//�����ļ�
		//1.�����˷����ļ���������
		//2.�õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�����д���ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;
		std::string realpath = DOWNLOAD_PATH + filename;
		while (boost::filesystem::exists(realpath)){//�ж��ļ��Ƿ�����
			static int i = 0;
			//2.��ȡ������׺���ļ���
			std::string name = filename.substr(0, filename.rfind("."));
			std::cout << name << std::endl;

			name += std::to_string(i);
			i++;

			//3.��ȡ��׺��
			std::string suffix_str = filename.substr(filename.find_last_of('.') + 1);
			std::cout << suffix_str << std::endl;

			filename = name + "."+ suffix_str;
			realpath = DOWNLOAD_PATH + filename;
		}
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "�����ļ�����ȡ��Ӧ��Ϣʧ��\n";
			return false;
		}
		if (!boost::filesystem::exists(DOWNLOAD_PATH)){
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		if (FileUtil::Write(realpath, rsp->body) == false){
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		std::cout << "�ļ����سɹ�" << std::endl;
		return true;
	}

	//�����ļ�����ʱʹ�÷ֿ鴫��

	bool RangeDownload2(const std::string &host_ip, std::string &name, int64_t s, int64_t e){
		std::string req_path = "/download/" + name;
		std::string realpath = DOWNLOAD_PATH + name;
		while (boost::filesystem::exists(realpath)){//�ж��ļ��Ƿ����
			static int i = 0;
			//2.��ȡ������׺���ļ���
			std::string name = name.substr(0, name.rfind("."));
			std::cout << name << std::endl;

			name += std::to_string(i);
			i++;


			//3.��ȡ��׺��
			std::string suffix_str = name.substr(name.find_last_of('.') + 1);
			std::cout << suffix_str << std::endl;

			name = name + "." + suffix_str;
			realpath = DOWNLOAD_PATH + name;
		}
		//httplib::Headers header;
		//header = httplib::make_range_header({{s, e}});
		//header.insert(httplib::make_range_header({ {s, e} }));//����һ��range����
		std::stringstream tmp;
		tmp << "byte=" << s << "-" << e;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		httplib::Headers header;
		header.insert(std::make_pair("Range", tmp.str()));
		auto rsp = cli.Get(req_path.c_str(), header);//�����˷���һ���ֶ�����
		if (rsp == NULL || rsp->status != 206) {
			if (rsp == NULL) { std::cout << "client rsp status NULL\n"; }
			else { std::cout << "��Ӧ״̬��" << rsp->status << "\n"; }
			std::cout << "range download failed\n";
			return false;
		}
		FileUtil::Write(realpath, rsp->body, s);
		std::cout << "client range write success\n";
		return true;
	}
	int64_t getfilesize(const std::string &host_ip, const std::string &req_path) {
		//1. ����HEAD����ͨ����Ӧ�е�Content-Length��ȡ�ļ���С
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200) {
			std::cout << "��ȡ�ļ���С��Ϣʧ��\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::Str2Dig(clen);
		return filesize;
	}
	bool RangeDownload1(const std::string &host_ip,  std::string &name) {
		std::string req_path = "/download/" + name;
		int64_t filesize = getfilesize(host_ip, req_path);
		//2. �����ļ���С���зֿ�
		//int range_count = filesize / MAX_RANGE;
		//a. ���ļ���СС�ڿ��С����ֱ�������ļ�
		if (filesize < MAX_RANGE) {
			std::cout << "�ļ���С,ֱ�������ļ�\n";
			return DownloadFile(host_ip, name);
		}
		//����ֿ����
		//b. ���ļ���С�����������С����ֿ����λ�ļ���С���Էֿ��СȻ��+1
		//c. ���ļ���С�պ��������С����ֿ���������ļ���С���Էֿ��С
		std::cout << "too max, range download\n";
		int range_count = 0;
		if ((filesize % MAX_RANGE) == 0) {
			range_count = filesize / MAX_RANGE;
		}
		else {
			range_count = (filesize / MAX_RANGE) + 1;
		}
		// 136   100    0~99  100~135
		int64_t range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++) {
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1)) {
				range_end = filesize - 1;
			}
			else {
				range_end = ((i + 1) * MAX_RANGE) - 1;
			}
			std::cout << "client range req:" << range_start << "-" << range_end << std::endl;
			//3. ��һ����ֿ��������ݣ��õ���Ӧ֮��д���ļ���ָ��λ��
			RangeDownload2(host_ip, name, range_start, range_end);
		}
		std::cout << "���سɹ�\n";
		return true;
	}

private:
	std::vector<Host> _online_host;
};