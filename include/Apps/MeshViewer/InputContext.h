#pragma once
#include <string_view>

namespace InputContext{
	enum class Actions : uint8_t{
		ToggleFullscren ,
		ToggleVsync,
		CycleMeshUp ,
		CycleMeshDown,
		MoveCameraUp,
		MoveCameraDown,
		MoveCameraLeft,
		MoveCameraRight,
		MoveCameraForward,
		MoveCameraBack,
		ZoomIn,
		ZoomOut,
		Quit,
		None
	};
	enum class States : uint8_t{
		CameraMovingUp,
		CameraMovingDown,
		CameraMovingRight,
		CameraMovingLeft,
		None
	};
	size_t GetActionId(std::string_view action);
	size_t GetStateId(std::string_view state);
}
