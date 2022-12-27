#define FD_SETSIZE 1028  // 유저 1027명까지 접속 허용
#include <iostream>
#include <map>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <string.h>
using boost::asio::ip::tcp;
using namespace std;
#define BUF_SIZE 512
#include <fstream>
#include <string>
#include <string.h>
#include <ctime>
#include "logger.h"
#include <list>
#include "flatbuffers/flatbuffers.h"
#include "C2S_CHATECHO_REQ_generated.h"
#include "C2S_PID_REQ_generated.h"
#include "C2S_ROOM_ENTER_REQ_generated.h"
#include "S2C_CHATECHO_ACK_generated.h"
#include "S2C_CHATECHO_NTY_generated.h"
#include "S2C_PID_ACK_generated.h"
#include "S2C_ROOM_ENTER_ACK_generated.h"
#include "S2C_ROOM_ENTER_NTY_generated.h"

int CurrentUserPid = 0;
CHAR port[10] = { 0, };// = "3587";

ofstream file;
ostreamFork osf(file, cout);

std::vector<int> roomList;

string TimeResult()
{
	time_t timer = time(NULL);// 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
	struct tm t;
	localtime_s(&t, &timer); // 포맷팅을 위해 구조체에 넣기
	osf << (t.tm_year + 1900) << "년 " << t.tm_mon + 1 << "월 " << t.tm_mday << "일 " << t.tm_hour << "시 " << t.tm_min << "분 " << t.tm_sec << "초 ";
	return " ";
}

enum Code
{
	NO_CHOICE,
	CHAT_ECHO,
	CHAT_ROOM,
	PID,
	VALID_ROOM_NO
};

struct Session
{
	shared_ptr<boost::asio::ip::tcp::socket> sock; // 소켓은 프로토콜 정보밖에 없는 객체 
	boost::asio::ip::tcp::endpoint ep;
	int userIndex;
	int bufferSize; // 총 버퍼 길이
	int roomNo = 0;

	char buffer[3000] = { 0, };
};

// 콜백 함수를 핸들러라고 부른다 
int userNum = 1;

class Server
{
	boost::asio::io_service ios;
	vector<boost::asio::io_service::work*> work;
	boost::asio::io_service::strand m_strand; // 멀티 스레드에서 동일 핸들러로 처리 될 경우 공유 자원에 대한 접근을 제어  == lock객체와 유사
	// => 같은 핸들러가 겹치기 않게 strand가 알아서 함수 실행 시켜줌 
	// lock  없이도 멀티 스레드 프로그래밍 가능 
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::tcp::acceptor gate;
	std::vector<Session*> sessions;
	boost::thread_group threadGroup;
	boost::mutex lock; // 동시적으로 여러개의 스레드로부터 공유되는 자원을 보호 ==> 해당 핸들러의 종료를 기다리며 lock이 풀리길 기다리며 무의미하게 기다림
	// 다른 스레드가 해당 변수에 접근하지 못하도록 제한 
	boost::system::error_code error;

public:
	Server(unsigned short port_num) :  // work는 io_service start 전에 post()에 기술된 핸들러 함수가 먼저 종료되는 상황에서 io_service start를 차 후에 호출하더라도 실제 post의 핸들러 함수는 돌지 않는다. 
		 // 이를 방지 하기 위해서 항상 block되어 돌도록 하기 위해서 사용 
		//work(new boost::asio::io_service::work(ios)),
		ep(boost::asio::ip::tcp::v4(), port_num), // ip 주소와 포트에 접속 
		gate(ios, ep.protocol()),
		m_strand(ios)
	{
		roomList.push_back(0); // 0 번방
		roomList.push_back(1);
		roomList.push_back(2);
		for (int i = 0; i < 1; i++)
			work.push_back(new boost::asio::io_service::work(ios));
	}
	void Start()
	{
		cout << "Start Server" << endl;
		cout << "Creating Threads" << endl;

		for (int i = 0; i < 1; i++) // 여기 개수 만큼 유저 추가 가능
			threadGroup.create_thread(boost::bind(&Server::WorkerThread, this));

		// thread 잘 만들어질때까지 잠시 기다리는 부분
		this_thread::sleep_for(chrono::milliseconds(100));
		cout << "Threads Created" << endl;
		/// <summary>
		/// /////////////
		/// </summary>

		ios.post(m_strand.wrap(boost::bind(&Server::OpenGate, this)));


		threadGroup.join_all();// 스레드가 종료 될 때 까지 기다림 
	}

private:
	void WorkerThread()
	{
		ios.run();
	}

	void OpenGate()
	{
		boost::system::error_code ec;
		gate.bind(ep, ec); // bind란 메소드와 객체를 묶어놓는 것 
		if (ec)
		{
			cout << "bind failed: " << ec.message() << endl;
			return;
		}

		gate.listen();
		cout << "Gate Opened" << endl;

		StartAccept();
		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
	}

	// 비동기식 Accept
	void StartAccept()
	{
		Session* session = new Session();
		shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(ios));
		session->sock = sock;
		session->userIndex = userNum++;
		session->roomNo = 0;
		gate.listen();
		gate.async_accept(*sock, session->ep, m_strand.wrap(boost::bind(&Server::OnAccept, this, _1, session))); //클라이언트 들어오면 아래 함수 실행
		// async_accept 비동기 승인
	}

	void OnAccept(const boost::system::error_code& ec, Session* session)
	{
		flatbuffers::FlatBufferBuilder builder;


		if (ec)
		{
			cout << "accept failed: " << ec.message() << endl;
			return;
		}

		//lock.lock();
		sessions.push_back(session);
		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
		//lock.unlock();

		ios.post(m_strand.wrap(boost::bind(&Server::Receive, this, session, ec)));
		StartAccept();

		osf << session->userIndex << " 번 째 클라이언트 접속" << endl;
		builder.Finish(CreateS2C_PID_ACK(builder, 12, 3, session->userIndex));
		char s2cPidAck[BUF_SIZE] = { 0, };
		memcpy(&s2cPidAck, builder.GetBufferPointer(), builder.GetSize());
		session->sock->async_write_some(boost::asio::buffer(s2cPidAck), m_strand.wrap(boost::bind(&Server::OnSend, this, error)));
		osf << TimeResult() << "[ACK] 포트 번호: " << port << " 유저 " << session->userIndex << " 번째 Client" << endl;
		builder.Clear();
	}

	void PacketReceive(Session* session, const boost::system::error_code& ec)
	{
		session->sock->async_read_some(boost::asio::buffer(session->buffer), m_strand.wrap(boost::bind(&Server::Receive, this, session, ec)));

		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}
		else if (session->buffer[0] == 0 && session->buffer[1] == 0 && session->buffer[2] == 0)
		{
		}
		else
		{
			auto s2cPidAck = GetS2C_PID_ACK(session->buffer);
			int code = 100;
			if (session->buffer[1] == 0 || session->buffer[2] == 0 || session->buffer[3] == 0)
				code = s2cPidAck->code();
			switch (code)
			{
			case Code::CHAT_ECHO:
				RecvCharEcho(session);
				break;
			case Code::VALID_ROOM_NO:
				RecvCharValidRoomNo(session);
				break;
			}
		}
	}

	// 동기식 Receive (쓰레드가 각각의 세션을 1:1 담당)
	void Receive(Session* session, const boost::system::error_code& ec)
	{
		//size_t size;
		//size = session->sock->read_some(boost::asio::buffer(session->buffer, sizeof(session->buffer)), ec);
		session->sock->async_read_some(boost::asio::buffer(session->buffer), m_strand.wrap(boost::bind(&Server::PacketReceive, this, session, ec))); // 유저한테 패킷 받을 때 마다 OnSend 이 함수로 감 => 이 함수를 이용 해라

		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}
		//else if (size == 0)
		//{
		//	cout << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
		//	CloseSession(session);
		//	return;
		//}
		else if (session->buffer[0] == 0 && session->buffer[1] == 0 && session->buffer[2] == 0)
		{
		}
		else
		{
			auto s2cPidAck = GetS2C_PID_ACK(session->buffer);
			int code = 100;
			if (session->buffer[1] == 0 || session->buffer[2] == 0 || session->buffer[3] == 0)
				code = s2cPidAck->code();
			switch (code)
			{
			case Code::CHAT_ECHO:
				RecvCharEcho(session);
				break;
			case Code::VALID_ROOM_NO:
				RecvCharValidRoomNo(session);
				break;
			}
		}

		/*session->buffer[size] = '\0';*/
		//session->buffer[sizeof(session->buffer)] = '\0';
	/*	Sleep(100);
		Receive(session);*/
	}


	void OnSend(const boost::system::error_code& ec)
	{
		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
			return;
		}
	}

	void CloseSession(Session* session)
	{
		// if session ends, close and erase
		for (int i = 0; i < sessions.size(); i++)
		{
			if (sessions[i]->sock == session->sock)
			{
				//lock.lock();
				sessions.erase(sessions.begin() + i);
				osf << TimeResult() << " 포트 번호: " << port << ", 종료" << "\" from server \"" << session->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

				//lock.unlock();
				break;
			}
		}

		session->sock->close();
		delete session;
	}
	void RecvCharEcho(Session* session)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto c2sEchoReq = GetC2S_CHATECHO_REQ(session->buffer);
		builder.Finish(CreateS2C_CHATECHO_NTY(builder, c2sEchoReq->size(), c2sEchoReq->code(), c2sEchoReq->userIdx(), builder.CreateString(c2sEchoReq->msg()->c_str())));
		auto s2cEchoNty = GetS2C_CHATECHO_NTY(builder.GetBufferPointer());

		for (int k = 0; k < sessions.size(); k++)// 
		{
			if (sessions[k]->userIndex == s2cEchoNty->userIdx())
				CurrentUserPid = k;
		}
		memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

		sessions[CurrentUserPid]->bufferSize = builder.GetSize();
		sessions[CurrentUserPid]->userIndex = s2cEchoNty->userIdx();

		osf << TimeResult() << "포트 번호: " << port << ", msg received. 전체 사이즈 : \"" << s2cEchoNty->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code() <<  "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

		for (int k = 0; k < sessions.size(); k++)
		{
			if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo)
			{
				sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, _1)));
			}
		}

		osf << TimeResult() << "[NTY]  포트 번호: " << port << ", [send] msg received. 전체 사이즈 : \"" << s2cEchoNty->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code()  << "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력
		memset(sessions[CurrentUserPid]->buffer, 0, builder.GetSize());

		builder.Clear();
	}
	void RecvCharValidRoomNo(Session* session)
	{
		flatbuffers::FlatBufferBuilder builder;

		auto c2sRoomReq = GetC2S_ROOM_ENTER_REQ(session->buffer); // c2sreq로 변경 s2cRoomNty.GetMsg() 이거 자체를 넣어버리면 char*가 deserialize에서 초기화 됨 

		for (int k = 0; k < sessions.size(); k++)
		{
			if (sessions[k]->userIndex == c2sRoomReq->userIdx())
				CurrentUserPid = k;
		}

		if (find(roomList.begin(), roomList.end(), c2sRoomReq->roomNo()) != roomList.end()) // 서버에 방 번호가 존재 할 시
		{
			builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, c2sRoomReq->size(), 2, c2sRoomReq->roomNo(), c2sRoomReq->userIdx())); // code 번호 변경
			auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());

			sessions[CurrentUserPid]->roomNo = s2cRoomNty->roomNo();
			sessions[CurrentUserPid]->bufferSize = s2cRoomNty->size();
			memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

			osf << TimeResult() << "포트 번호: " << port << ", [recv] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << 1 << "\" , RoomNo: \"" << s2cRoomNty->roomNo() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

			for (int k = 0; k < sessions.size(); k++)
			{
				if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo)
				{
					sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, _1)));
				}
			}

			osf << TimeResult() << "[NTY] 포트 번호: " << port << ", [send] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << 1 << "\" , RoomNo: \"" << s2cRoomNty->roomNo() << "\" from server \"" << s2cRoomNty->userIdx() << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력		
		}
		else // 방 번호가 존재 하지 않을 때  
		{
			builder.Clear();
			builder.Finish(CreateS2C_ROOM_ENTER_ACK(builder, c2sRoomReq->size(), 4, c2sRoomReq->roomNo(), 0)); // code 번호 변경
			auto s2cRoomAck = GetS2C_ROOM_ENTER_ACK(builder.GetBufferPointer());

			sessions[CurrentUserPid]->roomNo = 0;
			sessions[CurrentUserPid]->bufferSize = s2cRoomAck->size();
			memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

			osf << TimeResult() << "포트 번호: " << port << ", [recv] msg received. 총 버퍼 사이즈 : \"" << s2cRoomAck->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomAck->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << s2cRoomAck->result() << "\" , RoomNo: \"" << sessions[CurrentUserPid]->roomNo << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

			for (int k = 0; k < sessions.size(); k++)
			{
				if (k == CurrentUserPid)
				{
					sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, _1)));
				}
			}

			osf << TimeResult() << "[ACK] 포트 번호: " << port << ", [send] msg received. 총 버퍼 사이즈 : \"" << s2cRoomAck->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomAck->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << s2cRoomAck->result() << "\" , RoomNo: \"" << sessions[CurrentUserPid]->roomNo << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력		

		}
		builder.ReleaseBufferPointer();
		builder.Clear();
	}
};

int main(void)
{
	file.open("C:\\Users\\secrettown\\source\\repos\\Server\\Server\\log.txt", ios_base::out | ios_base::app);
	//file.open(".\\log.txt", ios_base::out | ios_base::app); // 파일 경로(c:\\log.txt)   exe파일 실행 해야함 
	//osf.rdbuf(file.rdbuf()); // 표준 출력 방향을 파일로 전환

	roomList.push_back(0);
	roomList.push_back(1);
	roomList.push_back(2);
	char c;
	ifstream fin("C:\\Users\\secrettown\\source\\repos\\chattingRoom_BoostAsioStrand\\Server\\Server\\port.txt");
	//ifstream fin(".\\port.txt");
	if (fin.fail())
	{
		osf << "포트 파일이 없습니다 " << endl;
		return 0;
	}
	int i = 0;
	while (fin.get(c))
	{
		port[i++] = c;
	}
	fin.close(); // 열었던 파일을 닫는다. 
	//osf << "Enter PORT number (3587) :";
	//cin >> port;
	Server serv(atoi(port));
	serv.Start();

	//for (int i = 0; i < 4; ++i) {
	//	thread{ [&]() {
	//		io_context.run();
	//	} }.detach(); // detach 스레드가 언제 종료될지 모른다. 
	//} // join -> 스레드가 종료되는 시점에 자원을 반환받는 것이 보장 
	return 0;
}

////https://codeantenna.com/a/pSHoTCGJrj 참조 ==> 오류	LNK1104	'libboost_thread-vc142-mt-gd-x64-1_81.lib' 파일을 열 수 없습니다.