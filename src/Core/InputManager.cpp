#include <Core/InputManager.h>
#include <Utils/FileTools.h>
#include <Utils/StringTools.h>
#include <charconv>


InputManager::InputManager(){

}


void InputManager::LoadContexts(const std::string &contextList, const std::string &directory, size_t (*GetActionId)(std::string_view), size_t (*GetStateId)(std::string_view)){
	std::vector<char> buffer;
	std::string filePath = "";
	filePath.append(directory);
	filePath.append(contextList);
	FileTools::LoadFileToBuffer(filePath, &buffer);
	std::vector<std::string_view> lines;
	StringTools::ParseLines(0, buffer.data(), buffer.size(), &lines);
	std::vector<std::string_view> tokenList;
	tokenList.reserve(2);
	std::string_view token;
	std::string name;
	std::string fileName;
	std::string_view line;
	for(int i = 0; i<lines.size();++i){
		line = lines[i];
		tokenList.clear();
		token="";
		StringTools::ParseString(line, ' ', &tokenList);
		if(tokenList.size()==1){
			token = tokenList.at(0);
			size_t num;
			std::from_chars(token.data(), token.data()+token.size(), num);
			actionMaps_.reserve(num);
			stateMaps_.reserve(num);
			continue;
		}
		name = tokenList.at(0);
		contextList_[name]= i-1;
		fileName = tokenList.at(1);
		std::unordered_map<KeyCodes, size_t> actionMap;
		std::unordered_map<KeyCodes, size_t> stateMap;
		
		filePath="";
		filePath.append(directory);
		filePath.append(fileName);
		ParseContext(filePath, actionMap, stateMap, GetActionId, GetStateId);
		actionMaps_.push_back(std::move(actionMap));
		stateMaps_.push_back(std::move(stateMap));
	}
}

void InputManager::PushContext(const std::string &name){
	auto iter = contextList_.find(name);
	if(iter!=contextList_.end()){
		activeContexts_.push_front(iter->second);
	}
}

void InputManager::PopContext(){
	activeContexts_.pop_front();
}

void InputManager::Clear(){
	currentMappedInput_.Actions.clear();
}

void InputManager::Dispatch(){
	MappedInput input = currentMappedInput_;
	for(auto iter = callbackList_.begin(); iter!=callbackList_.end(); ++iter){
		(iter->second)(input);
	}
}

void InputManager::AddCallback(std::function<void(MappedInput &)> callback, int priority){
	callbackList_.insert(std::make_pair(priority, callback));
}

void InputManager::SetKeyState(KeyCodes key, bool pressed, bool previouslyPressed){
	size_t action;
	size_t state;
	if(pressed&&!previouslyPressed){
		if(MappedAction(key, action)){
			currentMappedInput_.Actions.insert(action);
			return;
		}
	}
	if(pressed){
		if(MappedState(key, state)){
			currentMappedInput_.States.insert(state);
			return;
		}
	
	}
	ConsumeMapped(key);
}

void InputManager::ParseContext(const std::string &filePath,
	std::unordered_map<KeyCodes, size_t> &outActionMap,
	std::unordered_map<KeyCodes, size_t> &outStateMap,
	size_t (*GetActionId)(std::string_view),
	size_t (*GetStateId)(std::string_view)){

	std::vector<char> contextBuffer;
	FileTools::LoadFileToBuffer(filePath, &contextBuffer);
	std::vector<std::string_view> lines;
	StringTools::ParseLines(0, contextBuffer.data(), contextBuffer.size(), &lines);

	std::string_view token = lines.at(0);
	size_t numActions = 0;
	std::from_chars(token.data(), token.data()+token.size(), numActions);
	std::string_view line;
	std::vector<std::string_view> tokenList;
	for(size_t i = 1; i<numActions+1; ++i){
		tokenList.clear();
		line = lines.at(i);
		StringTools::ParseString(line, ' ', &tokenList);
		token = tokenList.at(0);
		std::string actionName(token);
		size_t action = GetActionId(actionName);
		token = tokenList.at(1);
		KeyCodes key = KeyCodes::None;
		std::string keyString(token);
		auto iter = keyLookup.find(keyString);
		if(iter!=keyLookup.end()){
			key = iter->second;
		}
		outActionMap[key] = action;
	}
	token = lines.at(numActions+1);
	size_t numStates = 0;
	std::from_chars(token.data(), token.data()+token.size(), numStates);
	for(size_t i = numActions+2; i<numActions+2+numStates; ++i){
		tokenList.clear();
		line = lines.at(i);
		StringTools::ParseString(line, ' ', &tokenList);
		token = tokenList.at(0);
		std::string stateName(token);
		size_t state = GetStateId(stateName);
		token = tokenList.at(1);
		KeyCodes key = KeyCodes::None;
		std::string keyString(token);
		auto iter = keyLookup.find(keyString);
		if(iter!=keyLookup.end()){
			key = iter->second;
		}
		outStateMap[key] = state;
	}

}


bool InputManager::MappedAction(KeyCodes button, size_t &outAction){
	for(auto iter = activeContexts_.begin(); iter!=activeContexts_.end(); ++iter){
		auto mapIter = actionMaps_[*iter].find(button);
		if(mapIter!=actionMaps_[*iter].end()){
			outAction = mapIter->second;
			return true;
		}
	}
	return false;
}

bool InputManager::MappedState(KeyCodes button, size_t &outState){
	for(auto iter = activeContexts_.begin(); iter!=activeContexts_.end(); ++iter){
		auto mapIter = stateMaps_[*iter].find(button);
		if(mapIter!=stateMaps_[*iter].end()){
			outState = mapIter->second;
			return true;
		}
	}
	return false;

}

void InputManager::ConsumeMapped(KeyCodes button){
	size_t action;
	size_t state;

	if(MappedAction(button, action)){
		currentMappedInput_.ConsumeAction(action);
	}

	if(MappedState(button, state)){
	
		currentMappedInput_.ConsumeState(state);
	}

}


