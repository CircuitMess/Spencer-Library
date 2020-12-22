#include <sstream>
#include <SerialFlash.h>
#include "TextToSpeech.h"
#include "../DataStream/FileWriteStream.h"
#include "../Util/Base64Decode.h"
#include "../Settings.h"

#define CA "DC:03:B5:D6:0C:F1:02:F1:B1:D0:62:27:9F:3E:B4:C3:CD:C9:93:BA:20:65:6D:06:DC:5D:56:AC:CC:BA:40:20"

const char* stash[] = {
		"recording-1.mp3",
		"recording-2.mp3",
		"recording-3.mp3",
		"recording-4.mp3"
};

#define STASH_COUNT (sizeof(stash) / sizeof(stash[0]))
#define CHAR_LIMIT 130

TextToSpeechImpl TextToSpeech;

TextToSpeechImpl::TextToSpeechImpl() : AsyncProcessor("TTS_Task", STASH_COUNT), fileStash(::stash, ::stash + STASH_COUNT){

}

void TextToSpeechImpl::releaseRecording(const char* filename){
	stashMut.lock();
	fileStash.insert(filename);
	stashMut.unlock();
}

void TextToSpeechImpl::doJob(const TTSJob& job){
	

	stashMut.lock();
	if(fileStash.size() == 0){
		Serial.println("TTS file limit reached");
		stashMut.unlock();
		*job.error = TTSError::FILELIMIT;
		*job.resultFilename = nullptr;
		*job.size = 0;
		return;
	}

	const char* filename = *fileStash.begin();
	fileStash.erase(filename);
	stashMut.unlock();

	*job.error = generateSpeech(job.text, job.size, filename);
	*job.resultFilename = filename;
}

TTSError TextToSpeechImpl::generateSpeech(const char* text, uint32_t *size, const char* filename){
	const char pattern[] = "{ 'input': { 'text': '%.*s' },"
						   "'voice': {"
						   "'languageCode': 'en-US',"
						   "'name': 'en-US-Standard-D',"
						   "'ssmlGender': 'NEUTRAL'"
						   "}, 'audioConfig': {"
						   "'audioEncoding': 'MP3',"
						   "'speakingRate': 0.96,"
						   "'pitch': 5.5,"
						   "'sampleRateHertz': 16000"
						   "}}";

	char* data = (char*) malloc(sizeof(pattern) + (strlen(text) > CHAR_LIMIT ? CHAR_LIMIT : strlen(text)) + 2);
	uint length = sprintf(data, pattern, CHAR_LIMIT, text);

	StreamableHTTPClient http;
	http.useHTTP10(true);
	http.setReuse(false);
	if(!http.begin("https://spencer.circuitmess.com:8443/tts/v1/text:synthesize", CA)){
		free(data);
		return TTSError::NETWORK;
	}
	http.addHeader("Key", "AIzaSyAfH6xrdxj1cC4qtKTBgAK4wdIY_Pin4Wc");
	http.addHeader("Content-Type", "application/json; charset=utf-8");
	http.addHeader("Accept-Encoding", "identity");
	http.addHeader("Content-Length", String(length));

	if(!http.startPOST()){
		Serial.println("Error connecting");
		http.end();
		http.getStream().stop();
		http.getStream().flush();
		free(data);
		return TTSError::NETWORK;
	}

	if(!http.send(reinterpret_cast<uint8_t*>(data), length)){
		Serial.println("Error sending data");
		http.end();
		http.getStream().stop();
		http.getStream().flush();
		free(data);
		return TTSError::NETWORK;
	}
	free(data);
	int code = http.finish();
	if(code != 200){
		Serial.printf("HTTP code %d\n", code);
		http.end();
		http.getStream().stop();
		http.getStream().flush();
		return TTSError::JSON;
	}

	enum { PRE, PROP, VAL, POST } state = PRE;
	WiFiClient& stream = http.getStream();
	bool processed = false;

	while(stream.available()){
		if(state == PRE){
			readUntilQuote(stream);
			state = PROP;
		}else if(state == PROP){
			String prop = stream.readStringUntil('"');
			if(prop.equals("audioContent")){
				readUntilQuote(stream);
				state = VAL;
			}else{
				readUntilQuote(stream);
				readUntilQuote(stream);
				state = PRE;
			}
		}else if(state == VAL){
			if(!processStream(stream, filename, size)){
				return TTSError::FILE;
			}
			processed = true;
			break;
		}
	}

	http.end();
	stream.stop();
	stream.flush();

	if(!processed){
		Serial.println("Error processing stream");
		return TTSError::JSON;
	}

	return TTSError::OK;
}

bool TextToSpeechImpl::processStream(WiFiClient& stream, const char* filename, uint32_t *size){
	if(filename == nullptr){
		*size = 0;
		return false;
	}
	SerialFlash.createErasable(filename, 64000);
	SerialFlashFile file = SerialFlash.open(filename);
	if(!(file)){
		*size = 0;
		return false;
	}
	file.erase();

	FileWriteStream fileStream(file);
	Base64Decode decodeStream(&fileStream);
	uint32_t written = 0;
	unsigned char byte;
	int status;
	while(stream.available() && (status = stream.read(&byte, 1)) && byte != '"'){
		if(status != 1) continue;
		if(byte == '\n') continue;
		written+=decodeStream.write_return(byte);
	}
	*size = written;
	fileStream.flush();
	file.close();
}

void TextToSpeechImpl::readUntilQuote(WiFiClient& stream){
	unsigned char byte;
	while(stream.available() && stream.read(&byte, 1) && byte != '"');
}
