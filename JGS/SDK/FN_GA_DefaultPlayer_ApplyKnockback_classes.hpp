#pragma once

// Fortnite (2.4.2) SDK

#ifdef _MSC_VER
	#pragma pack(push, 0x8)
#endif

#include "../SDK.hpp"

namespace SDK
{
//---------------------------------------------------------------------------
//Classes
//---------------------------------------------------------------------------

// BlueprintGeneratedClass GA_DefaultPlayer_ApplyKnockback.GA_DefaultPlayer_ApplyKnockback_C
// 0x0000 (0x0B89 - 0x0B89)
class UGA_DefaultPlayer_ApplyKnockback_C : public UGAB_GenericApplyKnockback_C
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_ApplyKnockback.GA_DefaultPlayer_ApplyKnockback_C");
		return ptr;
	}

};


}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
