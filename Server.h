#include"client.h"
class Server{
public:
	bool Start()
	{
		//�����Կͻ�������Ĵ���ʽ��ϵ
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);
		_srv.Get("/download/.*", Download);//������ʽ:�������ַ���ָ���ĸ�ʽ����ʾ����ĳ�ֹؼ�����
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp){
		//�����������
		rsp.status = 200;
		return;
	}

	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//��ȡ�����ļ��б�-������������һ������Ŀ¼���������Ŀ¼�µ��ļ�����Ҫ��������˵�
		//1.�鿴Ŀ¼�Ƿ���ڣ���Ŀ¼�������򴴽�Ŀ¼
		if (!boost::filesystem::exists(SHARED_PATH)){//���Ŀ¼���ٴ���
			boost::filesystem::create_directory(SHARED_PATH);//����Ŀ¼
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);
		boost::filesystem::directory_iterator end;
		//��ʼ����Ŀ¼
		for (; begin != end; begin++)
		{
			if (boost::filesystem::is_directory(begin->status())){
				//��ǰ�汾����ֻ��ȡ��ͨ�ļ����ƣ��������Ŀ¼
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
		std::string name = req_path.filename().string();//ֻ��ȡ�ļ���
		std::string realpath = SHARED_PATH + name;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath)){//�ж��ļ��Ƿ����
			rsp.status = 404;
			return;
		}
		//req.path--�ͻ����������Դ·��
		if (req.method == "GET"){
			//�ж��Ƿ��Ƿֿ鴫������ݾ�����������Ƿ���range
			if (req.has_header("Range")){//�ж��������Ƿ����Range�ֶ�
				std::string range_str = req.get_header_value("Range");
				int range_start;
				int range_end;
				FileUtil::GetRange(range_str, &range_start, &range_end);
				int range_len = range_end - range_start + 1;

				std::cout << "Range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << "�������Ӧ�����������\n";
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
			rsp.set_header("Content-Length", std::to_string(filesize));//������Ӧͷ����Ϣ
			rsp.status = 200;
		}
		return;

	}
	httplib::Server _srv;
};