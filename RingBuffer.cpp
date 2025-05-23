#include "RingBuffer.h"
#include <malloc.h>
#include <windows.h>

//링버퍼 생성자 - 기본 할당(SIZE만큼)
RingBuffer::RingBuffer():_front(0),_rear(0),_size(DEF)
{
	_buffer = (char*)malloc(sizeof(char) * DEF);
}

//링버퍼 생성자 - 원하는 크기 할당
RingBuffer::RingBuffer(int buffersize) :_front(0), _rear(0), _size(buffersize)
{
	_buffer = (char*)malloc(sizeof(char) * buffersize);
}

//링버퍼 소멸자
RingBuffer::~RingBuffer()
{
	free(_buffer);
}

//할당 새로해서 복사, _front, _rear 재세팅
void RingBuffer::Resize(int size)
{
	int deqret;
	char* temp = new char[_size];
	int usedsize = GetUsedSize();
	deqret = Dequeue(temp, usedsize);
	if (deqret != usedsize)
	{
		DebugBreak();
	}
	free(_buffer);
	_buffer = new char[size];
	memcpy(_buffer, temp, deqret);
	_front = 0;
	_rear = deqret;
}


//링버퍼의 전체 사이즈 얻기
int RingBuffer::GetBufferSize()
{
	return _size-1;
}
//링버퍼의 사용중 크기
int RingBuffer::GetUsedSize()
{
	int fixedfront = _front;
	int fixedrear = _rear;

	if (fixedfront <= fixedrear)
	{
		return fixedrear - fixedfront;
	}
	else
	{
		return _size - (fixedfront - fixedrear);
	}
}
//링버퍼의 미사용 크기
int RingBuffer::GetFreeSize()
{
	return (_size - GetUsedSize() - 1);
}


//링버퍼는 원형큐인거니까 최선을 다해 넣어주는걸로
// 일단은 안 들어가면 넣지말자로...
//링버퍼에 인큐
int RingBuffer::Enqueue(const char* data, int size)
{
	int freesize = GetFreeSize();
	if (freesize >= size)
	{
		if (_rear < _front)
		{
			memcpy(&_buffer[_rear], data, size);
			_rear = (_rear + size) % _size;
			return size;
		}
		else
		{
			if ((_size - _rear) >= size)
			{
				memcpy(&_buffer[_rear], data, size);
				_rear = (_rear + size) % _size;
				return size;
			}

			int remainbyte = size;
			int movebyte = _size - _rear;
			memcpy(&_buffer[_rear], data, movebyte);
			data += movebyte;
			remainbyte -= movebyte;
			memcpy(&_buffer[0], data, remainbyte);
			_rear = (_rear + size) % _size;
			return size;
		}
	}
	else
	{
		return 0;
	}
}
//그냥 최대한 있는거 주기                         ------O
//일단 그 size만큼 못 주면 return 0;...          ------X
int RingBuffer::Dequeue(char* dest, int size)
{
	if (GetUsedSize() >= size)
	{
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], size);
			_front = (_front + size) % _size;
			return size;
		}
		else
		{
			if ((_size - _front) >= size)
			{
				memcpy(dest, &_buffer[_front], size);
				_front = (_front + size) % _size;
				return size;
			}
			int remainbyte = size;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			_front = (_front + size) % _size;
			return size;
		}
	}
	else
	{
		//return 0;

		int usedsize = GetUsedSize();
		if (usedsize == 0)
		{
			return 0;
		}
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], usedsize);
			_front = (_front + usedsize) % _size;
			return usedsize;
		}
		else
		{
			int remainbyte = usedsize;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			_front = (_front + usedsize) % _size;
			return usedsize;
		}
	}
}

//최대한 주기
//데이터는 주지만 front를 움직이지는 않음
int RingBuffer::Peek(char* dest, int size)
{
	if (GetUsedSize() >= size)
	{
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], size);
			return size;
		}
		else
		{
			if ((_size - _front) >= size)
			{
				memcpy(dest, &_buffer[_front], size);
				return size;
			}
			int remainbyte = size;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			return size;
		}
	}
	else
	{
		//return 0;

		int usedsize = GetUsedSize();
		if (usedsize == 0)
		{
			return 0;
		}
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], usedsize);
			return usedsize;
		}
		else
		{
			int remainbyte = usedsize;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			return usedsize;
		}
	}
}

//버퍼의 모든 데이터 삭제
void RingBuffer::ClearBuffer()
{
	_front = _rear = 0;
}

int RingBuffer::DirectEnqueueSize()
{
	if (_rear < _front)
	{
		return _front - _rear - 1;
	}
	else
	{
		if (_front == 0)
		{
			return _size - _rear - 1;
		}
		else
		{
		return _size - _rear;
		}
		
	}
	
}
int RingBuffer::DirectDequeueSize()
{
		if (_front < _rear)
		{
			return _rear - _front;
		}
		else
		{
			return _size - _front;
		}
}

//링버퍼의 rear 변경
int RingBuffer::MoveRear(int size)
{
	if (this->GetFreeSize() >= size)
	{
		_rear = (_rear + size) % _size;
		return size;
	}
	else
	{
		if (_rear <= _front)
		{
			int byte = _front - _rear - 1;
			_rear = _front - 1;
			return byte;
		}
		else
		{
			int byte = (_size - _rear) + _front - 1;
			_rear = _front - 1;
			return byte;
		}
	}
}
//링버퍼의 front 변경
int RingBuffer::MoveFront(int size)
{
	if (this->GetUsedSize() >= size)
	{
		_front = (_front + size) % _size;
		return size;
	}
	else
	{
		if (_front <= _rear)
		{
			int byte = _rear - _front;
			_front = _rear;
			return byte;
		}
		else
		{
			int byte = (_size - _front) + _rear;
			_front = _rear;
			return byte;
		}
	}
}
//버퍼의 front포인터 얻기
char* RingBuffer::GetFrontBufferPtr()
{
	return &_buffer[_front];
}
//버퍼의 rear포인터 얻기
char* RingBuffer::GetRearBufferPtr()
{
	return &_buffer[_rear];
}
//버퍼의 첫 요소 포인터 얻기
char* RingBuffer::GetStartBufferPtr()
{
	return &_buffer[0];
}
