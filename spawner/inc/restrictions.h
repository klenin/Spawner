#ifndef _RESTRICTIONS_H_
#define _RESTRICTIONS_H_

typedef enum
{
	RESTRICTION_USER_TIME_LIMIT         = 0x0,
	RESTRICTION_MEMORY_LIMIT            = 0x1,
	RESTRICTION_PROCESSOR_TIME_LIMIT    = 0x2,
	RESTRICTION_SECURITY_LIMIT          = 0x3,
	RESTRICTION_WRITE_LIMIT             = 0x4,
    RESTRICTION_MAX                     = 0x5

} RESTRICTION_KIND;

typedef unsigned int restriction_t;


const restriction_t RESTRICTION_NO_LIMIT = 0xffffffff;
const restriction_t RESTRICTION_LIMITED  = 0x00000001;


/**
 * Base restriction class
 *
 * 

struct restriction_s
{
    RESTRICTION_KIND    kind;
    restriction_t       value;
};
class CRestriction
{
public:
	CRestriction(const RESTRICTION_KIND &k, const restriction_t &value):
		kind(k), restriction(value)
	{

	}
	virtual void ApplyRestriction(CProcessProxy &proxy) = 0;
	RESTRICTION_KIND Kind()
	{
		return kind;
	}

protected:
	RESTRICTION_KIND kind;
	restriction_t restriction;
};

class CMemoryRestriction: public CRestriction
{
public:
	CMemoryRestriction(const restriction_t &value): 
		CRestriction(RESTRICTION_MEMORY, value)
	{}
	virtual void ApplyRestriction(CProcessProxy &proxy);
protected:
private:
};

*/
#endif//_RESTRICTIONS_H_
