#ifndef PROFILE
#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName)	ProfileBegin(TagName)
#define PRO_END(TagName)	ProfileEnd(TagName)

#elif
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

void ProfileBegin(const char* szName);
void ProfileEnd(const char* szName);

void ProfileEndA(const char* szName);
void ProfileEndS(const char* szName);

void ProfileDataOutText(void);
void ProfileReset(void);

class Profile
{
public:
	Profile(const char* tag)
	{
		PRO_BEGIN(tag);
		_tag = tag;
	}
	~Profile()
	{
		PRO_END(_tag);
	}

	const char* _tag;
};

#endif