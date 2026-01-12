#pragma once

#include <set>
#include <unordered_map>
#include <map>
#include <string>
#include <string_view>
#include <list>
#include <Core/KeyCodes.h>
#include <vector>
#include <functional>

//Based on https://github.com/grimtraveller/scribblings-by-apoch/tree/master/inputmapping

struct MappedInput{
	std::set<size_t> Actions;
	std::set<size_t> States;


	void ConsumeAction(size_t action){ Actions.erase(action); }
	void ConsumeState(size_t state){ States.erase(state); }


};



class InputManager{
public:
	InputManager();
	void LoadContexts(const std::string &contextList, const std::string &directory, size_t (*GetActionId)(std::string_view), size_t (*GetStateId)(std::string_view));
	void PushContext(const std::string &name);
	void PopContext();
	void Clear();
	void Dispatch();
	void AddCallback(std::function<void(MappedInput&)> callback, int priority);
	void SetKeyState(KeyCodes key, bool pressed, bool previouslyPressed);

protected:


private:
	bool MappedAction(KeyCodes button,size_t &outAction);
	bool MappedState(KeyCodes button, size_t &outState);
	void ConsumeMapped(KeyCodes button);
	void ParseContext(const std::string &filePath,
		std::unordered_map<KeyCodes, size_t> &outActionMap,
		std::unordered_map<KeyCodes, size_t> &outStateMap,
		size_t (*GetActionId)(std::string_view),
		size_t (*GetStateId)(std::string_view));
	MappedInput currentMappedInput_;
	std::vector<std::unordered_map<KeyCodes, size_t>> actionMaps_;
	std::vector<std::unordered_map<KeyCodes, size_t>> stateMaps_;
	std::unordered_map<std::string, size_t> contextList_;
	std::list<size_t> activeContexts_;
	std::multimap<int,std::function<void(MappedInput &)>> callbackList_;



};