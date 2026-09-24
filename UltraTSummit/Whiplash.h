#ifndef Whiplash_H
#define Whiplash_H

#include "PID.h"

void whiplash() { 
  // andar para frente e, no momento em que um dos sensores laterais detectarem, acionar e travar em iSeeYou()
 motor.move_for_then(1023,1023, 300,
                     1023,-1023, 50);
 iSeeYou();
}

#endif
