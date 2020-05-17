#pragma once
int concurrent_start();
int concurrent_end();
void takeControl();
bool processInControl();
void setCameraString(std::string cam);
std::string getCameraString();