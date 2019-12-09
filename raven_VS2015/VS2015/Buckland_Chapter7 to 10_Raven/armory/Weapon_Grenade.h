#ifndef GRENADE_H
#define GRENADE_H
#pragma warning (disable:4786)
//-----------------------------------------------------------------------------
//
//  Name:   Grenade
////
//  Desc:   class to implement a grenade
//-----------------------------------------------------------------------------
#include "Raven_Weapon.h"



class  Raven_Bot;

class Grenade : public Raven_Weapon
{
private:

	void     InitializeFuzzyModule();

public:

	Grenade(Raven_Bot* owner);
	
	void Render();

	void ShootAt(Vector2D pos);

	double GetDesirability(double DistToTarget);

	double GetRangeDeceleration(double DistToTarget);
};



#endif