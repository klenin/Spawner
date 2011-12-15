#ifndef _RESTRICTIONS_H_
#define _RESTRICTIONS_H_

#include "processproxy.h"

//may be rename to rules?..

typedef enum
{
	RESTRICTION_USER_TIME,
	RESTRICTION_MEMORY,

} RESTRICTION_KIND;

typedef unsigned int restriction_t;


/**
 * Base restriction class
 *
 * 
*/
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


#endif//_RESTRICTIONS_H_
