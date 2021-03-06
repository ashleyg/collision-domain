#ifndef STEERINGBEHAVIOUR_H
#define STEERINGBEHAVIOUR_H

#include "Player.h"
#include "AiPlayer.h"
#include "utils.h"

using namespace Ogre;

class AiPlayer;

enum behaviour_type
	{
		none               = 0x00000,
	    seek               = 0x00002,
	    flee               = 0x00004,
	    powerup            = 0x00008,
	    wander             = 0x00010,
	    cohesion           = 0x00020,
	    separation         = 0x00040,
	    allignment         = 0x00080,
	    obstacle_avoidance = 0x00100,
	    wall_avoidance     = 0x00200,
	    follow_path        = 0x00400,
	    pursuit            = 0x00800,
	    evade              = 0x01000,
	    interpose          = 0x02000,
	    hide               = 0x04000,
	    flock              = 0x08000,
	    offset_pursuit     = 0x10000,
	};

class SteeringBehaviour
{
public:
	SteeringBehaviour(Player* agent);
	~SteeringBehaviour() {};
	Vector3 Calculate();
	void SetTargetPlayer1(Player *agent){mTargetPlayer1 = agent;}
	void SetTargetPlayer2(Player *agent){mTargetPlayer2 = agent;}
	void SetFleeTarget(Player *agent)	{mFleeTarget = agent;}
	void SetSeekTarget(Player *agent)   {mSeekTarget = agent;}
	void SetPowerupTarget(Ogre::Vector3 pos)  {mPowerup = pos;};
	void SetTarget(const Ogre::Vector3 t){mTarget = t;}
	Player* GetFleeTarget() { return mFleeTarget; }
	Player* GetSeekTarget() { return mSeekTarget; }
	Ogre::Vector3 GetPowerup() { return mPowerup;};
	void PowerupOn() {m_iFlags |= powerup;};
	void FleeOn(){m_iFlags |= flee;}
	void SeekOn(){m_iFlags |= seek;}
	void PursuitOn(){m_iFlags |= pursuit;}
	void EvadeOn(){m_iFlags |= evade;}
	void WanderOn(){m_iFlags |= wander;}
	void ObstacleOn(){m_iFlags |= obstacle_avoidance;}
	void WallAvoidanceOn(){m_iFlags |= wall_avoidance;}
	void PowerupOff()  {if(On(powerup)) m_iFlags ^= powerup;};
	void FleeOff()     {if(On(flee))    m_iFlags ^= flee;}
	void SeekOff()     {if(On(seek))    m_iFlags ^= seek;}
	void PursuitOff()  {if(On(pursuit)) m_iFlags ^= pursuit;}
	void EvadeOff()    {if(On(evade))   m_iFlags ^= evade;}
	void WanderOff()   {if(On(wander))  m_iFlags ^= wander;}
	void ObstacleOff() {if(On(wander))  m_iFlags ^= obstacle_avoidance;}
	void WallAvoidanceOff() {if(On(wander))  m_iFlags ^= wall_avoidance;}
	bool isPowerupOn() { return On(powerup);};
	bool isFleeOn(){return On(flee);}
	bool isSeekOn(){return On(seek);}
	bool isPursuitOn(){return On(pursuit);}
	bool isEvadeOn(){return On(evade);}
	bool isWanderOn(){return On(wander);}
	bool isObstacleOn(){return On(obstacle_avoidance);}
	bool isWallAvoidanceOn(){return On(wall_avoidance);}
	bool On(behaviour_type bt){return (m_iFlags & bt) == bt;} ///Test if a specific bit of m_iFlags is set

private:
	//pointer to owner of this object
	Player* mAiPlayer;
	Vector3 mSteeringForce;
	Vector3 mTarget;
	Player* mTargetPlayer1, *mTargetPlayer2;
	Player* mFleeTarget, *mSeekTarget;
	double mWanderRadius;
	double mWanderDistance;
	double mWanderJitter;
	Vector3 mWanderTarget;
	//multipliers to modify effect of each behaviour
	double mWeightSeek;
	double mWeightFlee;
	double mWeightArrive;
	double mWeightPursuit;
	double mWeightEvade;
	double mWeightWander;
	double mWeightSteering;
	double mWeightObstacleAvoidance;
	double mWeightWallAvoidance;
	//flag used to set behaviours
	int m_iFlags;
	//caluation method with best results, but more cpu usage
	Vector3 CalculatePrioritized(void);

	bool AccumulateForce(Vector3 &RuningTot, Vector3 ForceToAdd);
    Vector3 Seek(Vector3 TargetPos);
    Vector3 Flee(Vector3 TargetPos);
    Vector3 Pursuit(Car* evader);
    Vector3 Evade(Car* pursuer);
    Vector3 Wander(void);
    //Vector3 ObstacleAvoidance(const BaseGameEntity* obstacle);
    Ogre::Vector3 WallAvoidance(const Vector3 feeler, const Vector3 wallHit, const Vector3 normal);
	Ogre::Vector3 mPowerup;


};


#endif
