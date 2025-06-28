#include "IH.File.h"
#include "IH.Loader.h"

bool IHFileClass::OpenEx( const char* pFileName , FileAccessMode access )
{
	this->SetFileName( pFileName );
	return this->Open( access );
}

void IHFileClass::CDCheck( DWORD errorCode , bool bUnk , const char* pFilename )
{
	//ERROR if(errorCode != 0)
}

BOOL IHReadOnlyFileClass::CreateFile()
{
	//ERROR
	return FALSE;
}
BOOL IHReadOnlyFileClass::DeleteFile()
{
	//ERROR
	return FALSE;
}
int IHReadOnlyFileClass::WriteBytes( void* pBuffer , int nNumBytes )
{
	//ERROR
	return 0;
}