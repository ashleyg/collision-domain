#ifndef __Vehicle_h_
#define __Vehicle_h_

#include "stdafx.h"
#include "SharedIncludes.h"

class Vehicle : public btRaycastVehicle
{
private:

    btAlignedObjectArray<btVector3>	m_forwardWS;
	btAlignedObjectArray<btVector3>	m_axle;
	btAlignedObjectArray<btScalar>	m_forwardImpulse;
	btAlignedObjectArray<btScalar>	m_sideImpulse;

    btScalar m_currentVehicleSpeedKmHour;

public:

    Vehicle( const btVehicleTuning& tuning,btRigidBody* chassis,	btVehicleRaycaster* raycaster )
        : btRaycastVehicle( tuning, chassis, raycaster )
    {
    }

    virtual void    updateVehicle( btScalar step );
	virtual void 	updateFriction( btScalar timeStep );

    btScalar getCurrentSpeedKmHour() const
	{
		return m_currentVehicleSpeedKmHour;
	}
	
};

#endif // #ifndef __Vehicle_h_



