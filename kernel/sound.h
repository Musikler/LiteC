#ifndef SOUND_H
#define SOUND_H
void speaker_play(unsigned int frequency);
void speaker_stop(void);
void speaker_beep(unsigned int frequency, unsigned int duration_ms);
#endif