#include "PCH.h"
#include "enbhelper.h"
#include <shared_mutex> // Explicitly added to resolve 'shared_mutex' and 'shared_lock' errors

extern std::shared_mutex stateMutex;

struct ThreadCachedData {
    float time;
    float weatherTransition;
    unsigned long currentWeatherID;
    unsigned long outgoingWeatherID;
    int currentWeatherClass;
    int outgoingWeatherClass;
    unsigned long locationID;
    unsigned long worldSpaceID;
    unsigned long skyMode;
    bool isInterior;
    RE::NiTransform cameraLocal;
    RE::NiTransform cameraWorld;
    RE::NiTransform cameraOldWorld;
};
extern ThreadCachedData cachedData;

extern "C" __declspec(dllexport) bool GetTime(float& time)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	time = cachedData.time;
	return true;
}

extern "C" __declspec(dllexport) bool GetWeatherTransition(float& t)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	t = cachedData.weatherTransition;
	return true;
}

extern "C" __declspec(dllexport) bool GetCurrentWeather(unsigned long& id)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	id = cachedData.currentWeatherID;
	return true;
}

extern "C" __declspec(dllexport) bool GetOutgoingWeather(unsigned long& id)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	id = cachedData.outgoingWeatherID;
	return true;
}

extern "C" __declspec(dllexport) bool GetCurrentWeatherClassification(int& c)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	c = cachedData.currentWeatherClass;
	return true;
}

extern "C" __declspec(dllexport) bool GetOutgoingWeatherClassification(int& c)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	c = cachedData.outgoingWeatherClass;
	return true;
}

extern "C" __declspec(dllexport) bool GetCurrentLocationID(unsigned long& id)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	id = cachedData.locationID;
	return true;
}

extern "C" __declspec(dllexport) bool GetWorldSpaceID(unsigned long& id)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	id = cachedData.worldSpaceID;
	return true;
}

extern "C" __declspec(dllexport) bool GetSkyMode(unsigned long& mode)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	mode = cachedData.skyMode;
	return true;
}

extern "C" __declspec(dllexport) bool GetPlayerCameraTransformMatrices(RE::NiTransform& m_local, RE::NiTransform& m_world, RE::NiTransform& m_oldworld)
{
	std::shared_lock<std::shared_mutex> lock(stateMutex);
	m_local = cachedData.cameraLocal;
	m_world = cachedData.cameraWorld;
	m_oldworld = cachedData.cameraOldWorld;
	return true;
}

extern "C" __declspec(dllexport) bool IsLoaded() {
	return bLoaded;
}
