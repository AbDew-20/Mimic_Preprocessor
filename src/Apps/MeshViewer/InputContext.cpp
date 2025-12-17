#include <Apps/MeshViewer/InputContext.h>
#include <unordered_map>

namespace InputContext{
	namespace{
		constexpr std::pair<std::string_view, Actions> actionTable[] = {
			{"ToggleFullscreen" , Actions::ToggleFullscren}	,
			{"ToggleVsync",      Actions::ToggleVsync }       ,
			{"CycleMeshUp" ,     Actions::CycleMeshUp }       ,
			{"CycleMeshDown",    Actions::CycleMeshDown }     ,
			{"MoveCameraUp",     Actions::MoveCameraUp }      ,
			{"MoveCameraDown",   Actions::MoveCameraDown }    ,
			{"MoveCameraLeft",   Actions::MoveCameraLeft }    ,
			{"MoveCameraRight",  Actions::MoveCameraRight }   ,
			{"MoveCameraForward",Actions::MoveCameraForward } ,
			{"MoveCameraBack",   Actions::MoveCameraBack }    ,
			{"ZoomIn",           Actions::ZoomIn }            ,
			{"ZoomOut",          Actions::ZoomOut }           ,
			{"Quit",             Actions::Quit },
			{"None",			 Actions::None},
		};

		constexpr std::pair<std::string_view, States> stateTable[] = {
			{"CameraMovingUp",		States::CameraMovingUp},
			{"CameraMovingDown",	States::CameraMovingDown},
			{"CameraMovingRight",	States::CameraMovingRight},
			{"CameraMovingLeft",	States::CameraMovingLeft},
			{"None",				States::None},
		};
	
	}
	//static std::unordered_map<std::string_view, Actions> actionLookup = {
	//	{"ToggleFullscreen" , Actions::ToggleFullscren}	,
	//	{"ToggleVsync",      Actions::ToggleVsync }       ,
	//	{"CycleMeshUp" ,     Actions::CycleMeshUp }       ,
	//	{"CycleMeshDown",    Actions::CycleMeshDown }     ,
	//	{"MoveCameraUp",     Actions::MoveCameraUp }      ,
	//	{"MoveCameraDown",   Actions::MoveCameraDown }    ,
	//	{"MoveCameraLeft",   Actions::MoveCameraLeft }    ,
	//	{"MoveCameraRight",  Actions::MoveCameraRight }   ,
	//	{"MoveCameraForward",Actions::MoveCameraForward } ,
	//	{"MoveCameraBack",   Actions::MoveCameraBack }    ,
	//	{"ZoomIn",           Actions::ZoomIn }            ,
	//	{"ZoomOut",          Actions::ZoomOut }           ,
	//	{"Quit",             Actions::Quit }              
	//};

	//static std::unordered_map<std::string_view, States> stateLookup = {
	//	{"CameraMovingUp",		States::CameraMovingUp},
	//	{"CameraMovingDown",	States::CameraMovingDown},
	//	{"CameraMovingRight",	States::CameraMovingRight},
	//	{"CameraMovingLeft",	States::CameraMovingLeft}
	//};

	size_t GetActionId(std::string_view action){
		for(const auto &[name, actionId]:actionTable){
			if(name==action){
				return static_cast<size_t>(actionId);
			}
		}
		return SIZE_MAX;
	}

	size_t GetStateId(std::string_view state){
		for(const auto &[name, stateId]:stateTable){
			if(name==state){
				return static_cast<size_t>(stateId);
			}
		}
		return SIZE_MAX;
	}

}
