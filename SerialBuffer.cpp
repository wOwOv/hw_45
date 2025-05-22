#include "SerialBuffer.h"
#include <windows.h>


SBuffer::SBuffer():read(0),write(0),BufferSize(BUFFER_DEFAULT),DataSize(0)
{
	//기본 버퍼 사이즈 할당
	buffer = (char*)malloc(BUFFER_DEFAULT);
}
SBuffer::SBuffer(int size) :read(0), write(0),BufferSize(size),DataSize(0)
{
//인자의 BufferSize만큼 할당
	buffer = (char*)malloc(BufferSize);
}

SBuffer::~SBuffer()
{
	free(buffer);
}


	// 버퍼 청소.
void SBuffer::Clear(void)
{
	read = 0;
	write = 0;
	DataSize = 0;
}

	// 버퍼 사이즈 얻기
	int	SBuffer::GetBufferSize(void)
	{ 
		return BufferSize;
	}

	// 현재 사용중인 사이즈 얻기.
	int	SBuffer::GetDataSize(void)
	{ 
		return DataSize;
	}
	 


	// 버퍼 포인터 얻기
	char* SBuffer::GetBufferPtr(void)
	{
		return &buffer[read];
	}


	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::MoveWritePos(int size)
	{
		write += size;
		DataSize += size;
		return size;
	}
	int	SBuffer::MoveReadPos(int size)
	{
		read += size;
		DataSize -= size;
		return size;
	}






	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	SBuffer& SBuffer::operator = (SBuffer& srcbuffer)
	{
		memcpy(GetBufferPtr(), srcbuffer.buffer, srcbuffer.GetBufferSize());
		read = srcbuffer.read;
		write = srcbuffer.write;
		DataSize = srcbuffer.DataSize;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& SBuffer::operator << (unsigned char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[write];
			*temp = value;
			write+=sizeof(value);
			DataSize+= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator<<(signed char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator << (BYTE value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/




	SBuffer& SBuffer::operator << (short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (unsigned short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator << (WORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/


	SBuffer& SBuffer::operator << (unsigned int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (long value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (float value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (DWORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator << (__int64 value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (double value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& SBuffer::operator >> (unsigned char& value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[read];
			value=*temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator>>(signed char& value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (char &value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator >> (BYTE& value)
	{
		if (DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/




	SBuffer& SBuffer::operator >> (short &value)
	{
		if (DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	
	SBuffer& SBuffer::operator >> (unsigned short &value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator >> (WORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/



	SBuffer& SBuffer::operator>>(unsigned int value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator >> (int &value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (long &value)
	{
		if (DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (float &value)
	{
		if (DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (DWORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator >> (__int64 &value)
	{
		if (DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (double &value)
	{
		if (DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}




	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::GetData(char* dest, int size)
	{

		//사용중인 공간이 0일 때
		if (DataSize == 0)
		{
			return 0;
		}

		//사용중인 공간이 빼줄 데이터양보다 클 때
		if (DataSize >= size)
		{
			memcpy(dest, &buffer[read], size);
			read += size;
			DataSize -= size;
			return size;
		}
		//사용중인 공간이 빼줄 데이터양보다 작아서 그냥 있는거 다 빼줄때..?
		else
		{
			memcpy(dest, &buffer[read], DataSize);
			read += DataSize;
			DataSize = 0;
			return DataSize;
		}


	}

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::PutData(char* src, int srcsize)
	{
	//넣을 수 있는 공간이 없을때
		if (DataSize == BufferSize)
		{
			return 0;
		}
		//넣을 수 있는 공간이 충분할 때
		if (BufferSize - DataSize >= srcsize)
		{
			memcpy(&buffer[write], src, srcsize);
			write += srcsize;
			DataSize += srcsize;
			return srcsize;
		}
		//넣을 수 잇는 공간이 부족할 때 최대한 넣어주기
		else
		{
			int size = BufferSize - DataSize;
			memcpy(&buffer[write], src, size);
			write += size;
			DataSize += size;
			return size;
		}
	}









