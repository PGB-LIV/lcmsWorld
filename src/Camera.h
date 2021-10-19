#pragma once
#include "Structs.h"
#include "Landscape.h"
#include "glm/glm.hpp"

class Camera
{

public:
	Camera(Landscape* l);

	void updateCamera(glm::vec2 mouse) {}

	void updateCameraTarget(DataPoint newTarget, int db, int button, bool centre);
		void updateCamera(double time);
	glm::mat4 getViewMatrix() {
		return GlobalViewMatrix;
	}
	glm::mat4 getDirectionMatrix() {
		return GlobalDirectionMatrix;
	}
 
	void dragZoom(double z, double time);
	void dragRotate(double z, double time);
	void wheelMove(double mouse_wheel_pos, double time);
	DataPoint getTarget() { return currentTarget; }
	void dragTranslate(double x, double y, double time, int status);
	std::string to_string(DataPoint dp);
	DataPoint from_string(std::string);

	double distance = 0;

	DataPoint currentTarget = { 0,0,0 };
	void reset();

private:
	enum statusType { free, targeted, targeting, zooming, zoomTargeting };
	glm::mat4 GlobalViewMatrix;
	glm::mat4 GlobalDirectionMatrix;
	
	Landscape* owner = NULL;
	statusType status = statusType::free;
	double elevation = 0;
	glm::vec3 unitDirection = { 0,0,0 };
	
	glm::vec3 currentPos = { 0,0,0 };
	glm::vec3 position = { 0,0,0 };
	double aimAngle =0 ;
	double elevationAngle = 0;

	double dragSpeed = 0;
	double spinSpeed = 0;

	DataPoint localTarget = { 0,0,0 };

	DataPoint target = { 0,0,0 };
	DataPoint lastTarget = { 0,0,0 };

	double targetTime = 0;
	double timeToTarget = 0;
	double zoomSpeed = 0;


};