#include <Loop/LoopManager.h>
#include "PreparedStatement.h"
#include "Audio/AudioFileSourceSerialFlash.h"
#include "Audio/Playback.h"

PreparedStatement::~PreparedStatement(){
	for(auto* result : TTSresults){
		if(result == nullptr) return;
		TextToSpeech.releaseRecording(result->filename);
		delete result;
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
		if(part.type != Part::TTS) continue;

		if(j >= TTSresults.size()){
			TTSresults.push_back(nullptr);
			TextToSpeech.addJob({ static_cast<const char*>(part.content), &TTSresults.back() });
			return;
		}else{
			if(TTSresults[j] == nullptr) return;
		}

		j++;
	}


	CompositeAudioFileSource* source = new CompositeAudioFileSource();
	int i = 0;
	TTSError error = TTSError::OK;
	for(const Part& part : parts){
		if(part.type == Part::SAMPLE){
			source->add(static_cast<AudioFileSource*>(part.content));
		}else{
			if(TTSresults[i]->error != TTSError::OK){
				error = TTSresults[i]->error;
			}else{
				source->add(new AudioFileSourceSerialFlash(TTSresults[i]->filename, TTSresults[i]->size));
			}
			i++;
		}
	}

	LoopManager::removeListener(this);

	if(playCallback != nullptr){
		playCallback(error, source);
	}else{
		delete source;
	}
}

void PreparedStatement::prepare(void (*playCallback)(TTSError error, CompositeAudioFileSource* source)){
	this->playCallback = playCallback;

	TTSresults.reserve(parts.size());

	for(const Part& part : parts){
		if(part.type == Part::TTS){
			TTSresults.push_back(nullptr);
			TextToSpeech.addJob({ static_cast<const char*>(part.content), &TTSresults.back() });
			break;
		}
	}

	LoopManager::addListener(this);
}
