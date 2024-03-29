#include "Animation.h"
#include <Util/gifdec.h>
#include <memory>

Animation::Animation(fs::File* _file) : file(_file)
{
	auto* gif = CircuitOS::gd_open_gif(*file);
	// Serial.printf("width: %d, height: %d\n", gif->width, gif->height);
	while (gd_get_frame(gif) == 1) {
		uint8_t *buffer = (uint8_t*)malloc(gif->width * gif->height);
		//render 24-bit color frame into buffer
		gd_render_frame(gif, buffer, 1);
		frames.push_back({ buffer, static_cast<uint>(gif->gce.delay*10) });
	}

	width = gif->width;
	height = gif->height;

	gd_close_gif(gif);

	for(AnimationFrame frame : frames){
		totalDuration+=frame.duration;
	}
}

Animation::Animation(fs::FileImpl* _fileImpl) : Animation(new fs::File(fs::FileImplPtr(_fileImpl)))
{
}

Animation::~Animation()
{
	for(AnimationFrame i : frames){
		free(i.data);
	}
	delete file;
}

void Animation::rewind()
{
	currentFrame = 0;
}

AnimationFrame* Animation::getNextFrame()
{
	if(currentFrame >= frames.size()) return nullptr;
	return &frames[currentFrame++];
}

uint16_t Animation::getWidth()
{
	return width;
}

uint16_t Animation::getHeight()
{
	return height;
}
uint32_t Animation::getLoopDuration()
{
	return totalDuration;
}