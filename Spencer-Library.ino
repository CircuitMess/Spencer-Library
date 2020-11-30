#include <Arduino.h>
#include "src/Spencer.hpp"

Spencer spencer;

void setup(){
	spencer.begin();
	spencer.startLoopTask();
}

void loop(){


}