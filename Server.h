#include"client.h"
class Server{
public:
	bool Start()
	{
		//添加针对客户端请求的处理方式关系
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);
		_srv.Get("/download/.*", Download);//正则表达式:将特殊字符以指定的格式来表示具有某种关键特征
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp){
		//主机配对请求
		rsp.status = 200;
		return;
	}

	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//获取共享文件列表-在主机上设置一个共享目录，凡是这个目录下的文件都是要共享给别人的
		//1.查看目录是否存在，若目录不存在则创建目录
		if (!boost::filesystem::exists(SHARED_PATH)){//如果目录不再存在
			boost::filesystem::create_directory(SHARED_PATH);//创建目录
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);
		boost::filesystem::directory_iterator end;
		//开始迭代目录
		for (; begin != end; begin++)
		{
			if (boost::filesystem::is_directory(begin->status())){
				//当前版本我们只获取普通文件名称，不做多层目录
				continue;
			}
			std::string name = begin->path().filename().string();
			rsp.body += name + "\r\n";
		}
		rsp.status = 200;
		return;
	}
	static void Download(const httplib::Request &req, httplib::Response &rsp){
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string();//只获取文件名
		std::string realpath = SHARED_PATH + name;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath)){//判断文件是否存在
			rsp.status = 404;
			return;
		}
		//req.path--客户端请求的资源路径
		if (req.method == "GET"){
			//判断是否是分块传输的依据就是这次请求是否有range
			if (req.has_header("Range")){//判断请求中是否包含Range字段
				std::string range_str = req.get_header_value("Range");
				int range_start;
				int range_end;
				FileUtil::GetRange(range_str, &range_start, &range_end);
				int range_len = range_end - range_start + 1;

				std::cout << "Range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << "服务端响应区间数据完毕\n";
			}
			else
			{
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
		}
		else
		{
			int64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//设置响应头部信息
			rsp.status = 200;
		}
		return;

	}
	httplib::Server _srv;
};