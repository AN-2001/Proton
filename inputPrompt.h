#ifndef INPUT_PROMPT_H
#define INPUT_PROMPT_H
#include "proton.h"

extern char TextInput[BUFF_SIZE];

void InputPromptUpdate(RealType Delta);
void InputPromptDraw();

#endif /* INPUT_PROMPT_H */
