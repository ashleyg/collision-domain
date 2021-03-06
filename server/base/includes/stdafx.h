/**
 * @file	stdafx.h
 * @brief 	Centralizes headers and allows precompilation.
 *          See http://www.ogre3d.org/tikiwiki/Precompiled+headers for a tutorial on how to set this up.
 */

/*-------------------- INCLUDES --------------------*/

#include <string>

// Standard includes (it is unlikely these will need changing)
#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreRoot.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>

// OIS includes (the OIS libraries handle I/O)
#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

// Additional includes
#include "OgrePrerequisites.h"
#include "OgreCompositorLogic.h"
#include "OgreCompositorInstance.h"
#include <SdkTrays.h>
#include <SdkCameraMan.h>

// CEGUI includes
#include <CEGUI.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#define COLLISION_DOMAIN_SERVER

// Windows specific include
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include "BulletCollision\NarrowPhaseCollision\btGjkConvexCast.h"
    #define WIN32_LEAN_AND_MEAN
    #include "windows.h"
#else
	#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

    // This would save debugging time ;)
    #define OutputDebugString printf
#endif

