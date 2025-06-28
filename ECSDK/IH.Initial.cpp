#include "IH.Initial.h"
#include "SyringeEx.h"

//以下的InitialLoadParam及其派生类都是全局创建一次永不销毁
//所以尽管瞎分配内存就行

InitialLoadParam::InitialLoadParam( size_t sz )
	:size( sz )
{};

InitialLoadParam_CustomPath::InitialLoadParam_CustomPath( const char* _Path )
	: Path( _Path ) ,
	InitialLoadParam( sizeof( InitialLoadParam_CustomPath ) )
{}
InitialLoadParam_CustomPath::InitialLoadParam_CustomPath( UseRelativePath_t , const char* _Path )
	: Path( _strdup( ( SyringeData::ExecutableDirectoryPath() + _Path ).c_str() ) ) ,
	InitialLoadParam( sizeof( InitialLoadParam_CustomPath ) )
{}

InitialLoadParam_CustomFile::InitialLoadParam_CustomFile( const char* _Name )
	: FileName( _Name ) ,
	InitialLoadParam( sizeof( InitialLoadParam_CustomFile ) )
{}

InitialLoadParam_StringTablePair::InitialLoadParam_StringTablePair( const char* _Key , const wchar_t* _Value , const char* _Extra )
	: Key( _Key ) , Value( _Value ) , Extra( _Extra ) ,
	InitialLoadParam( sizeof( InitialLoadParam_StringTablePair ) )
{}

InitialLoadParam_RedirectFile::InitialLoadParam_RedirectFile( const char* _OldName , const char* _NewName )
	: OriginalName( _OldName ) , TargetName( _NewName ) ,
	InitialLoadParam( sizeof( InitialLoadParam_RedirectFile ) )
{}

InitialLoadParam_RegisterVTable::InitialLoadParam_RegisterVTable( const char* _Name , vptr_t _vptr , size_t size )
	: Name( _Name ) , vptr( _vptr ) , Size( size ) ,
	InitialLoadParam( sizeof( InitialLoadParam_RegisterVTable ) )
{}

InitialLoadParam_RegisterFunction::InitialLoadParam_RegisterFunction( const char* _Name , void* _Hd )
	: Name( _Name ) , Handle( _Hd ) ,
	InitialLoadParam( sizeof( InitialLoadParam_RegisterFunction ) )
{}