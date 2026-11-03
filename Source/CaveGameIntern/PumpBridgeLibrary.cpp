#include "PumpBridgeLibrary.h"

#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
	FSocket* PumpSocket = nullptr;

	void ClosePumpSocket()
	{
		if (PumpSocket)
		{
			PumpSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(PumpSocket);
			PumpSocket = nullptr;
		}
	}
}

bool UPumpBridgeLibrary::ConnectPumpBridge(const FString& Host, int32 Port)
{
	ClosePumpSocket();

	if (Port < 1 || Port > 65535)
	{
		UE_LOG(LogTemp, Error, TEXT("Pump bridge port is out of range: %d"), Port);
		return false;
	}

	FIPv4Address IpAddress;
	if (!FIPv4Address::Parse(Host, IpAddress))
	{
		UE_LOG(LogTemp, Error, TEXT("Pump bridge host must be an IPv4 address: %s"), *Host);
		return false;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	PumpSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("CaveGamePumpBridge"), false);
	if (!PumpSocket)
	{
		return false;
	}

	PumpSocket->SetNoDelay(true);
	TSharedRef<FInternetAddr> Address = SocketSubsystem->CreateInternetAddr();
	Address->SetIp(IpAddress.Value);
	Address->SetPort(Port);

	if (!PumpSocket->Connect(*Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not connect to pump bridge at %s:%d"), *Host, Port);
		ClosePumpSocket();
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Connected to pump bridge at %s:%d"), *Host, Port);
	return true;
}

bool UPumpBridgeLibrary::SetPumpActive(bool bActive)
{
	if (!PumpSocket || PumpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pump bridge is not connected; command was not sent."));
		return false;
	}

	const FString Command = bActive ? TEXT("HOLD\n") : TEXT("RELEASE\n");
	FTCHARToUTF8 Utf8Command(*Command);
	int32 BytesSent = 0;
	const bool bSent = PumpSocket->Send(
		reinterpret_cast<const uint8*>(Utf8Command.Get()), Utf8Command.Length(), BytesSent);
	if (!bSent || BytesSent != Utf8Command.Length())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to send full pump bridge command."));
		ClosePumpSocket();
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Sent pump command: %s"), bActive ? TEXT("HOLD") : TEXT("RELEASE"));
	return true;
}

void UPumpBridgeLibrary::DisconnectPumpBridge()
{
	if (PumpSocket && PumpSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
	{
		SetPumpActive(false);
	}
	ClosePumpSocket();
}
