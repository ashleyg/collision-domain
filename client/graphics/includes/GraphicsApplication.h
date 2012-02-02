/**
 * @file	GraphicsApplication.h
 * @brief 	Adds objects to the graphics interface and contains the framelistener
 */
#ifndef GRAPHICSAPPLICATION_H
#define GRAPHICSAPPLICATION_H

/*-------------------- INCLUDES --------------------*/
#include "stdafx.h"
#include "GameIncludes.h"


/*-------------------- CLASS DEFINITIONS --------------------*/
/**
 *  @brief  Adds objects to the graphics interface.
 *
 *          Derived from the Ogre Tutorial Framework (TutorialApplication.h).
 */
class GraphicsApplication : public GraphicsCore
{
public:
    GraphicsApplication(void);
    virtual ~GraphicsApplication(void);

    int clientID; ///< The client ID which is assigned by the server.
    bool firstFrameOccurred;
	
	void setWeatherMode (uint8_t mode);
	void setBloomMode (bool enable);

protected:
	CEGUI::OgreRenderer* mGuiRenderer;

    virtual void createScene(void);
    virtual void createFrameListener(void);
	
    // Ogre::FrameListener
    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
    virtual bool frameStarted(const Ogre::FrameEvent& evt);
    virtual bool frameEnded(const Ogre::FrameEvent& evt);
	

private:
	void setupCompositorChain (void);
	void setupShadowSystem (void);
    void setupLighting (void);
	void setupParticles (void);
    void setupArena (void);
    void setupNetworking (void);
	void createSpeedo (void);
	void updateSpeedo (float fSpeed);

	Ogre::OverlayContainer *olcSpeedo;
	Ogre::OverlayElement *oleNeedle;

	Ogre::Light* worldSun;
	Ogre::ParticleSystem* weatherSystem;
	bool weatherSystemAttached;
};

#endif // #ifndef GRAPHICSAPPLICATION_H
