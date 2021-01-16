#include <Loop/LoopManager.h>
#include "PreparedStatement.h"
#include "Audio/AudioFileSourceSerialFlash.h"
#include "Audio/Playback.h"

PreparedStatement::~PreparedStatement(){
	for(const char* filename : files){
		if(filename == nullptr) return;
		TextToSpeech.releaseRecording(filename);
	}

	for(String* string : stringTTS){
		delete string;
	}
}

void PreparedStatement::addSample(AudioFileSource* sample){
	parts.push_back({ Part::SAMPLE, sample });
}

bool PreparedStatement::addTTS(const char* text){
	if(text == nullptr) return false;
	if(strlen(text) > 130) return false;
	parts.push_back({ Part::TTS, (void*) text });
	return true;
}

bool PreparedStatement::addTTS(const String& text){
	if(text.length() == 0) return false;
	stringTTS.push_back(new String(text));
	addTTS(stringTTS.back()->c_str());
}

void PreparedStatement::loop(uint micros){
	uint8_t j = 0;
	for(const Part& part : parts){
		if(part.type == Part::TTS){
			if(j >= errors.size()){
				files.push_back(nullptr);
				errors.push_back(TTSError::UNDEFINED);
				fileSizes.push_back(0);
				TextToSpeech.addJob({ static_cast<const char*>(part.content), &files.back(), &errors.back(), &fileSizes.back() });
				return;
			}else{
				TTSError error = errors[j];
				if(error == TTSError::UNDEFINED) return;
				else if(error != TTSError::OK && error != TTSError::FILELIMIT){
					LoopManager::removeListener(this);
					if(playCallback != nullptr){
						playCallback(error, nullptr);
					}
					return;
				}
			}
			j++;
		}
	}


	CompositeAudioFileSource* source = new CompositeAudioFileSource();
	int i = 0;
	bool fileLimitError = false;
	for(const Part& part : parts){
		if(part.type == Part::SAMPLE){
			source->add(static_cast<AudioFileSource*>(part.content));
		}else{
			if(errors[i] != TTSError::FILELIMIT){
				source->add(new AudioFileSourceSerialFlash(files[i], fileSizes[i]));
			}else{
				fileLimitError = true;
			}
			i++;
		}
	}

	LoopManager::removeListener(this);

	if(playCallback != nullptr){
		playCallback(fileLimitError ? TTSError::FILELIMIT : TTSError::OK, source);
	}else{
		delete source;
	}
}

void PreparedStatement::prepare(void (*playCallback)(TTSError error, CompositeAudioFileSource* source)){
	this->playCallback = playCallback;

	files.reserve(parts.size());
	uint8_t temp = 0;
	for(Part& part : parts){
		if(part.type == Part::TTS){
			temp++;
		}
	}
	errors.reserve(temp);
	fileSizes.reserve(temp);

	for(const Part& part : parts){
		if(part.type == Part::TTS){
			files.push_back(nullptr);
			errors.push_back(TTSError::UNDEFINED);
			fileSizes.push_back(0);
			TextToSpeech.addJob({ static_cast<const char*>(part.content), &files.back(), &errors.back(), &fileSizes.back() });
			break;
		}
	}

	LoopManager::addListener(this);
}
