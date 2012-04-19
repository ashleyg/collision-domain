/**
 * @file        AudioCore.cpp
 * @brief       An interface for firing off sounds
 */

#include "stdafx.h"
#include "SharedIncludes.h"

using namespace OgreOggSound;

#define OUTPUT_ENABLED         true

#define FILE_HORN_LOW          "car-horn-low.wav"
#define FILE_HORN_MID          "car-horn-mid.wav"
#define FILE_HORN_HIGH         "car-horn-high.wav"

#define FILE_CAR_CRASH         "car-crash-1.wav"
#define FILE_EXPLOSION         "explosion.wav"

// The same sound is ok as rpms modulate it differently per car
#define FILE_ENGINE_SMALL      "own-truck-engine-idle.ogg"
#define FILE_ENGINE_COUPE      "own-truck-engine-idle.ogg"
#define FILE_ENGINE_TRUCK      "own-truck-engine-idle.ogg"
#define FILE_GEAR_CHANGE       "own-gear-change.ogg"

#define FILE_POWERUP_HEALTH    "powerup-health.wav"
#define FILE_POWERUP_SPEED     "powerup-speed.wav"
#define FILE_POWERUP_HEAVY     "powerup-heavy.wav"
#define FILE_POWERUP_RANDOM    "powerup-random.wav"

#define FILE_BACKING_TRACK     "rockTrack.ogg"

AudioCore::AudioCore()
    : mInitOK(false)
{
    mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
    mSoundDeletesPending = new std::list<OgreOggISound*>;
    
	if(!mSoundManager)
		return;

    mInitOK = mSoundManager->init() && OUTPUT_ENABLED;

    // don't bother with some stuff unless we have openal
    if (!mInitOK) return;

    // force the soundManager to buffer immediate sounds
    std::string tempName = "hello";

    //                                                     name      file                 stream loop   preBuffer scenemgr         (immediate)
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_HORN_LOW,       false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_HORN_MID,       false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_HORN_HIGH,      false, false, true, GameCore::mSceneMgr));
    
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_CAR_CRASH,      false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_EXPLOSION,      false, false, true, GameCore::mSceneMgr));

    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_ENGINE_SMALL,   false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_ENGINE_COUPE,   false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_ENGINE_TRUCK,   false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_GEAR_CHANGE,    false, false, true, GameCore::mSceneMgr));
    
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_POWERUP_HEALTH, false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_POWERUP_SPEED,  false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_POWERUP_HEAVY,  false, false, true, GameCore::mSceneMgr));
    mSoundManager->destroySound(mSoundManager->createSound(tempName, FILE_POWERUP_RANDOM, false, false, true, GameCore::mSceneMgr));

    mBackingTrack = mSoundManager->createSound("backingtrack", FILE_BACKING_TRACK,  false, true, true, GameCore::mSceneMgr);

    mBackingTrack->setVolume(0.15f);
    //playSoundOrRestart(mBackingTrack);

    // doppler effect is good now at the default 1.0
    //mSoundManager->setSpeedOfSound();
    //mSoundManager->setDopplerFactor(6000.0);
}

/// @brief  Deconstructor.
AudioCore::~AudioCore()
{
    delete mSoundDeletesPending;
}

/// does what it says on the tin. can be given null sounds
void AudioCore::playSoundOrRestart(OgreOggISound *sound)
{
    // fix for some crash, if !mInitOK
    // (example of using optical output and some app is using it for encoded (i.e. dolby))
    if (!sound) return;

    if (sound->isPlaying()) sound->stop();
    
    sound->setPlayPosition(0);
    sound->play();
}

/// it is quite possible mInitOK is false, so this may return null.
/// the ID needs to be unique to the sound type (can be same for different sounds)
OgreOggISound* AudioCore::getSoundInstance(SoundType h, int uniqueID, Ogre::SceneNode *attachTo, bool loop)
{
    if (!mInitOK) return NULL;

    std::string file;
    switch (h)
    {
        case HORN_LOW:      file = FILE_HORN_LOW;       break;
        case HORN_MID:      file = FILE_HORN_MID;       break;
        case HORN_HIGH:     file = FILE_HORN_HIGH;      break;
        case ENGINE_SMALL:  file = FILE_ENGINE_SMALL;   break;
        case ENGINE_COUPE:  file = FILE_ENGINE_COUPE;   break;
        case ENGINE_TRUCK:  file = FILE_ENGINE_TRUCK;   break;
        case GEAR_CHANGE:   file = FILE_GEAR_CHANGE;    break;
        case CAR_CRASH:     file = FILE_CAR_CRASH;      break;
        case EXPLOSION:     file = FILE_EXPLOSION;      break;
        default: return NULL;
    }
    std::string name = file + boost::lexical_cast<std::string>(uniqueID);
    OgreOggISound *sound = getNamedSoundInstance(name, file, attachTo, loop);

    // Apply some parameters to this sound
    switch (h)
    {
        case HORN_LOW: case HORN_MID: case HORN_HIGH:
            sound->setVolume(0.55f);
            sound->setRelativeToListener(true); // always on top of the listener
            break;
        case ENGINE_SMALL: case ENGINE_COUPE: case ENGINE_TRUCK:
            sound->setVolume(0.2f);
            sound->setRolloffFactor(1.5f);
            sound->setReferenceDistance(14.f);
            break;
        case GEAR_CHANGE:
            sound->setVolume(0.7f);
            sound->setRelativeToListener(true); // Gear changes are not positional yet (but every car does have a (loud) one)
            break;
        case CAR_CRASH:     break;
        case EXPLOSION:
            sound->setVolume(0.8f);
            sound->setRolloffFactor(1.8f);
            sound->setReferenceDistance(20.f);
            break;
        default:
            break;
    }

    return sound;
}

/// it is quite possible mInitOK is false, so this may return null.
/// the ID needs to be unique to the sound type (can be same for different sounds)
OgreOggISound* AudioCore::getSoundInstance(PowerupType p, int uniqueID, Ogre::SceneNode *attachTo)
{
    if (!mInitOK) return NULL;

    std::string file;
    switch (p)
    {
        case POWERUP_HEALTH: file = FILE_POWERUP_HEALTH; break;
        case POWERUP_MASS: file = FILE_POWERUP_HEAVY; break;
        case POWERUP_RANDOM: file = FILE_POWERUP_RANDOM; break;
        case POWERUP_SPEED: file = FILE_POWERUP_SPEED; break;
        default: return NULL;
    }
    std::string name = file + boost::lexical_cast<std::string>(uniqueID);

    return getNamedSoundInstance(name, file, attachTo);
}

OgreOggISound* AudioCore::getNamedSoundInstance(std::string name, std::string file, Ogre::SceneNode *attachTo, bool loop)
{
    if (!mInitOK) return NULL;

    OgreOggISound *soundObject = mSoundManager->createSound(name, file, false, loop, true, GameCore::mSceneMgr, false);
    
    if (attachTo != NULL)
    {
        attachTo->attachObject(soundObject);
        soundObject->setRolloffFactor(2.f);
        soundObject->setReferenceDistance(10.f);
    }

    // prebuffer + immediate
    return soundObject;
}

void AudioCore::deleteSoundInstance(OgreOggISound* sound)
{
    if (!sound) return;

    mSoundDeletesPending->insert(mSoundDeletesPending->end(), sound);
}

void AudioCore::frameEvent(Ogre::Real timeSinceLastFrame)
{
    if (!mInitOK) return;

    processSoundDeletesPending();

    // In an ideal world the camera would be attached to a scene node so the listener would update
    // automatically. But that's not how we've done things, so manually set listener.
    
    // This ifdef is here to stop getLocalPlayer problems (server has no local player).
#ifdef COLLISION_DOMAIN_CLIENT
    // Attach ears to the car instead of the camera. Otherwise it just sounds weird!
    //GameCore::mGraphicsCore->mCamera->getPosition(),
    //GameCore::mGraphicsCore->mCamera->getOrientation()
    Ogre::Vector3 earsPosition = GameCore::mPlayerPool->getLocalPlayer()->getCar()->mBodyNode->getPosition();
    Ogre::Quaternion earsOrientation = GameCore::mPlayerPool->getLocalPlayer()->getCar()->mBodyNode->getOrientation();

    // if framerate is low, timeSinceLastFrame is larger.                   if framerate is high, timeSinceLastFrame is lower.
    // if framerate is low, unscaled Velocity will be higher than expected. if framerate is high, unscaled velocity will be lower than expected.

    mSoundManager->getListener()->setPosition(earsPosition);
    mSoundManager->getListener()->setOrientation(earsOrientation);

    mSoundManager->getListener()->setVelocity(GameCore::mPlayerPool->getLocalPlayer()->getCar()->getLinearVelocity());

    // fire a frameevent for each car
    int numPlayers = GameCore::mPlayerPool->getNumberOfPlayers();
    for (int i = 0; i < numPlayers; i++)
    {
        Player *player = GameCore::mPlayerPool->getPlayer(i);
        if (player == NULL) continue;
        Car *car = player->getCar();
        if (car == NULL) continue;
        car->updateAudioPitchFrameEvent();
    }
    
    // fire a frameevent for the local player
    Player *localPlayer = GameCore::mPlayerPool->getLocalPlayer();
    if (localPlayer != NULL && localPlayer->getCar() != NULL)
    {
        localPlayer->getCar()->updateAudioPitchFrameEvent();
    }
#endif
}

void AudioCore::processSoundDeletesPending()
{
    // process deletes, BUT only for sounds which have finished playing
    std::list<OgreOggISound*>::iterator i = mSoundDeletesPending->begin();

    while (i != mSoundDeletesPending->end())
    {
        OgreOggISound* sound = NULL;
        sound = *i;
        
        if (sound && !sound->isPlaying())
        {
            mSoundManager->destroySound(sound);
            mSoundDeletesPending->erase(i++); // alternatively, i = items.erase(i);
        }
        else
        {
            // leave it be for now, its still playing
            ++i;
        }
    }
}
