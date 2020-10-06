
#include "Camera.h"
#include "MZData.h"
#include "glm/glm.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp" // after <glm/glm.hpp>
#include <sstream>
#include <iomanip>
std::string Camera::to_string(DataPoint cursor)
{

	std::ostringstream output;


	output << currentPos.x << "\n";
	output << currentPos.y << "\n";
	output << currentPos.z << "\n";

	output << position.x << "\n";
	output << position.y << "\n";
	output << position.z << "\n";


	output << aimAngle << "\n";
	output << elevationAngle << "\n";
	output << elevation << "\n";
	output << distance << "\n";
	output << dragSpeed << "\n";
	output << spinSpeed << "\n";

	output << currentTarget.mz << "\n";
	output << currentTarget.lc << "\n";
	output << currentTarget.signal << "\n";
 
	output << target.mz << "\n";
	output << target.lc << "\n";
	output << target.signal << "\n";

	output << lastTarget.mz << "\n";
	output << lastTarget.lc << "\n";
	output << lastTarget.signal << "\n";

 
	output << cursor.mz << "\n";
	output << cursor.lc << "\n";
	output << cursor.signal << "\n";

	output << targetTime << "\n";
	output << timeToTarget << "\n";
	output << zoomSpeed << "\n";
	int statusInt = (int)status;
	output << statusInt << "\n";

	output << Settings::scale.x << "\n";
	output << Settings::scale.y << "\n";
	output << Settings::scale.z << "\n";

	output << Settings::xScale_slider << "\n";;
	output << Settings::yScale_slider << "\n";;
	output << Settings::zScale_slider << "\n";;


	return output.str();
}

DataPoint Camera::from_string(std::string in_string)
{
	std::stringstream input(in_string);
	

	input >> currentPos.x ;
	input >> currentPos.y ;
	input >> currentPos.z ;

	input >> position.x ;
	input >> position.y ;
	input >> position.z ;


	input >> aimAngle ;
	input >> elevationAngle ;
	input >> elevation ;
	input >> distance ;
	input >> dragSpeed ;
	input >> spinSpeed ;

	
	input >> currentTarget.mz ;
	input >> currentTarget.lc ;
	input >> currentTarget.signal ;

	input >> target.mz ;
	input >> target.lc ;
	input >> target.signal ;

	input >> lastTarget.mz ;
	input >> lastTarget.lc ;
	input >> lastTarget.signal ;

	
	DataPoint cursor;
	input >> cursor.mz;
	input >> cursor.lc;
	input >> cursor.signal;

	input >> targetTime ;
	input >> timeToTarget;
	input >> zoomSpeed ;
	int statusInt;
	input >> statusInt;

	status = (statusType) statusInt;
	
	input >> Settings::scale.x;
	input >> Settings::scale.y;
	input >> Settings::scale.z;

	input >> Settings::xScale_slider;
	input >> Settings::yScale_slider;
	input >> Settings::zScale_slider;



	return cursor;
}

Camera::Camera(Landscape* l)
{
	status = targeted;

	mzFloat centreMz = (l->worldMzRange.max - l->worldMzRange.min) * 2/ 4 + l->worldMzRange.min;
	lcFloat centreLc = (l->worldLcRange.max + l->worldLcRange.min) / 2;

	target = DataPoint{ centreMz, centreLc, 0 };
	currentTarget = target;
	timeToTarget = 0;
	distance = 62000;
	aimAngle = 0;
	elevation = 11000;
	elevationAngle = 0.95;

	owner = l;

}

void Camera::wheelMove(double mouse_wheel_pos, double time)
{
 
	double scale = .005f;

		elevationAngle += mouse_wheel_pos *scale * Globals::mouseSpeed;
	elevationAngle = std::max(elevationAngle, 0.02);
	elevationAngle = std::min(elevationAngle, 3.14159/2 - 0.02);

}
const double MIN_DISTANCE = 15.0f;
const double MAX_DISTANCE = 185000.0f;

const double MIN_DISTANCE_ZOOMIN = 195.0f;

void Camera::dragZoom(double z, double time)
{
	//be aware, that due to a last-minute change, this is operated by the mouse wheel and not the mouse movement
	double scale = (distance * Globals::wheelSpeed);
	//if (z > 0)
	//	scale = distance + 40;
	z = -z;

	dragSpeed = (z ) / time;
	if (dragSpeed < -5000)
		dragSpeed = -5000;
	if (dragSpeed > 5000)
		dragSpeed = 5000;

 	distance += (z * scale);
	distance = std::max(distance, MIN_DISTANCE);
	distance = std::min(distance, MAX_DISTANCE);

	
}

void Camera::dragRotate(double z, double time)
{

 

	spinSpeed = -z ;
 	aimAngle += spinSpeed *Globals::mouseSpeed;
	spinSpeed /= time*100;

	if (spinSpeed < -100)
		spinSpeed = -100;
	if (spinSpeed > 100)
		spinSpeed = 100;


	if (aimAngle < 0)
		aimAngle += 3.14159265 * 2;
	if (aimAngle > 3.14159265 * 2)
		aimAngle -= 3.14159265 * 2;

	
}
void Camera::dragTranslate(double x, double y, double time, int status)
{

	double md = std::sqrt(x*x + y * y) * .90 * sqrt(Globals::mouseSpeed);
	double angle = std::atan2(x, y);
	angle += aimAngle;

	double mzDistance = sin(angle) * distance * md;
	double lcDistance = cos(angle) * distance* md;



	if (status)
	{
		localTarget.mz -= (mzFloat)(mzDistance / owner->xScale / Settings::scale.x);
		localTarget.lc -= (lcFloat)(lcDistance / owner->yScale / Settings::scale.y);
		return;
	}

	target.mz -= (mzFloat) (mzDistance / owner->xScale / Settings::scale.x);
	target.lc -= (lcFloat) (lcDistance / owner->yScale / Settings::scale.y);

}

extern float shotDistance;

void Camera::updateCameraTarget(DataPoint newTarget, int db, int button, bool nearTarget)
{

 	if (newTarget.lc < owner->worldLcRange.min) return;
	if (newTarget.lc > owner->worldLcRange.max) return;
	if (newTarget.mz < owner->worldMzRange.min) return;
	if (newTarget.mz > owner->worldMzRange.max) return;

	newTarget.signal = 0;
	auto dy = (newTarget.lc - currentTarget.lc);
	dy = (lcFloat) ( dy * Settings::scale.y * owner->yScale);

	auto dx = (newTarget.mz - currentTarget.mz);
	dx = (mzFloat) (dx * Settings::scale.x * owner->xScale);
	double dist =std::sqrt( dx * dx + dy * dy);

 
	if (0)
	if ( (nearTarget && (db ==0)) || (button == 1) )
	{
		targetTime = 1.0f;
		timeToTarget = targetTime;
		status = zooming;
		zoomSpeed = 1;
		if (button == 1)
			zoomSpeed = -1;

		return;

	}

	targetTime = 0.25 + (dist / 15000);
	targetTime = std::min(2.0, targetTime);
	
 

	lastTarget = currentTarget;
	target = newTarget;

	timeToTarget = targetTime;
	status = targeting;
	if (db)
		status = zoomTargeting;
 
 

	if (db == 3)
	{
		targetTime = 0;
		status = targeted;
		distance = shotDistance;
		aimAngle = 0.175f;
		elevationAngle = 0.7f;
	//	target.lc += 0.1;
	//	target.mz += 0.25;

	}

}

void Camera::updateCamera(double time)
{



	mzFloat centreMz = (owner->worldMzRange.max + owner->worldMzRange.min) / 2;
	lcFloat centreLc = (owner->worldLcRange.max + owner->worldLcRange.min) / 2;

	if (status == zooming)
	{

 
		timeToTarget -= time;
		if (timeToTarget < 0)
		{
			timeToTarget = 0; 
			status = targeted;
		}
		double scale = (distance + 100)*.45f;

		distance = distance - (scale * time * zoomSpeed);

		//temp bodge - should have a better way of stopping an 'auto' zoom (double-click)
		if (std::abs(zoomSpeed) > 1)
			if (distance < MIN_DISTANCE_ZOOMIN)
			{
				distance = MIN_DISTANCE_ZOOMIN;
				timeToTarget = 0;
			}
		


		if (distance < MIN_DISTANCE)
		{
			distance = MIN_DISTANCE;
			timeToTarget = 0;
		}
 
	}

	if (dragSpeed != 0)
	{
		double scale = (distance);
		if (dragSpeed > 0)
			scale = distance + 40;

		double friction = -dragSpeed * 2.5f;
 
		if (friction < 0)
			friction = std::min(friction, -0.5);
		else
			friction = std::max(friction, 0.5);

 
		double lastSpeed = dragSpeed;
		dragSpeed += friction * time;

		if ((lastSpeed * dragSpeed) < 0)
			dragSpeed = 0;
 		distance += dragSpeed * time * scale * 0.9;

		if (distance < MIN_DISTANCE)
		{
			distance = MIN_DISTANCE;
			dragSpeed = 0;
		}

 		distance = std::min(distance, MAX_DISTANCE);
	}

	if (spinSpeed != 0)
	{
 
		aimAngle += (spinSpeed * time * 1.0f);
		double lastSpeed = spinSpeed;

		double friction = -spinSpeed * 2.5f;
		if (friction < 0)
			friction = std::min(friction, -0.1);
		else
			friction = std::max(friction, 0.1);
		spinSpeed += friction * time;


		if ((lastSpeed * spinSpeed) < 0)
			spinSpeed = 0;

		if (aimAngle < 0)
			aimAngle += 3.14159265 * 2;
		if (aimAngle > 3.14159265 * 2)
			aimAngle -= 3.14159265 * 2;

 	}
	if (status == zooming)
	{
 		if (elevationAngle < 0.31)
			elevationAngle += (2*elevationAngle+.01) * time;

	}
	if ((status == targeted) || (status == zooming) )

	{
		double horizontalAngle = aimAngle;
		 

		double mzDistance = sin(horizontalAngle) * distance * cos(elevationAngle);
		double lcDistance = cos(horizontalAngle) * distance * cos(elevationAngle);

		//need to normalise to make it 'round!'

		double camMz = mzDistance ;
		double camLc = lcDistance ;
		double camHeight = sin(elevationAngle) * distance;

		currentTarget = target;

		glm::vec3 target_p(target.mz, target.signal,  target.lc); //, target.signal

		double elev = camHeight;
		glm::vec3 target_offset(localTarget.mz, localTarget.signal, localTarget.lc); //, target.signal

		target_p += target_offset;

		glm::vec3 direction(mzDistance, elev, camLc ); //camHeight
 
		target_p.x -= centreMz;
	//	target_p.y = elevation; //  centreLc;
		target_p.z -= centreLc;




		target_p.x *= owner->xScale;
		target_p.y *= owner->zScale;
		target_p.z *= owner->yScale;

		target_p.x *= Settings::scale.x;
		target_p.y *= Settings::scale.z;
		target_p.z *= Settings::scale.y;

 
		glm::vec3 up(0, 1, 0);

		currentPos = target_p + direction;

		auto di = direction;
 
		//	di.y = 60000;

		unitDirection = glm::normalize(di);
		


 		GlobalViewMatrix = glm::lookAt(
			target_p + direction,           // Camera is here
			target_p, // and looks here : at the same position, plus "direction"
			up                  // Head is up (set to 0,-1,0 to look upside-down)
		);

	 

	}



	if (status == targeting || status == zoomTargeting)
	{


		double timeDist = 1-(timeToTarget / targetTime);


		timeToTarget -= time;
		if (timeToTarget < 0)
		{
			timeToTarget = 0; // set angle andchange to targeted
			if (status == zoomTargeting)
			{
				

				targetTime = 1.1f + (distance / 80000) ;


				timeToTarget = targetTime;
				status = zooming;
				zoomSpeed = 2.5f;

			}
			else
			status = targeted;

			
		}
		 
	

		glm::vec3 target_p(target.mz - lastTarget.mz, target.signal -lastTarget.signal, target.lc-lastTarget.lc); //, target.signal
		target_p *= timeDist;
		target_p.x += lastTarget.mz;
		target_p.y += lastTarget.signal;
		target_p.z += lastTarget.lc;

		currentTarget.lc = target_p.z;
		currentTarget.mz = target_p.x;
		currentTarget.signal = target_p.y;

		target_p = glm::vec3(target_p.x - centreMz, elevation, target_p.z - centreLc);


		target_p.x *= owner->xScale;
		target_p.y *= owner->zScale;
		target_p.z *= owner->yScale;

		target_p.x *= Settings::scale.x;
		target_p.y *= Settings::scale.z;
		target_p.z *= Settings::scale.y;




	 //	glm::vec3 up = glm::cross(right, direction);

		glm::vec3 up(0, 1, 0);
		GlobalViewMatrix = glm::lookAt(
			currentPos,           // Camera is here
			target_p, // and looks here : at the same position, plus "direction"
			up                  // Head is up (set to 0,-1,0 to look upside-down)
		);

		glm::vec3 direction = currentPos - target_p;

		
		unitDirection = glm::normalize(direction);


		double tx = target_p.x - currentPos.x;
		double ty =  target_p.z - currentPos.z;
		
		distance = sqrt(tx*tx + ty * ty);
		double hdistance = distance;

		double tz = currentPos.y - target_p.y;


		distance =   sqrt(distance*distance + tz * tz);

#if 0
		tx /= owner->xScale;
		tx /= Settings::scale.x;


		ty /= owner->yScale;
		ty /= Settings::scale.y;

#endif

 		aimAngle = atan2(-tx,-ty);
	 
		elevationAngle =   atan2(tz, hdistance);

		 

	//	distance /= cos(elevationAngle);

	}

	glm::vec3 up(0, 1, 0);
	glm::vec3 zero(0, 0, 0);

	unitDirection.y += 1.15;
	unitDirection = glm::normalize(unitDirection);

	GlobalDirectionMatrix = glm::lookAt(

		unitDirection* 1000.0f, // and looks here : at the same position, plus "direction"
		zero,           // Camera is here
		up                  // Head is up (set to 0,-1,0 to look upside-down)
	);

//	GlobalViewMatrix = GlobalDirectionMatrix;


}