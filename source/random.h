// A simple random number generator for the whole game.
#ifndef RANDOM_H
#define RANDOM_H

// Returns a random whole number from min up to (but not including) max.
int random_range(int min, int max);

// Starts a new random sequence, so each game is different.
void random_seed(unsigned int value);

#endif
