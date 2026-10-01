// The ages your tribe climbs through, from the first campfire to the launch pad.
#ifndef AGES_H
#define AGES_H

typedef struct {
    const char *name;
    int advance_cost;   // science needed to reach the next age
} Age;

extern const Age AGES[];
extern const int AGE_COUNT;

#endif
