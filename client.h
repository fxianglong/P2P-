#pragma once
#include"util.h"
#include<thread>
#include<boost\filesystem.hpp>
#include<httplib.h>
#include<string>
#define P2P_PORT 29999//端口
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
	bool _pair_ret;//用于存放配对结果
};
class Client{
public:
	bool Start()
	{
		//客户端程序需要循环运行，因为下载文件不只是一次
		//循环运行，每次下载一个文件之后就会重新主动匹配主机，这样是不合理的
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
	//主机配对的线程入口函数
	void HostPair(Host* host){
		//1.组织http协议格式的请求数据
		//2.搭建一个tcp客户端，将数据发送
		//3.等待服务器的回复，并进行解析
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);//实例化httplib客户端对象
		auto rsp = cli.Get("/hostpair");//向服务端发送资源为/hostpair的Get请求，//若连接建立失败会返回NULL
		if (rsp && rsp->status == 200){//判断相应结果是否正确
			host->_pair_ret = true;//重置主机配对结果
		}
		return;
	}
	bool GetOnlineHost()//获取在线主机
	{
		char ch='Y';//是否重新匹配，默认是进行匹配，若已经匹配，online主机不为空则让用户选择
		if (!_online_host.empty())
		{
			std::cout << "是否重新查看在线主机（Y/N）";
			fflush(stdout);
			std::cin >> ch;
		}
		else
		{
			ch = 'Y';
		}
		if (ch == 'Y')
		{
			std::cout << "开始主机匹配……\n";
			//1.获取网卡信息，进而得到局域网中所有的IP地址列表
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); i++){
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				uint32_t net = ntohl(ip&mask);
				//计算最大主机号
				uint32_t max_host = (~ntohl(mask));
				for (int j = 1; j < (int32_t)max_host; j++)
				{
					uint32_t host_ip = htonl(net + j);//这个主机IP的计算应该使用主机字节序的网络号和主机号
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
			std::cout << "正在主机匹配中，请稍后……\n";
			for (int i = 0; i < host_list.size(); i++){
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true){
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}

		//将所有主机IP打印出来，供用户选择
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//3.若配对请求得到响应，则对主机位在线主机，则将IP添加到_online_host列表中
		//4.打印在线主机列表，使用户选择
		std::cout << "请选择配对主机，获取文件共享列表";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		//select_ip->Host
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			if (buf == select_ip){
				//用户选择主机IP地址
				GetShareList(select_ip);//用户选择主机IP地址
				return true;
			}
		}
		std::cout << "输入IP地址错误" << std::endl;
		return false;
	}
	bool GetShareList(std::string &host_ip){
		//获取文件列表
		//向服务端发送一个文件获取请求
		//1.发送请求
		//2.得到响应之后，解析正文（文件名称）
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cout << rsp->status << std::endl;
			std::cerr << "获取文件列表响应错误\n";
			return false;
		}
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件：";
		fflush(stdout);
		std::string filename;
		while(1){
			std::cin >> filename;
			if (rsp->body.find(filename) == std::string::npos)
			{
				std::cout << "输入不正确，请重新输入" << std::endl;
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
		//下载文件
		//1.向服务端发送文件下载请求
		//2.得到响应结果，响应结果中的body正文就是文件数据
		//3.创建文件，将文件数据写入文件中，关闭文件
		std::string req_path = "/download/" + filename;
		std::string realpath = DOWNLOAD_PATH + filename;
		while (boost::filesystem::exists(realpath)){//判断文件是否重名
			static int i = 0;
			//2.获取不带后缀的文件名
			std::string name = filename.substr(0, filename.rfind("."));
			std::cout << name << std::endl;

			name += std::to_string(i);
			i++;

			//3.获取后缀名
			std::string suffix_str = filename.substr(filename.find_last_of('.') + 1);
			std::cout << suffix_str << std::endl;

			filename = name + "."+ suffix_str;
			realpath = DOWNLOAD_PATH + filename;
		}
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "下载文件，获取响应信息失败\n";
			return false;
		}
		if (!boost::filesystem::exists(DOWNLOAD_PATH)){
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		if (FileUtil::Write(realpath, rsp->body) == false){
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "文件下载成功" << std::endl;
		return true;
	}

	//超大文件传输时使用分块传输

	bool RangeDownload2(const std::string &host_ip, std::string &name, int64_t s, int64_t e){
		std::string req_path = "/download/" + name;
		std::string realpath = DOWNLOAD_PATH + name;
		while (boost::filesystem::exists(realpath)){//判断文件是否存在
			static int i = 0;
			//2.获取不带后缀的文件名
			std::string name = name.substr(0, name.rfind("."));
			std::cout << name << std::endl;

			name += std::to_string(i);
			i++;


			//3.获取后缀名
			std::string suffix_str = name.substr(name.find_last_of('.') + 1);
			std::cout << suffix_str << std::endl;

			name = name + "." + suffix_str;
			realpath = DOWNLOAD_PATH + name;
		}
		//httplib::Headers header;
		//header = httplib::make_range_header({{s, e}});
		//header.insert(httplib::make_range_header({ {s, e} }));//设置一个range区间
		std::stringstream tmp;
		tmp << "byte=" << s << "-" << e;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		httplib::Headers header;
		header.insert(std::make_pair("Range", tmp.str()));
		auto rsp = cli.Get(req_path.c_str(), header);//向服务端发送一个分段请求
		if (rsp == NULL || rsp->status != 206) {
			if (rsp == NULL) { std::cout << "client rsp status NULL\n"; }
			else { std::cout << "响应状态码" << rsp->status << "\n"; }
			std::cout << "range download failed\n";
			return false;
		}
		FileUtil::Write(realpath, rsp->body, s);
		std::cout << "client range write success\n";
		return true;
	}
	int64_t getfilesize(const std::string &host_ip, const std::string &req_path) {
		//1. 发送HEAD请求，通过响应中的Content-Length获取文件大小
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200) {
			std::cout << "获取文件大小信息失败\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::Str2Dig(clen);
		return filesize;
	}
	bool RangeDownload1(const std::string &host_ip,  std::string &name) {
		std::string req_path = "/download/" + name;
		int64_t filesize = getfilesize(host_ip, req_path);
		//2. 根据文件大小进行分块
		//int range_count = filesize / MAX_RANGE;
		//a. 若文件大小小于块大小，则直接下载文件
		if (filesize < MAX_RANGE) {
			std::cout << "文件较小,直接下载文件\n";
			return DownloadFile(host_ip, name);
		}
		//计算分块个数
		//b. 若文件大小不能整除块大小，则分块个数位文件大小除以分块大小然后+1
		//c. 若文件大小刚好整除块大小，则分块个数就是文件大小除以分块大小
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
			//3. 逐一请求分块区间数据，得到响应之后写入文件的指定位置
			RangeDownload2(host_ip, name, range_start, range_end);
		}
		std::cout << "下载成功\n";
		return true;
	}

private:
	std::vector<Host> _online_host;
};