#pragma once

struct FNetworkObjectInfo
{
	AActor* Actor;
	double NextUpdateTime;
	double LastNetReplicateTime;
	float OptimalNetUpdateDelta;
	float LastNetUpdateTime;
	uint32_t bPendingNetUpdate : 1;
	uint32_t bForceRelevantNextUpdate : 1;
	std::vector<UNetConnection*> DormantConnections;
	std::vector<UNetConnection*> RecentlyDormantConnections;
};

inline int32_t Rand() { return rand(); };
inline float FRand() { return Rand() / (float)RAND_MAX; };

namespace Replication
{
	inline static UChannel* (*CreateChannel)(UNetConnection*, int, bool, int32_t);
	inline static __int64 (*ReplicateActor)(UActorChannel*);
	inline static __int64 (*SetChannelActor)(UActorChannel*, AActor*);
	inline static void (*CallPreReplication)(AActor*, UNetDriver*);
	inline static void (*SendClientAdjustment)(APlayerController*);
	inline static void (*ActorChannelClose)(UActorChannel*);

	UActorChannel* ReplicateToClient(AActor* InActor, UNetConnection* InConnection)
	{
		if (InActor->IsA(APlayerController::StaticClass()) && InActor != InConnection->PlayerController)
			return nullptr;

		auto Channel = (UActorChannel*)(CreateChannel(InConnection, 2, true, -1));

		if (Channel) {
			SetChannelActor(Channel, InActor);
			Channel->Connection = InConnection;

			return Channel;
		}

		return NULL;
	}

	int PrepConnections(UNetDriver* NetDriver)
	{
		int ReadyConnections = 0;

		for (int ConnIdx = 0; ConnIdx < NetDriver->ClientConnections.Num(); ConnIdx++)
		{
			UNetConnection* Connection = NetDriver->ClientConnections[ConnIdx];
			if (!Connection) continue;

			AActor* OwningActor = Connection->OwningActor;

			if (OwningActor)
			{
				ReadyConnections++;
				AActor* DesiredViewTarget = OwningActor;

				if (Connection->PlayerController)
				{
					if (AActor* ViewTarget = Connection->PlayerController->GetViewTarget())
					{
						DesiredViewTarget = ViewTarget;
					}
				}

				Connection->ViewTarget = DesiredViewTarget;

				for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
				{
					UNetConnection* ChildConnection = Connection->Children[ChildIdx];
					if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
					{
						ChildConnection->ViewTarget = DesiredViewTarget;
					}
				}
			}
			else
			{
				Connection->ViewTarget = nullptr;

				for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
				{
					UNetConnection* ChildConnection = Connection->Children[ChildIdx];
					if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
					{
						ChildConnection->ViewTarget = nullptr;
					}
				}
			}
		}

		return ReadyConnections;
	}

	UActorChannel* FindChannel(AActor* Actor, UNetConnection* Connection)
	{
		for (int i = 0; i < Connection->OpenChannels.Num(); i++)
		{
			auto Channel = Connection->OpenChannels[i];

			if (Channel && Channel->Class)
			{
				if (Channel->Class == UActorChannel::StaticClass())
				{
					if (((UActorChannel*)Channel)->Actor == Actor)
						return ((UActorChannel*)Channel);
				}
			}
		}

		return NULL;
	}

	UNetConnection* GetOwningConnection(AActor* Actor)
	{
		for (auto Owner = Actor; Actor; Actor = Actor->GetOwner())
		{
			if (Actor->IsA(APlayerController::StaticClass()))
			{
				return ((APlayerController*)Actor)->NetConnection;
			}
		}
	}

	void BuildConsiderList(UNetDriver* NetDriver, std::vector<FNetworkObjectInfo*>& OutConsiderList)
	{
		TArray<AActor*> Actors;
		Globals::GPS->STATIC_GetAllActorsOfClass(Globals::World, AActor::StaticClass(), &Actors);

		for (int i = 0; i < Actors.Num(); i++)
		{
			auto Actor = Actors[i];

			if (!Actor)
				continue;

			if (!Actor)
			{
				continue;
			}

			if (Actor->bActorIsBeingDestroyed)
			{
				continue;
			}

			if (Actor->RemoteRole == ENetRole::ROLE_None)
			{
				continue;
			}

			if (Actor->NetDormancy == ENetDormancy::DORM_Initial && Actor->bNetStartup)
			{
				continue;
			}

			if (Actor->Name.ComparisonIndex != 0)
			{
				if (CallPreReplication)
					CallPreReplication(Actor, NetDriver);
				
				FNetworkObjectInfo* Info = new FNetworkObjectInfo();
				Info->Actor = Actor;

				OutConsiderList.push_back(Info);
			}
		}

		FreeInternal((__int64)Actors.Data);
		Actors.Data = 0;
		Actors.Count = 0;
		Actors.Max = 0;
	}

	void ReplicateActors(UNetDriver* NetDriver)
	{
		++* (DWORD*)(__int64(NetDriver) + 712);

		auto NumClientsToTick = PrepConnections(NetDriver);

		if (NumClientsToTick == 0)
			return;

		std::vector<FNetworkObjectInfo*> ConsiderList;
		BuildConsiderList(NetDriver, ConsiderList);

		for (int i = 0; i < NetDriver->ClientConnections.Num(); i++)
		{
			auto Connection = NetDriver->ClientConnections[i];

			if (!Connection)
				continue;

			if (i >= NumClientsToTick)
				continue;

			if (Connection->PlayerController)
			{
				SendClientAdjustment(Connection->PlayerController);
			}

			for (int32_t ChildIdx = 0; ChildIdx < Connection->Children.Num(); ChildIdx++)
			{
				if (Connection->Children[ChildIdx]->PlayerController != NULL)
				{
					SendClientAdjustment(Connection->Children[ChildIdx]->PlayerController);
				}
			}

			for (auto ActorInfo : ConsiderList)
			{
				if (!ActorInfo)
					continue;

				if (!ActorInfo->Actor)
					continue;

				auto Channel = FindChannel(ActorInfo->Actor, Connection);

				if (!Channel)
					Channel = ReplicateToClient(ActorInfo->Actor, Connection);

				if (Channel)
				{
					if (ReplicateActor(Channel))
					{
					}
				}
			}
		}

		ConsiderList.empty();
		ConsiderList.clear();
	}

	void InitOffsets()
	{
		auto BaseAddress = (uintptr_t)GetModuleHandle(NULL);

		CreateChannel = decltype(CreateChannel)(BaseAddress + Offsets::CreateChannel);
		SetChannelActor = decltype(SetChannelActor)(BaseAddress + Offsets::SetChannelActor);
		ReplicateActor = decltype(ReplicateActor)(BaseAddress + Offsets::ReplicateActor);
		CallPreReplication = decltype(CallPreReplication)(BaseAddress + Offsets::CallPreReplication);
		SendClientAdjustment = decltype(SendClientAdjustment)(BaseAddress + Offsets::SendClientAdjustment);
		ActorChannelClose = decltype(ActorChannelClose)(BaseAddress + Offsets::ActorChannelClose);
	}
}