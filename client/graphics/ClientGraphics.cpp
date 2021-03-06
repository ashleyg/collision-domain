/**
 * @file    GraphicsCore.h
 * @brief     Configures the graphical settings and provides the common graphical functionality.
 */

/*-------------------- INCLUDES --------------------*/
#include "stdafx.h"
#include "ClientGraphics.h"
#include "GameCore.h"

#if linux
#include <sys/resource.h>
#endif

#define PHYSICS_FPS 60
//char g_GraphicalLevel = 0; // 0 = low, 1 = medium, 2 = high

/*-------------------- METHOD DEFINITIONS --------------------*/

/// @brief  Constructor, initialising all resources.
ClientGraphics::ClientGraphics (void) : SceneSetup(),
                                        mRoot(0),
                                        mCamera(0),
                                        mResourcesCfg(Ogre::StringUtil::BLANK),
                                        mPluginsCfg(Ogre::StringUtil::BLANK),
                                        mCameraMan(0),
                                        mCursorWasVisible(false),
                                        mDebrisVisible(false),
                                        mShutDown(false),
                                        mBigScreen(0),
                                        mGraphicsState(UNDEFINED)
{
}


/// @brief  Deconstructor.
ClientGraphics::~ClientGraphics (void)
{
    // Destroy camera manager.
    if (mCameraMan)
        delete mCameraMan;

    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);
    delete mRoot;
}


/// @brief  Starts the graphics.
void ClientGraphics::go (void)
{
    srand(time(NULL));

    if (!initApplication())
        return;

    mRoot->startRendering();

    // clean up
    destroyScene();
    GameCore::destroy();
}


/// @brief  Attempts to load the resources and add them to the scene.
/// @return Whether or not the setup was successful (if a configuration was provided).
bool ClientGraphics::initApplication (void)
{
    // Select and load the relevant resources
    mResourcesCfg = "../../media/resources.cfg";
#ifdef _DEBUG
    mPluginsCfg = "plugins_d.cfg";
#else
    mPluginsCfg = "plugins.cfg";
#endif
    mRoot = new Ogre::Root(mPluginsCfg, "ogre.cfg", "");
    setupResources();
    
    // Configure the renderer and exit if no configuration was provided.
    if (!configureRenderer())
        return false;

    // Create the window and scenemanager to control it.
    mWindow = mRoot->initialise(true, "Collision Domain");
    GameCore::mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
    
    // Create the camera and viewport for viewing the scene
    createCamera();
    createViewports();
    
    // Initialise the application critical elements.
    GameCore::initialise(this);

    // Load the lobby
#ifdef MENU_LOBBY
    loadLobby();

    setupUserInput();

    createFrameListener();
#else
    // First load CEGUI and it's resources by running the generic setup.
    //SceneSetup::setupGUI();

    setupUserInput();

    createFrameListener();

    //GameCore::mNetworkCore->AutoConnect( SERVER_PORT );

    // wait for connection to server.
    while (GameCore::mNetworkCore->m_szHost == NULL)
    {
        // Check for exit conditions
        mUserInput.capture();
        if (mUserInput.mKeyboard->isKeyDown(OIS::KC_ESCAPE))
            return false;

        // Handle networking
        GameCore::mNetworkCore->AutoConnect( SERVER_PORT );
        GameCore::mNetworkCore->frameEvent(NULL);

        // Have a lil nap.
        boost::this_thread::sleep( boost::posix_time::milliseconds( 10 ) );
    }

    loadGame();
#endif

    return true;
}


/// @brief  Locates the resources using an external file. No resources are loaded at this stage.
void ClientGraphics::setupResources (void)
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(mResourcesCfg);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }
}


/// @brief  Shows the configuration dialog and waits for the user to select their preferences
///         to initialise the system with.
///         You can skip this and use root.restoreConfig() to load configuration settings if 
///         you were sure there are valid ones saved in ogre.cfg
/// @return Whether or not the configuration was a success.
bool ClientGraphics::configureRenderer (void)
{
#if (defined(LINUX_GFX_OVERRIDE)) && (OGRE_PLATFORM == OGRE_PLATFORM_LINUX)
    // Manually initialise
    OutputDebugString("Linux system detected, using auto-settings
    RenderSystem* rs = NULL;
    RenderSystemList systems = Root::getSingleton().getAvailableRenderers();

    // Check if any render systems exist
    if (systems.empty())
        return false;
    // Check if OpenGL is one of those rendering systems (should be)
    for (RenderSystemList::iterator itr = systems.begin(); itr != systems.end(); itr++)
        if (!strcmp((*itr)->getName().c_str(), "OpenGL Rendering Subsystem"))
            rs = *itr;
    // If it wasn't, default to the first renderer
    if (rs == NULL)
    {
        rs = *systems.begin();
        OutputDebugString("OpenGL not found, defaulting to %s.\n", rs->getName().c_str());
    }

    Root::getSingleton().setRenderSystem(rs);
    rs->setConfigOption("Display Frequency", "60 MHz");
    rs->setConfigOption("FSAA", "0");
    rs->setConfigOption("Full Screen", "Yes");
    rs->setConfigOption("Video Mode", "800 x 600");

    return true;
#else
    // Show the configuration dialog and returns true if the user clicks OK.
    if (mRoot->showConfigDialog())
        return true;
    return false;
#endif
}


/// @brief  Creates and positions the camera.
void ClientGraphics::createCamera (void)
{
    // Create the camera
    mCamera = GameCore::mSceneMgr->createCamera("PlayerCam");

    // Position it looking back along -Z
    mCamera->setPosition(Ogre::Vector3(0, 3, 60));
    mCamera->lookAt(Ogre::Vector3(0, 0, -300));
    mCamera->setNearClipDistance(0.5);
    mCamera->setFarClipDistance(2500);

    // Create game camera
    mGameCam = new GameCamera( mCamera );
    // Set it to chase mode
    mGameCam->setCamType( CAM_CHASE );
    mGameCam->setCollidable( true );
    // Set how much the camera 'snaps' to locations
    // This gets multiplied by time since last frame
    // For cinematic style camera 0.2 works quite well
    mGameCam->setTension( 2.8f );
    // Positional offset - behind and above the vehicle
    mGameCam->setOffset( btVector3( 0.f, 5.f, -10.f ) );
    // Focus offset - slightly in front of car's local origin
    mGameCam->setLookOffset( btVector3( 0, 0, 3.0f ) );
    // Put the camera up in the air
    //mGameCam->setTransform( btVector3( 0, 500, 0 ) );
    //mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // create a default camera controller
}


/// @brief  Adds a single viewport that spans the entire window.
void ClientGraphics::createViewports (void)
{
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    this->screenWidth   = vp->getActualWidth();
    this->screenHeight  = vp->getActualHeight();
    // Set the background colour and match the aspect ratio to the window's.
    vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0));
    mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
}


#ifdef MENU_LOBBY
void ClientGraphics::loadLobby (void)
{
    // Set the graphics state
    mGraphicsState = IN_LOBBY;

    // First load CEGUI and it's resources by running the generic setup.
    SceneSetup::setupGUI();

    // Create the lobby and set it right up.
    mLobby = new Lobby(mGUIWindow);
    mLobby->setup(mWindow->getWidth(), mWindow->getHeight());
    mLobby->addServer("Default Server", "4", "Colosseum", true);
    //mLobby->addServer("Boobs Server", "15/32", "GAY MAP", false);
    //mLobby->addServer("Another Server", "0/32", "NEW MAP", false);
}
#endif


#ifdef MENU_LOBBY
void ClientGraphics::unloadLobby (void)
{
    // Set the graphics state
    mGraphicsState = UNDEFINED;
    mLobby->close();
    delete mLobby;
}
#endif


void ClientGraphics::loadGame (void)
{
    if (GameCore::mNetworkCore->m_szHost == NULL)
        return;

    // Set the graphics state (unloading the lobby set it as undefined).
#ifdef MENU_LOBBY
    if (mGraphicsState == IN_LOBBY)
        unloadLobby();
#endif

    // Create the splash screen (preloading its required resources in the process)
    SplashScreen splashScreen(mRoot);
    splashScreen.draw(mWindow->getWidth(), mWindow->getHeight());
    splashScreen.updateProgressBar(0, "Loading Resources...");
    Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(&splashScreen);

    // Load the remaining resources.
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
    loadResources();

    // Load the game elements
    GameCore::load(&splashScreen, 50);

    // Build the scene.
    createScene();
    //mSpawnScreen = NULL;

    // Clear the splash screen.
    splashScreen.clear();

    // Connect to the received broadcast address
    GameCore::mNetworkCore->Connect();

    mGraphicsState = IN_GAME;
}


void ClientGraphics::unloadGame (void)
{
    // Set the graphics state
    mGraphicsState = UNDEFINED;
}


/// @brief  Loads resources from the resources.cfg file into the ResourceGroup.
void ClientGraphics::loadResources (void)
{
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}


/// @brief  Creates the initial scene prior to the first render pass, adding objects etc.
void ClientGraphics::createScene (void)
{
    // Setup the scene environment.
    setupCompositorChain(mCamera->getViewport());
    setupShadowSystem();
    setupLightSystem();
    setupParticleSystem();
    setupArenaNodes();

    // Setup the GUI.
    setupGUI();

    // Setup the scene resources.
    setupMeshDeformer();
}


void ClientGraphics::setupGUI (void)
{
    // Initialise the GUI renderer if it hasn't already been.
    SceneSetup::setupGUI();

    // Attach the GUI components
    GameCore::mGui->setupSpawnScreen(mGUIWindow);

    GameCore::mGui->setupFPSCounter(mGUIWindow);
    GameCore::mGui->setupConsole(mGUIWindow);
    GameCore::mGui->setupChatbox(mGUIWindow);
    GameCore::mGui->setupOverlays(mGUIWindow);

    GameCore::mGameplay->setupOverlay();
}


void ClientGraphics::setupUserInput (void)
{
    OIS::ParamList     pl;
    size_t             windowHnd = 0;
    std::ostringstream windowHndStr;

    // Setup User Input, setting initial mouse clipping area.
    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(make_pair("WINDOW", windowHndStr.str()));
    mUserInput.createInputSystem(pl);

    // Force the mouse clipping to be recalculated.
    windowResized(mWindow);
}


/// @brief  Creates a frame listener for the main window.
void ClientGraphics::createFrameListener (void)
{
    // Listener registration
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);
    mRoot->addFrameListener(this);
}


/// @brief  Called once a frame as the CPU has finished its calculations and the GPU is about to start rendering.
/// @param  evt  The FrameEvent associated with this frame's rendering.
/// @return Whether the application should continue (i.e.\ false will force a shut down).
bool ClientGraphics::frameRenderingQueued (const Ogre::FrameEvent& evt)
{
    static const float physicsTimeStep = 1.0f / (float) PHYSICS_FPS;

    // Check for exit conditions.
    if (mWindow->isClosed())
        return false;
    if (mShutDown)
        return false;
    
#ifdef MENU_LOBBY
    if (mGraphicsState == IN_LOBBY)
    {
        // Capture input
        mUserInput.capture();

        // Update GUI
        CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);

        GameCore::mNetworkCore->frameEvent(NULL);
    }
    else
#endif
    if (mGraphicsState == IN_GAME || mGraphicsState == PROJECTOR)
    {
        // Capture input
        mUserInput.capture();

        if (mUserInput.mKeyboard->isKeyDown(OIS::KC_ESCAPE) && mUserInput.mKeyboard->isKeyDown(OIS::KC_F11))
            return false;

        if ((!mUserInput.mKeyboard->isKeyDown(OIS::KC_LMENU)) && mUserInput.mKeyboard->isKeyDown(OIS::KC_TAB))		
            GameCore::mGameplay->mSB->show();		
        else		
            GameCore::mGameplay->mSB->hide();
    
        // Update GUI
        CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);

        // Update the particle systems
        updateParticleSystems();

        // Rotate the VIP crowns (this should really be done somewhere else, but again waiting for a good place)
        mVIPIcon[0]->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(90 * evt.timeSinceLastFrame));
        mVIPIcon[1]->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(90 * evt.timeSinceLastFrame));
        

        // Collect input
            InputState *inputSnapshot = mUserInput.getInputState();
        mUserInput.processInterfaceControls();
    
        // Process the networking. Sends client's input and receives data
        GameCore::mNetworkCore->frameEvent(inputSnapshot);
        
        // Process the player pool. Perform updates on other players
        if (NetworkCore::bConnected)
        {

            GameCore::mPlayerPool->frameEvent(evt.timeSinceLastFrame);
            if (GameCore::mPlayerPool->getLocalPlayer()->getCar() != NULL)
            {
                    
                GameCore::mPlayerPool->getLocalPlayer()->processControlsFrameEvent(inputSnapshot, evt.timeSinceLastFrame);
            }
        }

        GameCore::mPowerupPool->frameEvent(evt.timeSinceLastFrame);

        //-PHYSICS-STEP--------------------------------------------------------------------
        // Minimum of 30 FPS (maxSubsteps=2) before physics becomes wrong
        GameCore::mPhysicsCore->stepSimulation(evt.timeSinceLastFrame, 3, physicsTimeStep);
        //-PHYSICS-STEP--------------------------------------------------------------------

        //Draw info items
        //GameCore::mGameplay->drawInfo();
    
        // Apply controls the player (who will be moved on frameEnd and frameStart).
        if (NetworkCore::bConnected)
        {
            if (GameCore::mPlayerPool->getLocalPlayer()->getCar() != NULL)
            {
                // Moved here as audio event needs freshest car position
                GameCore::mPlayerPool->getLocalPlayer()->updateLocalGraphics();
                GameCore::mGui->updateCounters();
                GameCore::mGui->updateSpeedo();
            }

            // AudioCore needs to be fed localPlayer even if NULL (to detect spawn screen)
            GameCore::mAudioCore->frameEvent(evt.timeSinceLastFrame);
            mGameCam->update( evt.timeSinceLastFrame );
            if( GameCore::mPlayerPool->getLocalPlayer() && GameCore::mPlayerPool->getLocalPlayer()->getCar() )
                GameCore::mPlayerPool->getLocalPlayer()->updateCameraFrameEvent(mUserInput.getMouseXRel(), mUserInput.getMouseYRel(), mUserInput.getMouseZRel(), evt.timeSinceLastFrame, mGameCam);
        }

		if (mGraphicsState == PROJECTOR)		
		{	
            // sure, this gets called every frame but its still going to be super fast
            GameCore::mGui->hideOverlaysForBigScreen();

			this->mBigScreen->updateMapView();
		}
        
        // Cleanup frame specific objects.
        delete inputSnapshot;
    }
    // Check for benchmarking
    else if (mGraphicsState == BENCHMARKING)
    {
        // Update the GUI and benchmark
        CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);
        updateBenchmark(evt.timeSinceLastFrame);
    }

    return true;
}


/// @brief  Called once a frame every time processing for a frame has begun.
/// @param  evt  The FrameEvent associated with this frame's rendering.
/// @return Whether the application should continue (i.e.\ false will force a shut down).
bool ClientGraphics::frameStarted (const Ogre::FrameEvent& evt)
{
    return true;
}


/// @brief  Called once a frame every time processing for a frame has ended.
/// @param  evt  The FrameEvent associated with this frame's rendering.
/// @return Whether the application should continue (i.e.\ false will force a shut down).
bool ClientGraphics::frameEnded (const Ogre::FrameEvent& evt)
{
    return true;
}


void ClientGraphics::updateBenchmark (const float timeSinceLastFrame)
{// Check for benchmarking
        static float    benchmarkProgress = 0;
        static float    CATriangles       = 0;
        static uint16_t CAi               = 0;
    benchmarkProgress += timeSinceLastFrame;
        CATriangles += ((float) (mWindow->getTriangleCount() - CATriangles)) / ((float) (++CAi));
        // stop the benchmark after 8 seconds
        if (benchmarkProgress > 8)
        {
                CAi               = 0;
                benchmarkProgress = 0;
                finishBenchmark(mBenchmarkStage, CATriangles);
        }

        // rotate the camera
    GameCore::mPlayerPool->getLocalPlayer()->updateCameraFrameEvent(500 * timeSinceLastFrame, 0.0f, 0.0f, timeSinceLastFrame, mGameCam);

        // update fps counter
        float avgfps = mWindow->getAverageFPS(); // update fps
        CEGUI::Window *fps = CEGUI::WindowManager::getSingleton().getWindow( "root_wnd/fps" );
        char szFPS[16];
        sprintf(szFPS, "FPS: %.2f", avgfps);
        fps->setText(szFPS);
}


void ClientGraphics::updateParticleSystems (void)
{
    unsigned short i;
    // Check for stale emitters in the systems and cleanup
#ifdef PARTICLE_EFFECT_SPARKS
    for (i = 0; i < mSparkSystem->getNumEmitters(); i++)
        if (!mSparkSystem->getEmitter(i)->getEnabled())
            mSparkSystem->removeEmitter(i--);
#endif
#ifdef PARTICLE_EFFECT_SPLINTERS
    for (i = 0; i < mSplinterSystem->getNumEmitters(); i++)
        if (!mSplinterSystem->getEmitter(i)->getEnabled())
            mSplinterSystem->removeEmitter(i--);
#endif
#ifdef PARTICLE_EFFECT_EXPLOSION
    for (i = 0; i < mExplosionNucleusSystem->getNumEmitters(); i++)
        if (!mExplosionNucleusSystem->getEmitter(i)->getEnabled())
            mExplosionNucleusSystem->removeEmitter(i--);
    for (i = 0; i < mExplosionSmokeSystem->getNumEmitters(); i++)
        if (!mExplosionSmokeSystem->getEmitter(i)->getEnabled())
            mExplosionSmokeSystem->removeEmitter(i--);
    for (i = 0; i < mExplosionDebrisSystem->getNumEmitters(); i++)
        if (!mExplosionDebrisSystem->getEmitter(i)->getEnabled())
            mExplosionDebrisSystem->removeEmitter(i--);
#endif

#ifdef PARTICLE_EFFECT_SHRAPNEL
    // Check for stale shrapnel systems
    #ifdef PARTICLE_EFFECT_SHRAPNEL_HIQUAL
        Ogre::ParticleSystem* currentSystem;

        char dstr[256];
        sprintf(dstr, "ShrapnelSystem queue size = %d\n", mShrapnelSystems.size());
        //OutputDebugString(dstr);

        while (!mShrapnelSystems.empty())
        {
            currentSystem = mShrapnelSystems.front();
            if (currentSystem->getNumParticles() == 0)
            {
                mShrapnelSystems.pop();
                GameCore::mSceneMgr->destroyParticleSystem(currentSystem);
            }
            else
                break;
        }
    #else
        for (i = 0; i < mShrapnelSystem->getNumEmitters(); i++)
            if (!mShrapnelSystem->getEmitter(i)->getEnabled())
                mShrapnelSystem->removeEmitter(i--);
    #endif
    
    #ifdef PARTICLE_EFFECT_SHRAPNEL_SPARKS
        for (i = 0; i < mShrapnelSparkSystem->getNumEmitters(); i++)
            if (!mShrapnelSparkSystem->getEmitter(i)->getEnabled())
                mShrapnelSparkSystem->removeEmitter(i--);
    #endif
#endif
}


#ifdef PARTICLE_EFFECT_SPLINTERS
void ClientGraphics::generateSplinters (Ogre::Vector3 location)
{
    unsigned short splinterIndex = mSplinterSystem->getNumEmitters();

    mSplinterSystem->addEmitter("Point");
    mSplinterSystem->getEmitter(splinterIndex)->setParameterList(mSplinterParams);
    mSplinterSystem->getEmitter(splinterIndex)->setPosition(location);
}
#endif


/// @brief  Generates shrapnel particles which are automatically cleared up after they are depleted.
/// @param  location    The location, in world space, for the shrapnel's origin to be placed.
/// @param  shrapnelTeam    The team of the car that generated the shrapnel. Alternatively NO_TEAM leaves the shrapnel the default colour.
/// @param  meanShrapnelQuantity    The average amount of shrapnel to create in one emission. The exact amount generated is randomised.
/// @param  planeOffset The offset in the y axis of the plane for the shrapnel to collide with from the shrapnel's origin provided by 
///                     the location parameter. This value should be negative.
/// @param  planeNormal The normal of the plane with which the shrapnel is to collide.
#ifdef PARTICLE_EFFECT_SHRAPNEL
void ClientGraphics::generateShrapnel (Ogre::Vector3 location, TeamID shrapnelTeam, float meanShrapnelQuantity, float maxShrapnelVelocity, float planeOffset, Ogre::Vector3 planeNormal)
{
    // Add shrapnel sparks
#ifdef PARTICLE_EFFECT_SHRAPNEL_SPARKS
    // Create the emitter.
    unsigned short shrapnelSparkIndex = mShrapnelSparkSystem->getNumEmitters();
    mShrapnelSparkSystem->addEmitter("Point");

    // Configure the emitter.
    mShrapnelSparkSystem->getEmitter(shrapnelSparkIndex)->setParameterList(mShrapnelSparkParams);
    mShrapnelSparkSystem->getEmitter(shrapnelSparkIndex)->setPosition(location);
    mShrapnelSparkSystem->getEmitter(shrapnelSparkIndex)->setEmissionRate(meanShrapnelQuantity * 2);
    mShrapnelSparkSystem->getEmitter(shrapnelSparkIndex)->setMaxParticleVelocity(maxShrapnelVelocity);
#endif



    // Get a reference to the final emitter.
#ifdef PARTICLE_EFFECT_SHRAPNEL_HIQUAL
    // First create the particle system, if we are running the high quality, plane colliding version, then get the emitter.
    int uid = GameCore::mPhysicsCore->getUniqueEntityID();
    Ogre::ParticleSystem* shrapnelSystem = GameCore::mSceneMgr->createParticleSystem("ShrapnelSystem" + boost::lexical_cast<std::string>(uid), "CollisionDomain/Shrapnel");
    Ogre::ParticleEmitter* shrapnelEmitter = shrapnelSystem->getEmitter(0);
#else
    // Create the emitter.
    unsigned short shrapnelIndex = mShrapnelSystem->getNumEmitters();
    mShrapnelSystem->addEmitter("Point");
    Ogre::ParticleEmitter* shrapnelEmitter = mShrapnelSystem->getEmitter(shrapnelIndex);
    shrapnelEmitter->setParameterList(mShrapnelParams);
#endif



    // Configure the emitter.
    shrapnelEmitter->setPosition(location);
    if (shrapnelTeam == BLUE_TEAM)
        shrapnelEmitter->setColour(Ogre::ColourValue(0.125f, 0.235f, 0.380f));
    else if (shrapnelTeam == RED_TEAM)
        shrapnelEmitter->setColour(Ogre::ColourValue(0.376f, 0.125f, 0.125f));
    shrapnelEmitter->setEmissionRate(meanShrapnelQuantity * 10);
    shrapnelEmitter->setMaxParticleVelocity(maxShrapnelVelocity);

    // Add the deflector plane to the high quality shrapnel.
#ifdef PARTICLE_EFFECT_SHRAPNEL_HIQUAL
    // Add the shrapnel plane with which it collides.
    // Calculate the plane's properties and convert them to string representations.
    Ogre::Vector3 planeLocation = location;
    planeLocation.y += planeOffset;
    char plane_point[32];
    char plane_normal[32];
    sprintf(plane_point,  "%.2f %.2f %.2f", planeLocation.x, planeLocation.y, planeLocation.z);
    sprintf(plane_normal, "%.2f %.2f %.2f", planeNormal.x,   planeNormal.y,   planeNormal.z);
    // Create and setup the plane affector
    /*Ogre::ParticleAffector* planeAffector = shrapnelSystem->addAffector("DeflectorPlane");
    planeAffector->setParameter("plane_point",  plane_point);
    planeAffector->setParameter("plane_normal", plane_normal);
    planeAffector->setParameter("bounce", "0.15");*/
#endif
    


#ifdef PARTICLE_EFFECT_SHRAPNEL_HIQUAL
    // Add the systems to the queue (so that they can be removed later), as well as the scene.
    mShrapnelSystems.push(shrapnelSystem);
    GameCore::mSceneMgr->getRootSceneNode()->attachObject(shrapnelSystem);
#endif
}
#endif


void ClientGraphics::updateVIPLocation (TeamID teamID, Ogre::Vector3 location)
{
    location.y += 4;
    if (teamID == BLUE_TEAM)
        mVIPIcon[0]->setPosition(location);
    else if (teamID == RED_TEAM)
        mVIPIcon[1]->setPosition(location);
    else
    {
        mVIPIcon[0]->setPosition(location);
        mVIPIcon[1]->setPosition(location);
    }
}


#ifdef PARTICLE_EFFECT_EXPLOSION
/// @brief  Generates an explosion.
void ClientGraphics::generateExplosion (Ogre::Vector3 location)
{
    unsigned short nucleusIndex = mExplosionNucleusSystem->getNumEmitters();
    unsigned short smokeIndex   = mExplosionSmokeSystem->getNumEmitters();
    unsigned short debrisIndex  = mExplosionDebrisSystem->getNumEmitters();

    mExplosionNucleusSystem->addEmitter("Point");
    mExplosionNucleusSystem->getEmitter(nucleusIndex)->setParameterList(mExplosionNucleusParams);
    mExplosionNucleusSystem->getEmitter(nucleusIndex)->setPosition(location);

    mExplosionSmokeSystem->addEmitter("Point");
    mExplosionSmokeSystem->getEmitter(smokeIndex)->setParameterList(mExplosionSmokeParams);
    mExplosionSmokeSystem->getEmitter(smokeIndex)->setPosition(location);

    mExplosionDebrisSystem->addEmitter("Point");
    mExplosionDebrisSystem->getEmitter(debrisIndex)->setParameterList(mExplosionDebrisParams);
    mExplosionDebrisSystem->getEmitter(debrisIndex)->setPosition(location);

    // Generate a sound (this isn't a particularly good way of doing it but it will work until a better method is available).
    static OgreOggSound::OgreOggISound* explosionSound = GameCore::mAudioCore->getSoundInstance(EXPLOSION, 0, NULL);
    explosionSound->setPosition(location);
    GameCore::mAudioCore->playSoundOrRestart(explosionSound);
}
#endif


#ifdef PARTICLE_EFFECT_SPARKS
void ClientGraphics::generateSparks (Ogre::Vector3 location, Ogre::Vector3 direction)
{
    unsigned short sparkIndex = mSparkSystem->getNumEmitters();

    mSparkSystem->addEmitter("Point");
    mSparkSystem->getEmitter(sparkIndex)->setParameterList(mSparkParams);
    mSparkSystem->getEmitter(sparkIndex)->setPosition(location);
    mSparkSystem->getEmitter(sparkIndex)->setDirection(direction);
}
#endif


/// @brief  Adds the finishing podium to the game world. Not exactly the best way to do this but nevermind.
/// @param  position    The position, in world co-ordinates, to add the podium at.
void ClientGraphics::addPodium (Ogre::Vector3 position)
{
    Ogre::SceneNode* podiumNode = GameCore::mSceneMgr->getRootSceneNode()->createChildSceneNode("PodiumNode");
    podiumNode->setPosition(position);
    GameCore::mPhysicsCore->auto_scale_scenenode(podiumNode);
    Ogre::Entity*  podiumEntity = GameCore::mSceneMgr->createEntity("PodiumEntity", "podium.mesh");
}


/// @brief  Adds a player to the podium.
/// @param  pPlayer A pointer to the player to add to the podium.
/// @param  scoreRank   The position of the player to be added to the podium. scoreRank=1 will put them on the first place, 
///                     2 on the second place, 3 on third and all others will be ignored.
void ClientGraphics::addToPodium (Player* pPlayer, unsigned int scoreRank)
{
    // Get references to this shizz.
    Ogre::SceneNode* podiumNode;
    try
    {
        podiumNode = GameCore::mSceneMgr->getSceneNode("PodiumNode");
    }
    catch (Exception e)
    {
        OutputDebugString("Exception caught:\n");
        OutputDebugString(e.getFullDescription().c_str());
        OutputDebugString("addToPodium likely called when a podium did not exist. Someone's gonna get a slap. Initiating crash\n");
        throw Ogre::Exception::ERR_DUPLICATE_ITEM;
    }

    // Offset this junk with the shizz.
    Ogre::Vector3 localPodiumPosition;
    if      (scoreRank == 1)
        localPodiumPosition = Ogre::Vector3(0, 0, 0);
    else if (scoreRank == 2)
        localPodiumPosition = Ogre::Vector3(-5.325f, -1.32f, 0);
    else if (scoreRank == 3)
        localPodiumPosition = Ogre::Vector3( 5.325f, -2.64f, 0);
    else
        return;
    Ogre::Vector3 worldPodiumPosition = podiumNode->convertLocalToWorldPosition(localPodiumPosition);

    // Convert the shizz to bullet jazz.
    btVector3    bulletPos(worldPodiumPosition.x, worldPodiumPosition.y, worldPodiumPosition.z);
    //Ogre::Quaternion podiumRot = podiumNode->getOrientation();
    //btQuaternion bulletRot(podiumRot.x, podiumRot.y, podiumRot.z, podiumRot.w);

    // Apply some jazz.
    pPlayer->getCar()->moveTo(bulletPos);
}


/// @param  Removes and destroys the finishing podium from the game world.
void ClientGraphics::removePodium (void)
{
    // Destroy shit
    GameCore::mSceneMgr->destroyEntity("PodiumNode");
    GameCore::mSceneMgr->destroySceneNode("PodiumEntity");
}


/// @brief  Removes everything from the scene.
void ClientGraphics::destroyScene (void)
{
    // or not, lol.
}


/// @brief  Adjust mouse clipping area when the window is resized.
/// @param  rw  The window that has been resized.
void ClientGraphics::windowResized (Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);

    const OIS::MouseState &ms = mUserInput.mMouse->getMouseState();
    ms.width = width;
    ms.height = height;
}


/// @brief  Closes a window, unattaching OIS before window shutdown (very important under Linux).
/// @param  rw  The window to close.
void ClientGraphics::windowClosed (Ogre::RenderWindow* rw)
{
    // Only close for window that created OIS
    if (rw == mWindow)
        mUserInput.destroyInputSystem();
}


/* ---------- Benchmarking ---------- */


void ClientGraphics::startBenchmark (uint8_t stage)
{
#if defined(COMPOSITOR_HDR) && defined(COMPOSITOR_BLOOM) && defined(COMPOSITOR_RADIAL_BLUR) && defined(COMPOSITOR_MOTION_BLUR)
        Ogre::CompositorManager& cm = Ogre::CompositorManager::getSingleton();
        Ogre::Viewport* vp = mCamera->getViewport();
        switch (stage)
        {
                case 0: // all off
                        //OutputDebugString("Starting benchmark...\n");
                        cm.removeCompositor(vp, "HDR");
                        cm.removeCompositor(vp, "Bloom");
                        cm.removeCompositor(vp, "MotionBlur");
                        cm.removeCompositor(vp, "RadialBlur");
                        break;
                case 1: // just hdr on
                        cm.addCompositor(vp, "HDR");
                        loadHDR(vp, 0);
                        break;
                case 2: // just bloom on
                        cm.removeCompositor(vp, "HDR");
                        cm.addCompositor(vp, "Bloom");
                        loadBloom(vp, 0, 0.15f, 1.0f);
                        break;
                case 3: // just MotionBlur on
                        cm.removeCompositor(vp, "Bloom");
                        cm.addCompositor(vp, "MotionBlur");
                        loadMotionBlur(vp, 0, 0.1f);
                        break;
                case 4: // just RadialBlur on
                        cm.removeCompositor(vp, "MotionBlur");
                        cm.addCompositor(vp, "RadialBlur");
                        cm.setCompositorEnabled(vp, "RadialBlur", true);
                        break;
                case 5: // all on
                        cm.addCompositor(vp, "HDR");
                        loadHDR(vp, 0);
                        cm.addCompositor(vp, "Bloom");
                        loadBloom(vp, 0, 0.15f, 1.0f);
                        cm.addCompositor(vp, "MotionBlur");
                        loadMotionBlur(vp, 0, 0.1f);
                        break;
        }
        
        mWindow->resetStatistics();
        mBenchmarkStage = stage;
        mGraphicsState = BENCHMARKING;
#else
    OutputDebugString("WARNING! You may not call benchmark without all compositors loaded.\n");
#endif
}


void ClientGraphics::setupProjector()		
{		
	mGraphicsState = PROJECTOR;		
	this->mBigScreen = new BigScreen();		
	this->mBigScreen->setupMapView();

    // players and powerups spawned before the bigscreen will have a NULL overlayElement. Fix this.
    std::vector<Player*> players = GameCore::mPlayerPool->getPlayers();
    for (unsigned int i = 0; i < players.size(); i++)
    {
        Car *car;
        if (car = players[i]->getCar())
        {
            car->reinitBigScreenOverlayElementIfNull();
        }
    }

    std::vector<Powerup *> powerups = GameCore::mPowerupPool->getPowerups();
    for (unsigned int i = 0; i < powerups.size(); i++)
    {
        powerups[i]->reinitBigScreenOverlayElementIfNull();
    }
}		

			
GraphicsState ClientGraphics::getGraphicsState()		
{		
	return this->mGraphicsState;		
}


void ClientGraphics::finishBenchmark (uint8_t stage, float averageTriangles)
{
        static float r[6];
        static float triangles;
        r[stage] = mWindow->getAverageFPS();
        if (stage == 0)
                triangles = averageTriangles;

        if (stage == 5)
        {
                std::ofstream rFile;
                rFile.open("BenchmarkResults.txt", std::ios::out | std::ios::trunc);
                rFile << std::fixed;
                rFile << "              BENCHMARKING RESULTS\n";
                rFile << " Average triangles per frame = " << std::setprecision(0) << triangles << "\n\n";
                rFile << "+-----+-------+-----+-----+-------+-------+\n";
                rFile << "| HDR | Bloom | MoB | RaB |  FPS  |  DIF  |\n";
                rFile << "+-----+-------+-----+-----+-------+-------+\n";
                rFile.precision(2);
                rFile << "|  0  |   0   |  0  |  0  | " << std::setw(5) << std::setfill(' ') << r[0] << " | 00.00 |\n";
                rFile << "|  1  |   0   |  0  |  0  | " << std::setw(5) << std::setfill(' ') << r[1] << " | " << std::setw(5) << std::setfill(' ') << r[0] - r[1] << " |\n";
                rFile << "|  0  |   1   |  0  |  0  | " << std::setw(5) << std::setfill(' ') << r[2] << " | " << std::setw(5) << std::setfill(' ') << r[0] - r[2] << " |\n";
                rFile << "|  0  |   0   |  1  |  0  | " << std::setw(5) << std::setfill(' ') << r[3] << " | " << std::setw(5) << std::setfill(' ') << r[0] - r[3] << " |\n";
                rFile << "|  0  |   0   |  0  |  1  | " << std::setw(5) << std::setfill(' ') << r[4] << " | " << std::setw(5) << std::setfill(' ') << r[0] - r[4] << " |\n";
                rFile << "|  1  |   1   |  1  |  1  | " << std::setw(5) << std::setfill(' ') << r[5] << " | " << std::setw(5) << std::setfill(' ') << r[0] - r[5] << " |\n";
                rFile << "+-----+-------+-----+-----+-------+-------+\n";
                rFile.close();
                //OutputDebugString("Benchmark complete. See $(OGRE_HOME)/bin/debug/BenchmarkResults.txt for the results.\n");
                Ogre::CompositorManager::getSingleton().removeCompositor(mCamera->getViewport(), "HDR");

        mGraphicsState = IN_GAME;
        }
        else
        {
                startBenchmark(stage+1);
        }
}




/*------------------------------ SPLASH SCREEN CLASS ------------------------------*/

SplashScreen::SplashScreen (Ogre::Root* root) : resourceTotal(0), resourceCount(0), mRoot(root)
{
    // Preload resources (for the splash screen)
    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("ClientSplash");
}

SplashScreen::~SplashScreen (void)
{
}

void SplashScreen::draw (int width, int height)
{
    // Create the splash screen overlay
    splashOverlay = Ogre::OverlayManager::getSingleton().create("OVERLAY_SPLASH");
    splashOverlay->setZOrder(500);
    splashOverlay->show();

    // Create an overlay container and add the splash screen to it.
    splashContainer = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", "SplashScreen"));
    splashContainer->setMetricsMode(Ogre::GMM_PIXELS);
    splashContainer->setHorizontalAlignment(Ogre::GHA_LEFT);
    splashContainer->setVerticalAlignment(Ogre::GVA_TOP);
    splashContainer->setDimensions(width, height);
    splashContainer->setMaterialName("CollisionDomain/SplashScreen");
    splashContainer->setPosition(0, 0);
    splashOverlay->add2D(splashContainer);

    // Add the loading bar frame to the splash screen.
    loadingFrame = Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", "LoadingFrame");
    loadingFrame->setMetricsMode(Ogre::GMM_PIXELS);
    loadingFrame->setDimensions(630, 40);
    loadingFrame->setMaterialName("CollisionDomain/LoadingFrame");
    loadingFrame->setPosition(width/2 - 630/2, height-50);
    splashContainer->addChild(loadingFrame);

    // Add the loading bar text to the splash screen.
    loadingText = Ogre::OverlayManager::getSingleton().createOverlayElement("TextArea", "LoadingText");
    loadingText->setMetricsMode(Ogre::GMM_PIXELS);
    loadingText->setDimensions(600, 50);
    loadingText->setPosition(width/2, height-60);
    loadingText->setParameter("font_name", "DejaVuSans");
    loadingText->setParameter("char_height", "12");
    loadingText->setParameter("alignment", "center");
    loadingText->setColour(Ogre::ColourValue(0, 0, 0));
    loadingText->setCaption("");
    splashContainer->addChild(loadingText);

    // Add the loading bar to the splash screen.
    loadingBar = Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", "LoadingBar");
    loadingBar->setMetricsMode(Ogre::GMM_PIXELS);
    loadingBar->setDimensions(0, 10);
    loadingBar->setMaterialName("CollisionDomain/LoadingBar");
    loadingBar->setPosition(width/2 - 600/2, height-35);
    splashContainer->addChild(loadingBar);

    // Render the screen.
    forceRedraw();
}

void SplashScreen::clear (void)
{
    splashOverlay->clear();
}

void SplashScreen::updateProgressBar (int percent, const Ogre::DisplayString& text)
{
    loadingText->setCaption(text);
    updateProgressBar(percent);
}

void SplashScreen::updateProgressBar (int percent)
{
    loadingBar->setDimensions(percent*6, 10);
    forceRedraw();
}

void SplashScreen::forceRedraw (void)
{
    mRoot->renderOneFrame();
}

void SplashScreen::resourceGroupScriptingStarted (const Ogre::String &groupName, size_t scriptCount)
{
    if (scriptCount > 10)
    {
        resourceTotal = scriptCount;
        resourceCount = 0;
    }
}

void SplashScreen::scriptParseEnded (const Ogre::String &scriptName, bool skipped)
{
    if (resourceTotal > 0)
        updateProgressBar(++resourceCount * ((float) (100 / resourceTotal) * 0.5f));
}




/*------------------------------ MAIN FUNCTION ------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain (HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
    int main (int argc, char *argv[])
#endif
    {

// collect command line arguments into a platform-independant mush.
/*#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        char** argv = NULL;
        int argc = 0;

        int i = 0;
        while (strCmdLine[i] != 0)
            if (strCmdLine[i++] == ' ')
                argc++;
        if (i > 0)
            argc++;

        if (argc > 0)
        {
            argv = (char**) calloc(sizeof (char*), argc);

            char* split_ptr;
            split_ptr = strtok(strCmdLine, " ");
            i = 0;
            while (split_ptr != NULL)
            {
                argv[i++] = split_ptr;
                split_ptr = strtok(NULL, " ");
            }
        }
#endif*/


//linux default stack size is smaller the visual studio c++
#ifdef linux
    	//set stack size to 32 mb
    	const rlim_t stackSize = 64 * 1024 *1024;
    	struct rlimit rl;
    	int result = getrlimit(RLIMIT_STACK, &rl);

    	if(!result)
    	{
    		if(rl.rlim_cur < stackSize)
    		{
    			//set the new stacksize
    			rl.rlim_cur = stackSize;
    			result = setrlimit(RLIMIT_STACK, &rl);
    			if(result != 0)
    			{
    				fprintf(stderr, "Setting the stack size return with error code %d\n", result);
    			}
    		}
    	}
#endif

        // set graphical options if under linux (windows gets a fancy-not-at-all-hacky message box)
/*#ifdef linux
        if (argc > 1)
        {
            if (strcmp(argv[1], "--graphics=0"))
                g_GraphicalLevel = 0;
            else if (strcmp(argv[1], "--graphics=1"))
                g_GraphicalLevel = 1;
            else if (strcmp(argv[1], "--graphics=2"))
                g_GraphicalLevel = 2;
        }
#endif*/

        // Create application object
        ClientGraphics application;

        try
        {
            application.go();
        }
        catch (Ogre::Exception& e)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox(NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " << e.getFullDescription().c_str() << std::endl;
#endif
        }

        return 0;
    }
#ifdef __cplusplus
}
#endif
