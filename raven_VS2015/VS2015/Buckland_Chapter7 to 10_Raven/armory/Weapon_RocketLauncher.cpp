#include "Weapon_RocketLauncher.h"
#include "../Raven_Bot.h"
#include "misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RocketLauncher::RocketLauncher(Raven_Bot*   owner):

                      Raven_Weapon(type_rocket_launcher,
                                   script->GetInt("RocketLauncher_DefaultRounds"),
                                   script->GetInt("RocketLauncher_MaxRoundsCarried"),
                                   script->GetDouble("RocketLauncher_FiringFreq"),
                                   script->GetDouble("RocketLauncher_IdealRange"),
                                   script->GetDouble("Rocket_MaxSpeed"),
								   script->GetDouble("Rocket_Deceleration"),		  
                                   owner)
{
    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -3),
                                           Vector2D(6, -3),
                                           Vector2D(6, -1),
                                           Vector2D(15, -1),
                                           Vector2D(15, 1),
                                           Vector2D(6, 1),
                                           Vector2D(6, 3),
                                           Vector2D(0, 3)
                                           };
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void RocketLauncher::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire off a rocket!
    m_pOwner->GetWorld()->AddRocket(m_pOwner, pos);

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RocketLauncher_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RocketLauncher::GetDesirability(double DistToTarget)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

double RocketLauncher::GetRangeDeceleration(double DistToTarget)
{
	if (DistToTarget > (0.6 * GetIdealRange()))
	{
		return script->GetDouble("Rocket_Deceleration");
	}
	else
	{
		return 0.0;
	}
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RocketLauncher::InitializeFuzzyModule()
{
  FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

  FzSet& Target_Close = DistToTarget.AddLeftShoulderSet("Target_Close",0,25,150);
  FzSet& Target_MediumClose = DistToTarget.AddTriangularSet("Target_MediumClose",25,150,300);
  FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium", 150, 300, 500);
  FzSet& Target_MediumFar = DistToTarget.AddTriangularSet("Target_MediumFar", 300, 500, 700);
  FzSet& Target_Far = DistToTarget.AddRightShoulderSet("Target_Far",500,700,1000);

  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability"); 

  FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 15, 30);
  FzSet& LessDesirable = Desirability.AddTriangularSet("LessDesirable", 15, 30, 50);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 30, 50, 65);
  FzSet& MoreDesirable = Desirability.AddTriangularSet("MoreDesirable", 50, 65, 80);
  FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 65, 80, 100);
  
 

  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");

  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 0, 10);
  FzSet& Ammo_OkayLow = AmmoStatus.AddTriangularSet("Ammo_OkayLow", 0, 10, 30);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 30, 50);
  FzSet& Ammo_OkayLoads = AmmoStatus.AddTriangularSet("Ammo_OkayLoads", 30, 50, 70);
  FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 50, 70, 100);
  
  
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_OkayLow), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_OkayLoads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_MediumClose, Ammo_Low), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumClose, Ammo_OkayLow), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumClose, Ammo_Okay), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumClose, Ammo_OkayLoads), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumClose, Ammo_Loads), Desirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_OkayLow), MoreDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), MoreDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_OkayLoads), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_MediumFar, Ammo_Low), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumFar, Ammo_OkayLow), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumFar, Ammo_Okay), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumFar, Ammo_OkayLoads), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_MediumFar, Ammo_Loads), MoreDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_OkayLow), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_OkayLoads), LessDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);


  //m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
 // m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
  //m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);

  //m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);
  //m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), VeryDesirable);
  //m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), Desirable);

  //m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
  //m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), Undesirable);
  //m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RocketLauncher::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->RedPen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}