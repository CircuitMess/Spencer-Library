#ifndef SPENCER_PREPAREDSTATEMENT_H
#define SPENCER_PREPAREDSTATEMENT_H

#include <vector>
#include <AudioFileSource.h>
#include "Audio/CompositeAudioFileSource.h"
#include <Loop/LoopListener.h>
#include "Speech/TextToSpeech.h"
class PreparedStatement : public LoopListener {
public:
	virtual ~PreparedStatement();

	void addSample(AudioFileSource* sample);
	/**
	 * @brief Adds a text to be downloaded as speech and added to the CompositeAudioFileSource.
	 *        Limited to 4 samples in a single PreparedStatement!
	 *
	 * 
	 * @param text Text to be converted to speech, downloaded and queued up.
	 */
	void addTTS(const char* text);

	/**
	 * @brief Does the TTS downloading and combining into a single audio file. Executes callback when done or when error occurs.
	 * 
	 * @param playCallback Callback to be executed when done or when error occurs.
	 * @param error Enum to indicate error. (OK = 0)
	 * @param source AudioFileSource pointer to combined file. Is nullptr if error occured.
	 */
	void prepare(void (*playCallback)(TTSError error, CompositeAudioFileSource* source));
	void loop(uint micros) override;

private:
	struct Part {
		enum { SAMPLE, TTS } type;
		void* content;
	};
	std::vector<Part> parts;
	std::vector<const char*> files;
	std::vector<uint32_t> fileSizes;
	std::vector<TTSError> errors;

	void (*playCallback)(TTSError error, CompositeAudioFileSource* source) = nullptr;
};


#endif //SPENCER_PREPAREDSTATEMENT_H
