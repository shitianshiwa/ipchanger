#include <iostream>
#include <boost/program_options.hpp>

#include "config.h"
#include "myipinfo.h"
#include "iputil.h"

namespace po = boost::program_options;

void exit_with_info(std::string info, int exitCode = 1)
{
	info.append("\n�밴�س����˳�...\n");
	std::cout << info;
	getchar();
	exit(exitCode);
}

void print_ip_info(std::shared_ptr<myipinfo> &info)
{
	const in_addr &ip = info->ip();
	std::string strip = inet_ntoa(ip);
	std::cout << "����ip��ַ: " << strip << std::endl;
	std::cout << "��ǰ��������: "  << info->downflow() << "MB" << std::endl;
	std::cout << "��ǰ�ϴ�����: "  << info->upflow() << "MB" << std::endl;
}

enum{err_success, err_replaceip, err_checkstate};
int changeip(std::shared_ptr<myipinfo> &info, std::shared_ptr<IP_ADAPTER_INFO> &adapter)
{
	std::cout << "\n���ڲ���һ�����õ�ip...\n";
	in_addr newaddr = iputil::find_available_ip(info->ip());
	std::string strAddr = inet_ntoa(newaddr);
	std::cout << "find it: " << strAddr << std::endl;
	std::cout << "��ʼ�޸�ip...\n";
	DWORD context = iputil::replace_ip(adapter, info->ip() , newaddr);
	if (!context) return err_replaceip;

	return err_success;
}

void check_changeip(boost::asio::deadline_timer *timer, std::shared_ptr<myipinfo> &info, std::shared_ptr<IP_ADAPTER_INFO> &adapter)
{
	int bRet = err_success;
	while (bRet == err_replaceip || info->downflow() > 900 || info->upflow() > 900){
		bRet = changeip(info, adapter);
	}

	timer->async_wait(boost::bind(&check_changeip, timer, info, adapter));
}

class ipchanger{
public:
	ipchanger(std::shared_ptr<myipinfo> &info, std::shared_ptr<IP_ADAPTER_INFO> &adapter)
		: m_info(info), m_adapter(adapter)
	{}
public:
	void asyn_start_changeip()
	{
		;
	}
private:
	std::shared_ptr<myipinfo> m_info; 
	std::shared_ptr<IP_ADAPTER_INFO> m_adapter;
};

int main(int argc, char *argv[])
{
	po::options_description opts("usage:ipchanger [-c|-t]\n���õ�ѡ��");
	opts.add_options()
		("changeip,c", "ֱ�ӻ�һ��ip����֤��ip�����㹻ʹ��")
		("find,f", "���һ�����õ�ip, ֻ��֤��ipû����ռ��,\n����֤��ip������������")
		("info,i", "��ǰip������ʹ�����")
		("background,b", "��ΪĬ��ѡ�\n�ں�̨���У�ÿ��5���Ӽ��һ�Σ���������������꣬���Զ��л�ip")
		("help,h", "������Ϣ")
		;

	po::variables_map vm;
	try{
		po::store(po::parse_command_line(argc, argv, opts), vm);
		po::notify(vm);
	}
	catch(std::exception &e){
		std::cout << e.what() << std::endl;
		std::cout << opts << std::endl;
		return 0;
	}

	if (vm.count("help") != 0 ){
		std::cout << opts << std::endl;
		return 0;
	}

	boost::asio::io_service ioservice;
	std::shared_ptr<myipinfo> info(new myipinfo(ioservice));
	info->asyn_getinfo(ip_checker_server);

	std::cout << "���ڼ��ipʹ��״̬...\n";

	ioservice.run();
	ioservice.reset();

	if (info->checkstate()){
		print_ip_info(info);
	}
	else {
		exit_with_info("���ipʹ��״̬ʧ��");
	}

	if ( 0 == vm.size()) goto background;

	if (vm.count("find")){

		std::cout << "\n���ڲ���һ�����õ�ip...\n";
		in_addr addr = iputil::find_available_ip(info->ip());
		std::string strAddr = inet_ntoa(addr);
		std::cout << "find it: " << strAddr << std::endl;
	}
	else if (vm.count("changeip")){
		IP_ADDR_STRING *pAddr = NULL;
		std::shared_ptr<IP_ADAPTER_INFO> adapter = iputil::getAdapterInfo(info->ip(), pAddr);

		if (!adapter) exit_with_info("��ȡ��������Ϣʧ��");
		if (adapter->DhcpEnabled == TRUE)
			exit_with_info("��̬�����ip�������޸�");

		DWORD adapterIndex = adapter->Index;
		DWORD oldContext = pAddr->Context;
		DWORD netmask = inet_addr(pAddr->IpMask.String);

		do {
			int ret = changeip(info, adapter);
			if (err_replaceip == ret) exit_with_info("�޸�ipʧ��");
			std::cout << "ip�޸ĳɹ�!!!\n��ȴ�5����...\n";
			::Sleep(5000);

			std::cout << "��ʼ���¼��ipʹ��״̬...\n";
			info->clear();
			info.reset(new myipinfo(ioservice));
			info->asyn_getinfo(ip_checker_server);

			ioservice.run();
			ioservice.reset();

			if (info->checkstate()){
				print_ip_info(info);
			}
			else {
				exit_with_info("���ipʹ��״̬ʧ��");
			}

			
		} while (info->downflow() > 100 || info->upflow() > 100);

		exit_with_info("�Ѿ��л������ʵ�Ip :)", 0);
	}
	else if (vm.count("background")){
background:

checkandset:
		// check_changeip(&timer, info, adapter);
		while (info->downflow() > 900 || info->upflow() > 900)
		{
			IP_ADDR_STRING *pAddr = NULL;
			std::shared_ptr<IP_ADAPTER_INFO> adapter = iputil::getAdapterInfo(info->ip(), pAddr);

			if (!adapter) exit_with_info("��ȡ��������Ϣʧ��");
			if (adapter->DhcpEnabled == TRUE)
				exit_with_info("��̬�����ip�������޸�");

			int ret = changeip(info, adapter);
			if (err_replaceip == ret) exit_with_info("�޸�ipʧ��");
			std::cout << "ip�޸ĳɹ�!!!\n��ȴ�5����...\n";
			::Sleep(5000);

			std::cout << "��ʼ���¼��ipʹ��״̬...\n";
			// info->reset();
			info->clear();
			info.reset(new myipinfo(ioservice));
			info->asyn_getinfo(ip_checker_server);

			ioservice.run();
			ioservice.reset();

			if (info->checkstate()){
				print_ip_info(info);
			}
			else {
				exit_with_info("���ipʹ��״̬ʧ��");
			}
		}
		{
			boost::asio::deadline_timer timer(ioservice);
			boost::posix_time::ptime expertime = boost::posix_time::second_clock::local_time();
			expertime += boost::posix_time::minutes(5);

			std::cout << "���� " << boost::posix_time::to_simple_string(expertime) << " �������һ�μ��\n";

			timer.expires_from_now(boost::posix_time::minutes(5));

			boost::system::error_code err;
			timer.wait(err);
		}

		info->clear();
		info.reset(new myipinfo(ioservice));
		info->asyn_getinfo(ip_checker_server);

		ioservice.run();
		ioservice.reset();

		if (info->checkstate()){
			print_ip_info(info);
		}
		else {
			exit_with_info("���ipʹ��״̬ʧ��");
		}
		goto checkandset;
	}

}