#pragma once  

//以下的InitialLoadParam及其派生类都是全局创建一次永不销毁
//所有传入const char*的位置不可传入临时对象（如std::string运算的结果）
//可以直接传入稳定的全局对象或常量。
//如果创建了临时对象作为参数，请套一层strdup再传
//直接传入临时对象会导致崩溃

struct InitialLoadParam //IH不持有所有权
{
	size_t size;
	InitialLoadParam() = default;
	explicit InitialLoadParam(size_t sz);
};

struct IHInitialLoadService
{
	const char* Service;
	const InitialLoadParam* Param;
	int Reserved[6];
};

class UseRelativePath_t {} static UseRelativePath;
struct InitialLoadParam_CustomPath : public InitialLoadParam
{
	const char* Path{ nullptr };

	InitialLoadParam_CustomPath() = delete;
	explicit InitialLoadParam_CustomPath(const char* _Path);// e.g. "C:\\WTF\\"
	explicit InitialLoadParam_CustomPath(UseRelativePath_t, const char* _Path);// e.g. "\\WTF\\"
};

struct InitialLoadParam_CustomFile : public InitialLoadParam
{
	const char* FileName{ nullptr };// e.g. "ra2md.csf" or ini/json

	InitialLoadParam_CustomFile() = delete;
	explicit InitialLoadParam_CustomFile(const char* _Name);
};

struct InitialLoadParam_StringTablePair : public InitialLoadParam
{
	const char* Key{ nullptr };
	const wchar_t* Value{ nullptr };
	const char* Extra{ nullptr };

	InitialLoadParam_StringTablePair() = delete;
	explicit InitialLoadParam_StringTablePair(const char* _Key, const wchar_t* _Value, const char* _Extra = "");
};

struct InitialLoadParam_RedirectFile : public InitialLoadParam
{
	const char* OriginalName{ nullptr };
	const char* TargetName{ nullptr };

	InitialLoadParam_RedirectFile() = delete;
	explicit InitialLoadParam_RedirectFile(const char* _OldName, const char* _NewName);
};

using vptr_t = unsigned int;

struct InitialLoadParam_RegisterVTable : public InitialLoadParam
{
	const char* Name{ nullptr };
	vptr_t vptr{ 0 };
	size_t Size{ 0 };

	InitialLoadParam_RegisterVTable() = delete;
	explicit InitialLoadParam_RegisterVTable(const char* _Name, vptr_t _vptr, size_t Size);
};

struct InitialLoadParam_RegisterFunction : public InitialLoadParam
{
	const char* Name{ nullptr };
	void* Handle{ 0 };

	InitialLoadParam_RegisterFunction() = delete;
	explicit InitialLoadParam_RegisterFunction(const char* _Name, void* _Hd);
};

template<typename T>
struct InitialLoadParam_RegisterObject : public InitialLoadParam
{
	const char* Name { nullptr };
	size_t ObjSize { 0 };
	T* Handle { nullptr };

	InitialLoadParam_RegisterObject() = delete;
	explicit InitialLoadParam_RegisterObject( const char* _Name , T& _Object )
		: Name( _Name ) , ObjSize( sizeof( T ) ) , Handle( new T { _Object } ) ,
		InitialLoadParam( sizeof( InitialLoadParam_RegisterObject ) ) {}
};