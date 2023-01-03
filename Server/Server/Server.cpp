#define FD_SETSIZE 1028  // user 1027 access
#include <iostream>
#include <map>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <string.h>
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
	time_t timer = time(NULL);
	struct tm t;
	localtime_s(&t, &timer); 
	osf << (t.tm_year + 1900) << "Y " << t.tm_mon + 1 << "M " << t.tm_mday << "D " << t.tm_hour << "H" << t.tm_min << "M " << t.tm_sec << "S ";
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

enum RoomResult
{
	FAILED_ROOM,
	SUCCESSED_ROOM
};

struct Session
{
	shared_ptr<boost::asio::ip::tcp::socket> sock; 
	boost::asio::ip::tcp::endpoint ep;
	int userIndex;
	int bufferSize;
	int roomNo = 0;

	char buffer[3000] = { 0, };
};

int userNum = 1;

class Server
{
	boost::asio::io_service ios;
	shared_ptr<boost::asio::io_service::work> work; 
	boost::asio::io_service::strand m_strand;
	boost::asio::ip::tcp::endpoint ep; 
	boost::asio::ip::tcp::acceptor gate;
	std::vector<shared_ptr<Session>> sessions;
	boost::thread_group threadGroup;
	boost::mutex lock;
	boost::system::error_code error;

public:
	Server(unsigned short port_num) : 
		work(new boost::asio::io_service::work(ios)),
		ep(boost::asio::ip::tcp::v4(), port_num),
		gate(ios, ep.protocol()),
		m_strand(ios)
	{
		roomList.push_back(0);
		roomList.push_back(1);
		roomList.push_back(2);
	}
	void Start()
	{
		cout << "Start Server" << endl;
		cout << "Creating Threads" << endl;

		for (int i = 0; i < 8; i++)
			threadGroup.create_thread(boost::bind(&Server::WorkerThread, this));

		this_thread::sleep_for(chrono::milliseconds(100));
		cout << "Threads Created" << endl;

		ios.post(m_strand.wrap(boost::bind(&Server::OpenGate, this)));

		threadGroup.join_all();
	}

private:
	void WorkerThread()
	{
		ios.run();
	}

	void OpenGate()
	{
		boost::system::error_code ec;
		gate.bind(ep, ec);
		if (ec)
		{
			cout << "bind failed: " << ec.message() << endl;
			return;
		}

		cout << "Gate Opened" << endl;

		StartAccept();
		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
	}

	void StartAccept()
	{
		shared_ptr<Session> session = make_shared<Session>();
		shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(ios));
		session->sock = sock;
		session->userIndex = userNum++;
		session->roomNo = 0;
		gate.listen();
		gate.async_accept(*sock, session->ep, m_strand.wrap(boost::bind(&Server::OnAccept, this, _1, session))); 
	}

	void OnAccept(const boost::system::error_code& ec, shared_ptr<Session> session)
	{
		flatbuffers::FlatBufferBuilder builder;

		if (ec)
		{
			cout << "accept failed: " << ec.message() << endl;
			return;
		}

		sessions.push_back(shared_ptr<Session>(session));
		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;

		ios.post(m_strand.wrap(boost::bind(&Server::Receive, this, session, ec))); 
		StartAccept();

		osf << session->userIndex << " st client access" << endl;
		builder.Finish(CreateS2C_PID_ACK(builder, 12, 3, session->userIndex));
		char s2cPidAck[BUF_SIZE] = { 0, };
		memcpy(&s2cPidAck, builder.GetBufferPointer(), builder.GetSize());
		session->sock->async_write_some(boost::asio::buffer(s2cPidAck), m_strand.wrap(boost::bind(&Server::OnSend, this, session, error)));
		osf << TimeResult() << " [ACK] Port No: " << port << " User " << session->userIndex << " st client" << endl;
	}

	void Receive(shared_ptr<Session> session, const boost::system::error_code& ec)
	{
		boost::system::error_code r_ec;

		session->sock->async_read_some(boost::asio::buffer(session->buffer), m_strand.wrap(boost::bind(&Server::Receive, this, session, r_ec))); 
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

	void OnSend(shared_ptr<Session> session, const boost::system::error_code& ec)
	{
		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}
	}

	void CloseSession(shared_ptr<Session> session)
	{
		if (session.use_count() > 0)
		{
			for (int i = 0; i < sessions.size(); i++)
			{
				if (sessions[i]->sock == session->sock)
				{
					sessions.erase(sessions.begin() + i);
					osf << TimeResult() << " Port No : " << port << ", Closed" << "\" from server \"" << session->userIndex << "\" st Client" << endl;

					break;
				}
			}
			session->sock->close();
		}
	}

	void RecvCharEcho(shared_ptr<Session> session)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto c2sEchoReq = GetC2S_CHATECHO_REQ(session->buffer);
		builder.Finish(CreateS2C_CHATECHO_NTY(builder, c2sEchoReq->size(), 1, c2sEchoReq->userIdx(), builder.CreateString(c2sEchoReq->msg()->c_str())));
		auto s2cEchoNty = GetS2C_CHATECHO_NTY(builder.GetBufferPointer());

		for (int k = 0; k < sessions.size(); k++)// 
		{
			if (sessions[k]->userIndex == s2cEchoNty->userIdx())
				CurrentUserPid = k;
		}
		memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

		sessions[CurrentUserPid]->bufferSize = builder.GetSize();
		sessions[CurrentUserPid]->userIndex = s2cEchoNty->userIdx();

		osf << TimeResult() << "Port No. : " << port << ", msg received. Total size : \"" << s2cEchoNty->size() << "\" , Code: \"" << s2cEchoNty->code() <<  "\" , String: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;

		for (int k = 0; k < sessions.size(); k++)
		{
			if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo && sessions[k] != sessions[CurrentUserPid])
			{
				sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, sessions[CurrentUserPid], error)));
			}
		}

		osf << TimeResult() << " [NTY]  Port No. : " << port << ", [send] msg received. Total Size : \"" << s2cEchoNty->size() << "\" , Code: \"" << s2cEchoNty->code()  << "\" , String: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;

		builder.Clear();

		builder.Finish(CreateS2C_CHATECHO_ACK(builder, c2sEchoReq->size(), 5, 1, builder.CreateString(c2sEchoReq->msg()->c_str())));
		auto s2cEchoAck = GetS2C_CHATECHO_ACK(builder.GetBufferPointer());

		memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

		sessions[CurrentUserPid]->bufferSize = builder.GetSize();

		sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, sessions[CurrentUserPid], error)));

		osf << TimeResult() << " [ACK] Port No. : " << port << ", [send] msg received. Total Size : \"" << s2cEchoAck->size() << "\" , Code: \"" << s2cEchoAck->code() << "\" , String: \"" << s2cEchoAck->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;
		
		memset(&sessions[CurrentUserPid]->buffer, 0, 3000);
	}

	void RecvCharValidRoomNo(shared_ptr<Session> session)
	{
		flatbuffers::FlatBufferBuilder builder;

		auto c2sRoomReq = GetC2S_ROOM_ENTER_REQ(session->buffer); 

		for (int k = 0; k < sessions.size(); k++)
		{
			if (sessions[k]->userIndex == c2sRoomReq->userIdx())
				CurrentUserPid = k;
		}

		if (find(roomList.begin(), roomList.end(), c2sRoomReq->roomNo()) != roomList.end()) // Room No exists in the server
		{
			builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, c2sRoomReq->size(), 2, c2sRoomReq->roomNo(), c2sRoomReq->userIdx())); 
			auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());

			sessions[CurrentUserPid]->roomNo = s2cRoomNty->roomNo();
			sessions[CurrentUserPid]->bufferSize = s2cRoomNty->size();
			memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

			osf << TimeResult() << "Port No: " << port << ", [recv] msg received. Total Buffer Size : \"" << s2cRoomNty->size() << "\" ,Code: \"" << s2cRoomNty->code() << "\" , Result: \"" << RoomResult::SUCCESSED_ROOM << "\" , RoomNo: \"" << s2cRoomNty->roomNo() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;

			for (int k = 0; k < sessions.size(); k++)
			{
				if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo && sessions[k] != sessions[CurrentUserPid])
				{
					sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, sessions[CurrentUserPid], error)));
				}
			}
			                                                                                       
			osf << TimeResult() << " [NTY] Port No : " << port << ", [send] msg received. Total Buffer Size : \"" << s2cRoomNty->size() << "\" ,Code: \"" << s2cRoomNty->code() << "\" , Result: \"" << RoomResult::SUCCESSED_ROOM << "\" , RoomNo: \"" << s2cRoomNty->roomNo() << "\" from server \"" << s2cRoomNty->userIdx() << "\" st Client" << endl;	

			builder.Clear();
			builder.Finish(CreateS2C_ROOM_ENTER_ACK(builder, c2sRoomReq->size(), 4, c2sRoomReq->roomNo(), RoomResult::SUCCESSED_ROOM)); 
			auto s2cRoomAck = GetS2C_ROOM_ENTER_ACK(builder.GetBufferPointer());
			memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

			sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, sessions[CurrentUserPid], error)));

			osf << TimeResult() << " [ACK] Port No: " << port << ", [send] msg received. Total Buffer Size : \"" << s2cRoomNty->size() << "\" ,Code: \"" << s2cRoomNty->code() << "\" , Result: \"" << RoomResult::SUCCESSED_ROOM << "\" , RoomNo: \"" << s2cRoomNty->roomNo() << "\" from server \"" << s2cRoomNty->userIdx() << "\" st Client" << endl;
		}
		else 
		{
			builder.Clear();
			builder.Finish(CreateS2C_ROOM_ENTER_ACK(builder, c2sRoomReq->size(), 4, c2sRoomReq->roomNo(), RoomResult::FAILED_ROOM)); 
			auto s2cRoomAck = GetS2C_ROOM_ENTER_ACK(builder.GetBufferPointer());

			sessions[CurrentUserPid]->roomNo = 0;
			sessions[CurrentUserPid]->bufferSize = s2cRoomAck->size();
			memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

			osf << TimeResult() << "Port No: " << port << ", [recv] msg received. Total Buffer Size : \"" << s2cRoomAck->size() << "\" ,Code: \"" << s2cRoomAck->code() << "\" , Result: \"" << s2cRoomAck->result() << "\" , RoomNo: \"" << sessions[CurrentUserPid]->roomNo << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;

			sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), m_strand.wrap(boost::bind(&Server::OnSend, this, sessions[CurrentUserPid], error)));

			osf << TimeResult() << " [ACK] Port No: " << port << ", [send] msg received. Total Buffer Size : \"" << s2cRoomAck->size() << "\" ,Code: \"" << s2cRoomAck->code() << "\" , Result: \"" << s2cRoomAck->result() << "\" , RoomNo: \"" << sessions[CurrentUserPid]->roomNo << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" st Client" << endl;	

		}
		memset(&sessions[CurrentUserPid]->buffer, 0, 3000);
	}
};

int main(void)
{
	file.open("C:\\Users\\secrettown\\source\\repos\\Server\\Server\\log.txt", ios_base::out | ios_base::app);

	roomList.push_back(0);
	roomList.push_back(1);
	roomList.push_back(2);
	char c;
	ifstream fin("C:\\Users\\secrettown\\source\\repos\\chattingRoom_BoostAsioStrand\\Server\\x64\\Debug\\port.txt");
	if (fin.fail())
	{
		osf << "Port File not exists" << endl;
		return 0;
	}
	int i = 0;
	while (fin.get(c))
	{
		port[i++] = c;
	}
	fin.close(); 
	Server serv(atoi(port));
	serv.Start();

	return 0;
}

////https://codeantenna.com/a/pSHoTCGJrj Reference  ==> LNK1104	'libboost_thread-vc142-mt-gd-x64-1_81.lib' not open the file.
