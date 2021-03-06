
#include "stdafx.h"
#include "MeshDeformer.h"
#include <limits.h>


MeshDeformer::MeshDeformer(void) {
	//DBOUT("MeshDeformer Instanciated\n");
}

void MeshDeformer::deformMesh(
		const Ogre::MeshPtr    mesh,
		const Ogre::Vector3    &epicentre,
		float				   damage,
        bool                   isFront,
        const Ogre::Vector3    &position,
        const Ogre::Quaternion &orient,
		const Ogre::Vector3    &scale) {

	bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

	float dentSize = damage / 600;
    if(isFront) dentSize *= 0.2f;
	float distanceCheck = 55 + (15 * dentSize);
 
    // Run through the submeshes again, adding the data into the arrays
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i) {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);
 
        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
 
        if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared)) {
            if(submesh->useSharedVertices) {
                added_shared = true;
                shared_offset = current_offset;
            }
 
            const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
			//Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY
			unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL)); //READ_ONLY

            //Ogre::Real* pReal;
            float* pReal;
            
			//Ogre::Vector3 tp(100, 90+(-10+(rand()%20)), 250+(-10+(rand()%20)));
			Ogre::Vector3 diff;
			Ogre::Vector3 origin(0, 30, 0);
			// x0+d*(x1-x0)/|x1-x0|

			int once = 0;
 
            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize()) {
				
                posElem->baseVertexPointerToElement(vertex, &pReal);
                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
                //pt = (orient * (pt * scale)) + position;
				// x0+d*(x1-x0)/|x1-x0|
				//std::stringstream ss;
				//ss << "dist " << pt.distance(epicentre) << "\n";
				//OutputDebugString(ss.str().c_str());
				if(pt.distance(epicentre) <= distanceCheck) {
					//OutputDebugString("deform\n");
					diff = origin - pt;
					diff.normalise();
					diff *= 10.f+(dentSize*10.f);
                    

					pReal[0] += (diff.x);//*(-(diff.length()*0.5f) + uniform_deviate(rand())*(diff.length()*0.5f));
					pReal[1] += (diff.y);//*(-(diff.length()*0.5f) + uniform_deviate(rand())*(diff.length()*0.5f));
					pReal[2] += (diff.z);//*(-(diff.length()*0.5f) + uniform_deviate(rand())*(diff.length()*0.5f));
				}
            }
            
            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }
		
    }

}



void MeshDeformer::traceNodeHierarchy(Ogre::SceneNode *rootnode) {
	Ogre::SceneNode::ObjectIterator oi = rootnode->getAttachedObjectIterator();
	while(oi.hasMoreElements()) {
		//DBOUT(((Ogre::Entity*)oi.getNext())->getName() << "\n");
	}
	//DBOUT("number child objects: " << rootnode->numAttachedObjects() << "\n");
	//DBOUT("number ofchild nodes: " << rootnode->numChildren() << "\n");
}

void MeshDeformer::collisonDeform(Ogre::SceneNode *vehicle, const Ogre::Vector3 &epicentre, float damage, bool isFront) {
	Ogre::Vector3 adjust = vehicle->convertWorldToLocalPosition(epicentre);
	Ogre::SceneNode::ChildNodeIterator kids = vehicle->getChildIterator();
	Ogre::SceneNode::ObjectIterator childEnts = vehicle->getAttachedObjectIterator();
	Ogre::Entity *currentEnt;
	std::string currentName;
	while(childEnts.hasMoreElements()) {
		currentEnt = (Ogre::Entity*) childEnts.getNext();
		
		currentName = currentEnt->getName();
		//std::strstr(currentName.c_str
		if(std::strstr(currentName.c_str(), "UnIqUe_") != NULL) {
			/*std::stringstream ss;
	 		ss << "deform: " << currentEnt->getName() << "\n";
			OutputDebugString(ss.str().c_str());*/
			deformMesh(currentEnt->getMesh(), adjust, damage, isFront);
		}

		//deformMesh(currentEnt->getMesh(), adjust);
	}

	Ogre::SceneNode *currentChild;
	while(kids.hasMoreElements()) {
		currentChild = (Ogre::SceneNode*) kids.getNext();
		collisonDeform(currentChild, epicentre, damage, isFront);
	}
	//DBOUT("adjusted: " << adjust << "\n");
}

unsigned MeshDeformer::time_seed() {
    time_t now = time ( 0 );
    unsigned char *n = (unsigned char *)&now;
    unsigned seed = 0;
    for (size_t z = 0; z < sizeof(now); z++) {
        seed = seed * (UCHAR_MAX + 2U) + n[z];
    }
    return seed;
}

double MeshDeformer::uniform_deviate ( int seed ) {
    return seed * ( 1.0 / ( RAND_MAX + 1.0 ) );
}

static unsigned int lineCounter = 0;

#if _WIN32
    Ogre::ManualObject* MeshDeformer::drawLine(Ogre::SceneManager *sm, Ogre::SceneNode* parent, Ogre::Vector3 &start, Ogre::Vector3 &end, Ogre::ColourValue &col)
#else
    Ogre::ManualObject* MeshDeformer::drawLine(Ogre::SceneManager* sm, Ogre::SceneNode* parent, Ogre::Vector3 start, Ogre::Vector3 end , Ogre::ColourValue col)
#endif
{
    std::stringstream ss;
    ss << "LINE" << (lineCounter++);
    Ogre::String n = ss.str().c_str();
    
	Ogre::ManualObject* manual = sm->createManualObject(n);
    
	manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);
    manual->colour(col);
	manual->position(start);
	manual->position(end);
	manual->end();
	parent->createChildSceneNode("cont"+n)->attachObject(manual);
	return manual;
}

