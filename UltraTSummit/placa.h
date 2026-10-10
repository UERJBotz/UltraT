#ifndef placa_H
#define placa_H
/* ! os sensores do meio na frente tão ligados com esses "pinos" (não usar) ! */
#define S0 VP // 5?
#define S1 VIN
/* ! ^^^^ ! */

#define S2 34
#define S3 35
#define S4 32
#define S5 33
#define S6 25
#define S7 27
#define S8 14

#define IR_PIN 15
#define LED_STRIP 2

#define D26 26
#define BTN2 12

#define MA2 19
#define MA1 18
#define MB1 4
#define MB2 23

#define S_ESQ S5
#define S_DIR S4
#define S_FESQ S3
#define S_FDIR S2
#define LI_ESQ S6
#define LI_DIR S8

void setupPortas() {
  pinMode(S_ESQ, INPUT);
  pinMode(S_DIR, INPUT);
  pinMode(S_FESQ, INPUT);
  pinMode(S_FDIR, INPUT);

  #ifdef LI_ESQ && LI_DIR  //! DESABILITAR LINHAS SEPARADO
    pinMode(LI_ESQ, INPUT);
    pinMode(LI_DIR, INPUT);
  #endif
  pinMode(LED_STRIP, OUTPUT);
}

#endif