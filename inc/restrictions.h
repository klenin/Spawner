#ifndef _RESTRICTIONS_H_
#define _RESTRICTIONS_H_

//may be rename to rules?..

class CRestriction
{
public:
    CRestriction(/*argument*/);
    static void GetRestrictionList(/*arguments*/);
    virtual void ApplyRestriction(/*process*/) = 0;

};

#endif//_RESTRICTIONS_H_
