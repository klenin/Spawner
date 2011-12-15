#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include "restrictions.h"
#include "processproxy.h"

class CProcess
{
public:
	CProcess();
	void SetRestrictions(CRestriction *restriction)
	{
		if (!restriction)
			return;
		restrictions.push_back(restriction);
	}
	void SetArguments(); // ?!
	int Run(char *argv[]);
	~CProcess();
protected:
	void apply_restrictions()
	{
		for (std::list<CRestriction*>::iterator i = restrictions.begin();
			i != restrictions.end(); ++i)
		{
			(*i)->ApplyRestriction(proxy);
		}
	}
	std::list<CRestriction*> restrictions;
	CProcessProxy proxy;
};


#endif//_PROCESS_H_
