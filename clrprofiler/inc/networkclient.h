#ifndef NETCLIENT
#define NETCLIENT

#pragma once
#include "INetworkClient.h"

class Cprofilermain;


#define _SECOND ((__int64) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)

enum NetworkCommands
{
	PING_SERVER,// 0x1
	PING_AGGREGATOR,
	PING_AGENT,
	SEND_MODULENAME,
	SEND_CLASSNAME,
	SEND_FUNCTIONNAME

};



class NetworkClient : public INetworkClient
{
public:
	NetworkClient(std::wstring hostName, std::wstring port);
	~NetworkClient();
	static SOCKET m_SocketConnection;
	static PTP_IO m_ptpIO;
	void SetPTPIO(PTP_IO ptpIO);
	static std::vector<char> s_OverflowBuffer;
private:
	// Create a singleton socket so we can control what happens if this
	// class is instantiated more than once.

	std::wstring m_HostName;
	std::wstring m_HostPort;
	

	// List of ICommand implementations by code
	std::map<short, std::shared_ptr<Commands::ICommand>> m_CommandList;

	// Double buffered queues for reading and writing so we don't bog down the enter/exit methods
	std::queue<std::shared_ptr<std::vector<char>>> m_OutboundQueueFront;
	std::queue<std::shared_ptr<std::vector<char>>> m_OutboundQueueBack;
	std::queue<std::shared_ptr<std::vector<char>>> m_InboundQueueFront;
	std::queue<std::shared_ptr<std::vector<char>>> m_InboundQueueBack;


	//Double buffered queues need double the locks
	CRITICAL_SECTION FrontOutboundLock;
	CRITICAL_SECTION BackOutboundLock;
	CRITICAL_SECTION FrontInboundLock;
	CRITICAL_SECTION BackInboundLock;

	static CRITICAL_SECTION OverflowBufferLock;

	PTP_TIMER recvTimer;

	PTP_TIMER sendTimer;

	static VOID CALLBACK SendTimerCallback(
		PTP_CALLBACK_INSTANCE pInstance, // See "Callback Termination Actions" section
		PVOID pvContext,
		PTP_TIMER pTimer);

	static VOID CALLBACK ReceiveTimerCallback(
		PTP_CALLBACK_INSTANCE pInstance, // See "Callback Termination Actions" section
		PVOID pvContext,
		PTP_TIMER pTimer);

	bool insideSendLock = false;
	bool insideReceiveLock = false;

	



	// This is the main loop that will be used for sending and receving data. When a call comes in we will have a callback to a correct processor
	void ControllerLoop();

	// Completion routines
	static void CALLBACK NewDataReceived(
		IN DWORD dwError,
		IN DWORD cbTransferred,
		IN LPWSAOVERLAPPED lpOverlapped,
		IN DWORD dwFlags
		);

	static void CALLBACK DataSent(
		IN DWORD dwError,
		IN DWORD cbTransferred,
		IN LPWSAOVERLAPPED lpOverlapped,
		IN DWORD dwFlags
		);

	static VOID CALLBACK SendRecvData(
		_In_ PVOID   lpParameter,
		_In_ BOOLEAN TimerOrWaitFired
		);

public:
	// Start the network client when we're ready.
	void Start();
	// Shutdown the socket and stop send and recv.
	void Shutdown();

	HRESULT SendCommand(std::shared_ptr<Commands::ICommand> packet);


	// Send a single command to the buffer to be processed.
	template<typename C>
	HRESULT SendCommand(std::shared_ptr<C> packet)
	{
		auto cshFQ = critsec_helper::critsec_helper(&FrontOutboundLock);
		m_OutboundQueueFront.emplace(packet->Encode());
		cshFQ.leave_early();
		return S_OK;
	}

	HRESULT SendNow();
	HRESULT RecvNow();

	std::shared_ptr<Commands::ICommand> ReceiveCommand();

	HRESULT SendCommands(std::vector<std::shared_ptr<Commands::ICommand>> &packet);

	std::vector<std::shared_ptr<Commands::ICommand>>& ReceiveCommands();

	static HANDLE SendRecvEvent;
	static VOID CALLBACK IoCompletionCallback(
		_Inout_     PTP_CALLBACK_INSTANCE Instance,
		_Inout_opt_ PVOID                 Context,
		_Inout_opt_ PVOID                 Overlapped,
		_In_        ULONG                 IoResult,
		_In_        ULONG_PTR             NumberOfBytesTransferred,
		_Inout_     PTP_IO                Io
		);
};


struct NetClietCallback
{
	HANDLE rstHandle;
	NetworkClient *netclient;
	std::queue<std::shared_ptr<std::vector<char>>> *sendqueue;
	LPWSABUF queue;
};


#endif // !NETCLIENT
