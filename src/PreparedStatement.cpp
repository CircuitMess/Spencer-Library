#include <Loop/LoopManager.h>
#include "PreparedStatement.h"
#include "Audio/AudioFileSourceSerialFlash.h"
#include "Audio/Playback.h"

PreparedStatement::~PreparedStatement(){
	for(const char* filename : files){
		if(filename == nullptr) return;
		TextToSpeech.releaseRecording(filename);
	}
}

void PreparedStatement::addSample(AudioFileSource* sample){
	parts.push_back({ Part::SAMPLE, sample });
}

bool PreparedStatement::addTTS(const char* text){
	if(strlen(text) > 130) return false;
	parts.push_back({ Part::TTS, (void*) text });
	return true;
}

void PreparedStatement::loop(uint micros){
	for(TTSError error : errors){
		if(error != TTSError::OK && error != TTSError::UNDEFINED){
			LoopManager::removeListener(this);
			if(playCallback != nullptr){
				playCallback(error, nullptr);
			}
			return;
		}
		if(error == TTSError::UNDEFINED) return;
	}


	CompositeAudioFileSource* source = new CompositeAudioFileSource();
	int i = 0;
	for(const Part& part : parts){
		if(part.type == Part::SAMPLE){
			source->add(static_cast<AudioFileSource*>(part.content));
		}else{
			source->add(new AudioFileSourceSerialFlash(files[i], fileSizes[i]));
			i++;
		}
	}

	LoopManager::removeListener(this);

	if(playCallback != nullptr){
		playCallback(TTSError::OK, source);
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

		}
	}

	LoopManager::addListener(this);
}
