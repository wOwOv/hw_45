#ifndef __RINGBUFFER__
#define __RINGBUFFER__

#define DEF 30000

class RingBuffer
{
public:
	char* _buffer;
	int _front;
	int _rear;
	int _size;


public:
	RingBuffer();
	RingBuffer(int buffersize);
	~RingBuffer();

	void Resize(int size);
	
	int GetBufferSize();
	int GetUsedSize();
	int GetFreeSize();
	
	int Enqueue(const char* data, int size);
	int Dequeue(char* data, int size);
	int Peek(char* dest, int size);
	
	void ClearBuffer();
	
	int DirectEnqueueSize();
	int DirectDequeueSize();

	int MoveRear(int size);
	int MoveFront(int size);
	char* GetFrontBufferPtr();
	char* GetRearBufferPtr();
	char* GetStartBufferPtr();

private:
	//int MoveRear(int size);
	//int MoveFront(int size);
	//char* GetFrontBufferPtr();
	//char* GetRearBufferPtr();
	//char* GetStartBufferPtr();


};


#endif