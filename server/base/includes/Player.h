/**
 * @file	Player.h
 * @brief 	Contains the player car and the related data specific to each player.
 */
#ifndef PLAYER_H
#define PLAYER_H

/*-------------------- INCLUDES --------------------*/
#include "stdafx.h"
#include "GameIncludes.h"
#include "CarCam.h"


/*-------------------- CLASS DEFINITIONS --------------------*/
/**
 *  @brief 	Contains the Player nodes and the related data.
 */
class Player
{
 
public:
    Player (void);
    ~Player (void);
    void createPlayer (CarType carType, CarSkin skin);
    void attachCamera (Ogre::Camera* cam);
    void processControlsFrameEvent (InputState *userInput, Ogre::Real secondsSinceLastFrame, float targetPhysicsFrameRate);
    void updateCameraFrameEvent (int XRotation, int YRotation, int ZDepth);
	float getCameraYaw (void);
    Car* getCar (void);
    //void collisionTickCallback (int damage, Player *causedByPlayer);
	void collisionTickCallback(btVector3 &hitPoint, float damage, Player *causedByPlayer);
    void applyHealthBonus (void);
        
    const char *getNickname (void) { return mNickname; }
    int getCarType (void) { return mCarType; }

    // Probably a better alternative to strdup (could use std::string but I've never been a fan, I like C strings :D )
    void setNickname (char *szNick) { mNickname = strdup( szNick ); }
	int	 getHP (void);

	void setGUID(RakNet::RakNetGUID playerGUID);
	std::string getGUID(void);
	
	void setOverlayElement (Ogre::OverlayElement* ole);
    void setSpawned (void); //Marks the car as spawned
    Ogre::OverlayElement* getOverlayElement (void);

	InputState* newInput;

	RakNet::RakNetGUID getPlayerGUID();
	void setPlayerGUID(RakNet::RakNetGUID playerGUID);

	//Just some getters and setters
	void setAlive(bool pAlive);
	bool getAlive();
    void setVIP(bool newState) { mIsVIP = newState; };
    bool getVIP(void) { return mIsVIP; };
    void setTeam(int newTeam) { mTeam = newTeam; };
    int  getTeam(void) { return mTeam; };

	//Now have a "Kill" method that will also set the call backs
	void killPlayer();
	RakNet::RakNetGUID playerGUID;
	void killPlayer(Player* causedBy);
	void resetHP();

	//Increase
	void addToScore(int amount);
	int getRoundScore();	
	void addToGameScore(int amount);
private:
    const float      cameraRotationConstant;
	int		         hp;
    int              mTeam;
	bool		     mAlive;
    bool             mIsVIP;
	bool             mSpawned;
    char*            mNickname;
    Car*             mCar;
    CarSnapshot*     mCarSnapshot;
	CarCam*	         mCarCam;
	Ogre::SceneNode* camNode;
	Ogre::SceneNode* camArmNode;
    CarType          mCarType;

	Ogre::OverlayElement* mOLE;
	RakNet::RakNetGUID mPlayerGUID;
	bool							  processingCollision;
	std::map<std::string, float>	  collisionDamages;
	std::map<std::string, btVector3>  collisionPositions;
	int							      numCollisionDataPoints;
	btVector3					      averageCollisionPoint;
								      
	std::string					      stringGUID;

	int roundScore;
	int gameScore;
};

#endif // #ifndef PLAYER_H
