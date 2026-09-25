#ifndef placa_H
#define placa_H
/* ! os sensores do meio na frente tão ligados com esses "pinos" (não usar) ! */
#define S0 VP
#define S1 VIN
/* ! ^^^^ ! */

#define S2 34  //frontal direita
#define S3 35  //frontal esquerda
#define S4 32  //lateral direita
#define S5 33  //lateral esquerda
#define S6 25  //linha esquerda
#define S7 27
#define S8 14  //linha direita

#define IR_PIN 15
#define LED_STRIP 2

#define D26 26
#define BTN2 12

#define MA2 19
#define MA1 18
#define MB1 4
#define MB2 23

void setupPortas() {
  pinMode(S3, INPUT);
  pinMode(S5, INPUT);
  pinMode(S8, INPUT);
  pinMode(S2, INPUT);
  pinMode(S4, INPUT);
  pinMode(S6, INPUT);
  pinMode(LED_STRIP, OUTPUT);
}

#endif