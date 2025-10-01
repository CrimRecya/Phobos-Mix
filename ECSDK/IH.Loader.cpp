#include "IH.Loader.h"
#include "IH.Config.h"
#include "IH.Initial.h"
#include "IH.InitialService.h"
#include "EC.Misc.h"
#include "SyringeEx.h"
#include <Windows.h>
#include <string_view>

#ifndef IHCore
void MyInit( InitResult& Result );
#endif

namespace SyringeData
{
	void InitRemoteData();
}

LibFuncHandle __cdecl ListenerAccess( const char* Name );

namespace Init
{
	bool InitAvailable { false };
	bool VeryFirst { true };
	HMODULE LibListDLL;
	InitInput* LibInput;
	InitResult Result;

	bool( __cdecl* _RegisterEntry )( LibFuncHandle );
	const LibFuncHandle* ( __cdecl* _GetEntries )( void );
	void( __cdecl* _ServiceRequest )( IHInitialLoadService );
	void( __cdecl* _QueryServiceRequest )( const char* , PArray<IHInitialLoadService>& );
	ConfData* ( __cdecl* _GetConfData )( void );
	unsigned int( __cdecl* _RandomUINT )( void );

	bool IsSyringeReadingHooks()
	{
		char Buf[1000];
		return GetEnvironmentVariableA( "HERE_IS_SYRINGE" , Buf , 1000 ) ? true : false;
	}

#ifndef IHCore
	InitResult* __cdecl InitFn( InitInput* Input )
	{
		LibInput = Input;
		MyInit( Result );
		return &Result;
	}

	void RegisterListenerImpl()
	{
		InitialLoad::CreateRequestAndSubmit<InitialLoadParam_RegisterFunction>( "EC::Internal::ListenerAccess" , _strdup( RandStr( 12 ).c_str() ) , ListenerAccess );
	}

	bool Initialize()
	{
		if( IsSyringeReadingHooks() )return false;

		SyringeData::InitRemoteData();

		if( InitAvailable )return true;

		LibListDLL = LoadLibraryW( L"Patches\\IHLibList.dll" );
		if( LibListDLL == NULL )LibListDLL = LoadLibraryW( L"IHLibList.dll" );
		if( LibListDLL == NULL )return false;

#define ____GetFunc(x) {_ ## x=(decltype(_ ## x))GetProcAddress(LibListDLL, #x);\
			if(_ ## x==nullptr)return false;}
		____GetFunc( RegisterEntry );
		____GetFunc( GetEntries );
		____GetFunc( ServiceRequest );
		____GetFunc( QueryServiceRequest );
		____GetFunc( GetConfData );
		____GetFunc( RandomUINT );
#undef ____GetFunc

		InitAvailable = true;
		RegisterEntry( InitFn );
		return true;
	}
#else
	void RegisterListenerImpl()
	{
		InitialLoad::CreateRequestAndSubmit<InitialLoadParam_RegisterFunction>( "EC::Internal::ListenerAccess" , "EC::Internal::IHCore" , ListenerAccess );
	}

	bool Initialize()
	{
		if( IsSyringeReadingHooks() )return false;

		SyringeData::InitRemoteData();

		if( InitAvailable )return true;

		LibListDLL = LoadLibraryW( L"Patches\\IHLibList.dll" );
		if( LibListDLL == NULL )LibListDLL = LoadLibraryW( L"IHLibList.dll" );
		if( LibListDLL == NULL )return false;

#define ____GetFunc(x) {_ ## x=(decltype(_ ## x))GetProcAddress(LibListDLL, #x);\
			if(_ ## x==nullptr)return false;}
		____GetFunc( ServiceRequest );
		____GetFunc( QueryServiceRequest );
		____GetFunc( GetConfData );
#undef ____GetFunc

		InitAvailable = true;
		RegisterListenerImpl();
		return true;
	}
#endif
	bool __cdecl RegisterEntry( LibFuncHandle Entry )
	{
		if( InitAvailable )return _RegisterEntry( Entry );
		else return false;
	}

	ConfData* GetConfData()
	{
		if( _GetConfData )return _GetConfData();
		else return nullptr;
	}

	PArray<IHInitialLoadService> __cdecl QueryServiceRequest( const char* Name )
	{
		if( InitAvailable )
		{
			PArray<IHInitialLoadService> pa;
			_QueryServiceRequest( Name , pa );
			return pa;
		}
		else return PArray<IHInitialLoadService>();
	}
}

ConfData& GetConfigData()
{
	auto ptr = Init::GetConfData();
	if( !ptr )MessageBoxA( Game::hWnd , "Init::GetConfData() == nullptr" , "EC SDK" , MB_OK );
	return *ptr;
}
void UseOriginalFileClass( bool Option )
{
	GetConfigData().UseOriginalCCFile = Option;
}
bool UseOriginalFileClass()
{
	return GetConfigData().UseOriginalCCFile;
}


namespace InitialLoad
{
	void ServiceRequest( IHInitialLoadService IService )
	{
		if( Init::InitAvailable )Init::_ServiceRequest( IService );
	}
	unsigned int RandomUINT()
	{
		if( Init::_RandomUINT )return Init::_RandomUINT();
		else return ( unsigned ) rand();//regardless of quality when IHLibList version is low
	}
}



bool IHAvailable()
{
	return Init::InitAvailable;
}

