#ifndef SPENCER_TEXT2SPEECH_H
#define SPENCER_TEXT2SPEECH_H

#include <HTTPClient.h>
#include <set>
#include "../Util/StreamableHTTPClient.h"
#include "../AsyncProcessor.hpp"

enum class TTSError { OK = 0, NETWORK, FILE, JSON, KEY, UNDEFINED, CHARLIMIT };
struct TTSJob {
	const char* text;
	const char** resultFilename;
	TTSError *error;
	uint32_t* size;
};
class TextToSpeechImpl : public AsyncProcessor<TTSJob> {
public:
	TextToSpeechImpl();

	void releaseRecording(const char* filename);

protected:
	void doJob(const TTSJob& job) override;

private:
	TTSError generateSpeech(const char* text, uint32_t* size, const char* filename = "speech.mp3");
	bool processStream(WiFiClient& stream, const char* filename, uint32_t* size);
	void readUntilQuote(WiFiClient& stream);

	std::set<const char*> fileStash;
	Mutex stashMut;
};

extern TextToSpeechImpl TextToSpeech;
#endif