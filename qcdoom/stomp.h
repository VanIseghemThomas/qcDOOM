#ifndef STOMP_H
#define STOMP_H

const static int N_STOMPS = 12;

enum Action
{
    Button = 0,
    Rotary = 1,
};

struct StompMessage{
    enum Action action;
    unsigned int value;
    unsigned int timestamp;
};

enum KnobValue
{
    Released = 0,
    Pressed = 1,
    TurnedRight = 2,
    TurnedLeft = 3,
};

void OpenFiles(int fds[N_STOMPS]);
void CloseFiles(FILE *files[N_STOMPS]);
struct StompMessage **ReadStomps(int *files[N_STOMPS]);

#endif /* STOMP_H */