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
	boost::filesystem::directory_iterator begin(ptr);//����һ��Ŀ¼������
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin){
		if (boost::filesystem::is_directory(begin->status()))//�жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
		{
			std::cout << begin->path().string() << "��һ���ļ�Ŀ¼\n";
		}
		else
		{
			std::cout << begin->path().string() << "��һ����ͨ�ļ�\n";
			std::cout << begin->path().filename().string()<<std::endl;//ֻ��ȡ�ļ�·�����е����Ʋ�Ҫ·��
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
	//�����߳���ҩ���пͻ���ģ���Լ������ģ��
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread thr_client(ClientRun);

	Server srv;
	srv.Start();
	system("pause");
	return 0;
}