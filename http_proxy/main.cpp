#pragma comment(lib,"Ws2_32.lib")
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <regex>
#include <fcntl.h>

using namespace std;

int InBound = 0, OutBound = 0,c;
string InBound_Data[10], InBound_Change_Data[10], OutBound_Data[10], OutBound_Change_Data[10];

void check_argc(int argc)
{
	if ((argc <3 && argc>3))
	{
		cout << "[�����̸�] [���Ͻ� IP][���Ͻ� Port]" << endl;
		cout << "ex) ./�����̸� 127.0.0.1 8080" << endl;
		exit(1);
	}

	system("cls");

	cout << "****************************************************" << endl;
	cout << "*********************HTTP Proxy*********************" << endl;
	cout << "****************************************************" << endl;
}

void init_WSA()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nRet != 0)
	{
		switch (nRet)
		{
		case WSASYSNOTREADY:
			cout << "��Ʈ��ũ ������ �غ� ����" << endl;
			exit(1);
			break;
		case WSAVERNOTSUPPORTED:
			cout << "�䱸�� ���� ������ ����Ʈ �ȵʸ���" << endl;
			exit(1);
			break;
		case WSAEINPROGRESS:
			exit(1);
			cout << "���ŷ ������ ������" << endl;
			break;
		case WSAEPROCLIM:
			exit(1);
			cout << "�ִ� ���� ���μ��� ó���� �ʰ�" << endl;
			break;
		case WSAEFAULT:
			exit(1);
			cout << "�ι�° �μ��� �����Ͱ� ��ȿ" << endl;
			break;
		}
	}
}

sockaddr_in init_Addr(string addr, int port)
{
	sockaddr_in return_Addr;
	memset(&return_Addr, 0, sizeof(return_Addr));
	return_Addr.sin_family = AF_INET;
	return_Addr.sin_port = htons(port);

	if (addr.empty())
	{
		cout << "���� �ּ�ü�� �ʱ�ȭ ����" << endl;
		exit(1);
	}
	else
		inet_pton(AF_INET, addr.c_str(), &return_Addr.sin_addr.s_addr);
		//return_Addr.sin_addr.s_addr = htonl(INADDR_ANY);	
	
	return return_Addr;
}

void socket_init(SOCKET &sock, sockaddr_in &addr)
{
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cout << "���� ���� ����" << endl;
		exit(1);
	}

	if (::bind(sock, (struct sockaddr*)&addr, sizeof(sockaddr)) != 0)
	{
		cout << "bind  ����" << endl;
		exit(1);
	}

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR) 
	{
		cout << "listen  ����" << endl;
		exit(1);
	}

}

string getAddr(char *Client_Data)
{
	string data(Client_Data);

	std::smatch result;
	std::regex p("Host: (.*)");

	if (std::regex_search(data, result, p))
		return result[1];

	return "";
}

string Get_Site_Addr(string Host_Site)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	struct sockaddr_in *sin;
	int *listen_fd;
	int listen_fd_num = 0;
	char buf[80] = { 0x00, };
	int i = 0;
	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(Host_Site.c_str(), NULL, &hints, &result) != 0) {
		perror("getaddrinfo");
		return std::string("");
	}
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		listen_fd_num++;
	}
	listen_fd = (int *)malloc(sizeof(int)*listen_fd_num);

	for (rp = result, i = 0; rp != NULL; rp = rp->ai_next, i++)
	{
		if (rp->ai_family == AF_INET)
		{
			sin = (sockaddr_in *)rp->ai_addr;
			inet_ntop(rp->ai_family, &sin->sin_addr, buf, sizeof(buf));
			return std::string(buf);
		}
	}

	free(listen_fd);
	return NULL;
}

void httpproxy_th2(SOCKET Client, SOCKET Web)//httpproxy_th2
{
	char buf[65535];//������ ���� ����.
	char *web_buf;
	int recv_len;
	bool ch = false;
	while ((recv_len = recv(Web, buf, 65535, 0)) > 0)//Ÿ�Ӿƿ� �ɱ�
	{
		if (recv_len == SOCKET_ERROR)
		{
			cout << "th2 recv �Լ� ����" << endl;
			continue;
		}
		web_buf = (char *)malloc(sizeof(char)*recv_len); //recv ���� ����Ʈ ��ŭ ����

		memcpy(web_buf, buf, recv_len);
		
		//inbound-> ���� Ŭ���̾�Ʈ�� ��û�� ������� �����Ͱ� Ŭ���̾�Ʈ�� �����ִ� ��Ŷ.

		if (InBound == 1)
		{
			std::string InBound_String(web_buf);

			for (int i = 0; i < c; i++)
			{
				if (InBound_String.find(InBound_Data[i]) != string::npos)
				{
					ch = true;
					while (InBound_String.find(InBound_Data[i]) != string::npos)
					{
						InBound_String.replace(InBound_String.find(InBound_Data[i]), InBound_Data[i].length(), InBound_Change_Data[i]);
						cout << "Inbound Data Change Sucess!" << endl;
					}
				}
			}
		
			if (ch)
			{
				char *In_Data = strdup(InBound_String.c_str());

				//cout << web_buf << endl;
				if (send(Client, In_Data, recv_len, 0) == SOCKET_ERROR)
				{
					cout << "th2 send �Լ� ����" << endl;
					continue;
				}
			}

			
			//free(In_Data);
		}
		//cout << Inbound_String << endl;
		
		if (send(Client, web_buf, recv_len, 0) == SOCKET_ERROR)
		{
			cout << "th2 send �Լ� ����" << endl;
			continue;
		}
		
		//free(web_buf);
	}
}

void Client_th1(sockaddr_in Server_Addr, SOCKET Client)//Ŭ���̾�Ʈ�� ��û�� �޴� ������ Client_th1
{
	int http_port = 80,recv_len;

	char buf[65535];
	char *recv_buf;
	bool ch=false;

	string host_Addr, Domain_Addr;
	SOCKET Web_Socket;
	sockaddr_in Web_Addr;

	while ((recv_len = recv(Client, buf, 65535, 0)) > 0)
	{
		if (recv_len == SOCKET_ERROR)
		{
			cout << "th1 recv �Լ� ����" << endl;
			break;
		}

		// client�� ��û�� ������ Buf�� ������ .

		recv_buf = (char *)malloc(sizeof(char)*recv_len);

		memcpy(recv_buf, buf, recv_len);

		if (OutBound == 1)
		{
			std::string OutBound_String(recv_buf);

			for (int i = 0; i < c; i++)
			{
				if (OutBound_String.find(OutBound_Data[i]) != string::npos)
				{
					ch = true;
					while (OutBound_String.find(OutBound_Data[i]) != string::npos)
					{
						OutBound_String.replace(OutBound_String.find(OutBound_Data[i]), OutBound_Data[i].length(), OutBound_Change_Data[i]);
						cout << "OutBound Data Change Sucess!" << endl;
					}
				}
			}

			if (ch)
			{
				char *Out_Data = strdup(OutBound_String.c_str());
				memcpy(recv_buf, Out_Data, OutBound_String.length());
			}
		}

		host_Addr = getAddr(recv_buf);

		if (host_Addr == "")
		{
			cout << "Host_Addr �ּ� ��ȯ ����" << endl;
			break;
		}
		else
		{
			//cout << "Ŭ���̾�Ʈ�� ��û�� Site: "<< host_Addr << endl;
			//cout << recv_buf << endl;
			Domain_Addr = Get_Site_Addr(host_Addr);
			if (Domain_Addr == "")
			{
				cout << "Ŭ���̾�Ʈ�� ��û�� Site�� IP�ּ� ��ȯ ����" << endl;
				break;
			}
			cout << "Site IP Addr: " << Domain_Addr << endl;
		}

		Web_Addr = init_Addr(Domain_Addr, http_port);

		if ((Web_Socket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			cout << "Web ���� ���� ����" << endl;
			break;
		}

		else
		{
			if (connect(Web_Socket, (sockaddr*) &Web_Addr, sizeof(Web_Addr)) == SOCKET_ERROR)
			{
				std::cout << "Web �������" << endl;
				break;
			}

			//cout <<hex<< Web_Addr.sin_addr.s_addr << " " << hex << Web_Addr.sin_port;

			std::thread(httpproxy_th2, Client, Web_Socket).detach();

			//Ŭ���̾�Ʈ��(�������� ��û�ϴ� ����)

			if (send(Web_Socket, recv_buf, recv_len, 0) == SOCKET_ERROR)
			{
				printf("send to webserver failed.");
				continue;
			}

			//free(Out_Data);
			//free(recv_buf);
			//std::cout << "���Ͻ÷� ����\n" << endl;
	
		}
	}

	memset(buf, NULL, 65535); //NULL �ʱ�ȭ
	closesocket(Client);
}

using namespace std;

int main(int argc, char *argv[])
{
	check_argc(argc);	// argc üũ
	init_WSA();			// ���� �ʱ�ȭ
	
	string Server_IP = argv[1];
	int Server_Port = atoi(argv[2]);	//���Ͻ� ���� ��Ʈ�� �ʱ�ȭ
	int Select_Mode = 0, count;

	cout << endl;
	cout << "0. Noting" << endl;
	cout << "1. InBound" << endl;
	cout << "2. OutBound" << endl;
	cout << "Select: ";
	cin >> Select_Mode;
	

	if (Select_Mode == 1)
	{	
		InBound = 1;
		system("cls");
		cout << "InBound_Data_Count: ";
		cin >> count;
		c = count;

		for (int i = 0; i < count; i++)
		{
			cout << i + 1 << ". InBound_Data: ";
			cin >> InBound_Data[i];
			cout << i + 1 << ". InBound_Change_Data: ";
			cin >> InBound_Change_Data[i];
		}
	}

	else if (Select_Mode == 2)
	{
		OutBound = 1;
		system("cls");
		cout << "OutBound_Data_Count: ";
		cin >> count;
		c = count;

		for (int i = 0; i < count; i++)
		{
			cout << i + 1 << ". OutBound_Data: ";
			cin >> OutBound_Data[i];
			cout << i + 1 << ". OutBound_Change_Data: ";
			cin >> OutBound_Change_Data[i];
		}
	}

	system("cls");

	//cout << Server_IP  <<" "<< Server_Port << endl;

	sockaddr_in Server_Addr = init_Addr(Server_IP, Server_Port);

	//cout <<hex<< Server_Addr.sin_addr.s_addr << " " << hex <<Server_Addr.sin_port;

	SOCKET Server_Socket, Client_Socket; //����, Ŭ���̾�Ʈ ���� ����.

	socket_init(Server_Socket, Server_Addr);

	while (true)
	{
		if ((Client_Socket = accept(Server_Socket, NULL, NULL)) == INVALID_SOCKET)
		{
			cout << "Accept ����" << endl;
			exit(1);
		}
		else
			std::thread(Client_th1, Server_Addr, Client_Socket).detach();
	}
		
	WSACleanup();
	return 0;
}