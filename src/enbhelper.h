#pragma once

extern bool bLoaded;

extern "C" {
	__declspec(dllexport) bool GetTime(float& time);
	__declspec(dllexport) bool GetWeatherTransition(float& t);
	__declspec(dllexport) bool GetCurrentWeather(unsigned long& id);
	__declspec(dllexport) bool GetOutgoingWeather(unsigned long& id);
	__declspec(dllexport) bool GetCurrentWeatherClassification(int& c);
	__declspec(dllexport) bool GetOutgoingWeatherClassification(int& c);
	__declspec(dllexport) bool GetCurrentLocationID(unsigned long& id);
	__declspec(dllexport) bool GetWorldSpaceID(unsigned long& id);
	__declspec(dllexport) bool GetSkyMode(unsigned long& mode);
	__declspec(dllexport) bool GetPlayerCameraTransformMatrices(RE::NiTransform& m_local, RE::NiTransform& m_world, RE::NiTransform& m_oldworld);
	__declspec(dllexport) bool IsLoaded();
}
