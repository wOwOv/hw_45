#ifndef __SERIALBUFFER__
#define __SERIALBUFFER__

#include <windows.h>

class SBuffer
{
public:

	enum en_CBuffer
	{
		BUFFER_DEFAULT = 1024		// 패킷의 기본 버퍼 사이즈.
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	SBuffer();
	SBuffer(int iBufferSize);

	virtual	~SBuffer();


	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void);
	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void);



	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void);

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);


	//연산자 오버로딩
	SBuffer& operator=(SBuffer& srcbuffer);

	SBuffer& operator << (unsigned char value);
	SBuffer& operator << (signed char value);
	SBuffer& operator << (char value);
	//CBuffer& operator<<(BYTE value);

	SBuffer& operator << (short value);
	SBuffer& operator << (unsigned short value);
	//CBuffer& operator << (WORD value);

	SBuffer& operator << (unsigned int value);
	SBuffer& operator << (int value);
	SBuffer& operator << (long value);
	SBuffer& operator << (float value);
	SBuffer& operator << (DWORD value);

	SBuffer& operator << (__int64 iValue);
	SBuffer& operator << (double dValue);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& operator>>(unsigned char &value);
	SBuffer& operator>>(signed char& value);
	SBuffer& operator>>(char &value);
	//CBuffer& operator>>(BYTE& value);

	SBuffer& operator>>(short &value);
	SBuffer& operator>>(unsigned short &value);
	//CBuffer& operator>>(WORD& value);

	SBuffer& operator >> (unsigned int value);
	SBuffer& operator>>(int &value);
	SBuffer& operator>>(long &value);
	SBuffer& operator>>(float &value);
	SBuffer& operator>>(DWORD& value);

	SBuffer& operator>>(__int64 &value);
	SBuffer& operator>>(double &value);





	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* dest, int size);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* src, int srcsize);



protected:
int	BufferSize;//버퍼 전체 사이즈
	char* buffer;
	int read;
	int write;
	
	int	DataSize;//사용중인 사이즈





	
};








#endif