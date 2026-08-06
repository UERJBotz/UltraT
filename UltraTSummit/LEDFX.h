#ifndef ledFX_H
#define ledFX_H
#include "PID.h"
#include <Adafruit_NeoPixel.h>

#define LED4 3
#define LED5 4
#define LED6 5
#define LED7 7
#define LED1 0
#define PIN 2 //pino do anel de leds
#define NUMPIXELS 8 // quantidade de leds do anel
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800); // necessario

void ledBlink(int r, int g, int b, int time) {    // pisca todas os leds em um intervalo de tempo
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();   
  }
  pixels.clear();
  delay(time);
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();    
  }
  delay(time);
}
/*----------------------------------------------------------------------------------------*/
void ledLight (int r, int g, int b) {   // luz contínua
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();    
  }
}
/*----------------------------------------------------------------------------------------*/
void ledCircle(int r, int g, int b, int time) {   // luzes "andam" em círculo numa certa velocidade dependendo do tempo
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();    
    delay(time);
    pixels.clear(); 
  }
}
/*----------------------------------------------------------------------------------------*/
void ledCircleBlink(int r, int g, int b, int time) {  // luzes "andam" em círculo e piscam no final numa certa velocidade dependendo do tempo
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();    
  }
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();
    delay(time);
    pixels.clear();
  }
}


void setDefaultColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
}

  

void ledDetection() {
  setDefaultColor(150, 0, 0); // Define cor padrão
  leituraSensores();
   
  if (leitura[0]) {
    Serial.println("ESQUERDA LATERAL DETECTADO");
    pixels.setPixelColor(LED7, pixels.Color(0, 150, 0));              // LED principal
    pixels.setPixelColor((LED7 - 1) % NUMPIXELS, pixels.Color(0, 150, 0)); // LED ao lado
  }

  if (leitura[1]) {
    Serial.println("ESQUERDA FRONTAL DETECTADO");
    pixels.setPixelColor(LED5, pixels.Color(0, 0, 150));
    pixels.setPixelColor((LED5 + 1) % NUMPIXELS, pixels.Color(0, 0, 150));
  }

  if (leitura[2]) {
    Serial.println("DIREITA FRONTAL DETECTADO");
    pixels.setPixelColor(LED4, pixels.Color(0, 0, 150));
    pixels.setPixelColor((LED4 - 1) % NUMPIXELS, pixels.Color(0, 0, 150));
  }

  if (leitura[3]) {
    Serial.println("DIREITA LATERAL DETECTADO");
    pixels.setPixelColor(LED1, pixels.Color(0, 150, 0));
    pixels.setPixelColor((LED1 + 1) % NUMPIXELS, pixels.Color(0, 150, 0));
  }
  pixels.show();
  delay(10);
}




#endif

