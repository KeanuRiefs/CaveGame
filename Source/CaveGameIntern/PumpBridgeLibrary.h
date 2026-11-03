#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PumpBridgeLibrary.generated.h"

/** Blueprint-accessible TCP connection to the local Python pump bridge. */
UCLASS()
class CAVEGAMEINTERN_API UPumpBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Connects to the TCP bridge. For the local mock, use 127.0.0.1 and port 8765. */
	UFUNCTION(BlueprintCallable, Category = "Pump Bridge", meta = (DisplayName = "Connect Pump Bridge"))
	static bool ConnectPumpBridge(const FString& Host, int32 Port);

	/** Sends HOLD or RELEASE over the current connection. */
	UFUNCTION(BlueprintCallable, Category = "Pump Bridge", meta = (DisplayName = "Set Pump Active"))
	static bool SetPumpActive(bool bActive);

	/** Closes the connection. The Python bridge also treats disconnect as RELEASE. */
	UFUNCTION(BlueprintCallable, Category = "Pump Bridge", meta = (DisplayName = "Disconnect Pump Bridge"))
	static void DisconnectPumpBridge();
};
