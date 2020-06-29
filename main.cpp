#include"util.h"
#include"client.h"
#include"Server.h"
void helloworld(const httplib::Request &req, httplib::Response &rsp){
	rsp.set_content("<html><h1>hello fengxianglong</h1></html>","text/html");
	rsp.status = 200;
}
void Scandir()
{
	const char* ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//定义一个目录迭代器
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin){
		if (boost::filesystem::is_directory(begin->status()))//判断当前文件是否是一个目录
		{
			std::cout << begin->path().string() << "是一个文件目录\n";
		}
		else
		{
			std::cout << begin->path().string() << "是一个普通文件\n";
			std::cout << begin->path().filename().string()<<std::endl;//只获取文件路径名中的名称不要路径
		}
	}
}
void test()
{
	//std::vector<Adapter> list;
	//AdapterUtil::GetAllAdapter(&list);

	//httplib::Server srv;
	//srv.Get("/", helloworld);

	//srv.listen("0.0.0.0",19998);
	//Scandir();

}
void ClientRun()
{
	Sleep(1);
	Client cli;
	cli.Start();
}
int main(int argc, char *argv[])
{
	//在主线程中药运行客户端模块以及服务端模块
	//创建一个线程运行客户端模块，主线程运行服务端模块
	std::thread thr_client(ClientRun);

	Server srv;
	srv.Start();
	system("pause");
	return 0;
}