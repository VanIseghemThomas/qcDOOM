#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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

enum Action DecodeAction(unsigned char bytes[32]){
    if(bytes[10] == 0x11 && bytes[11] == 0x01){
        return Button;
    }
    else if(bytes[10] == 0x08 && bytes[11] == 0x00){
        return Rotary;
    }
    else{
        return -1;
    }
}

enum KnobValue DecodeValue (unsigned char bytes[32]){
    char valueByte = bytes[12];
    if(valueByte == 0x00){
        return Released;
    }
    else if(valueByte == 0x01){
        if(DecodeAction(bytes) == Rotary){
            return TurnedRight;
        }
        else if(DecodeAction(bytes) == Button){
            return Pressed;
        }
        else{
            return -1;
        }
    }
    else if(valueByte == 0xff){
        return TurnedLeft;
    }
    else{
        return -1;
    }
}

// The timestamp is stored in the first 4 bytes of the message (little endian)
unsigned int DecodeTimestamp(unsigned char bytes[32]) {
    unsigned int timestamp = *((unsigned int *)bytes);
    return timestamp;
}

char* ToHexString(unsigned char bytes[]){
    // ?? Not sure why I have to multiply by 4 here
    // Still too new to C to understand probably
    unsigned int amountOfBytes = sizeof(bytes) * 4;
    char* hexString = malloc((amountOfBytes * 3 + 1) * sizeof(char));
    if (hexString == NULL) {
        return NULL;
    }
    for (int i = 0; i < amountOfBytes; i++)
    {
        sprintf(&hexString[i*3], "%02X ", bytes[i]);
    }
    hexString[amountOfBytes*3] = '\0';
    return hexString;
}

void OpenFiles(int fds[12]) {
    for (int i = 1; i < N_STOMPS; i++) {
        char filename[32];
        sprintf(filename, "/dev/zencoder/knob_stomp%d", i);
        printf("\nTrying to open %s", filename);
        // First check if file exists
        int fd = open(filename, O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            printf("\t[!] File %s does not exist", filename);
        } else {
            printf("\t[+] File %s opened", filename);
            fds[i] = fd;
        }
    }
    printf("\n");
}

void CloseFiles(FILE *files[12]){
    for (int i = 1; i < N_STOMPS; i++)
    {
        if(files[i] == NULL){
            continue;
        }
        fclose(files[i]);
    }
}

struct StompMessage *ProcessFile(int *file){
    if(file == -1){
        return;
    }
    
    unsigned char buffer[32];
    struct StompMessage *stompMessage = malloc(sizeof(struct StompMessage));
    ssize_t bytesRead = read(file, buffer, sizeof(buffer));

    if (bytesRead == -1) {
        // Handle error (e.g., EAGAIN/EWOULDBLOCK means no data available)
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            free(stompMessage); // Free memory before returning
            return NULL;
        } else {
            perror("read error");
            // Handle other read errors here
        }
    } else if (bytesRead == 0) {
        // End of file reached
        close(*file);
        *file = -1;
        free(stompMessage); // Free memory before returning
        return NULL;
    } else {
        // char* hexString = ToHexString(buffer);
        // printf("%s\n", hexString);

        enum Action action = DecodeAction(buffer);
        enum KnobValue value = DecodeValue(buffer);
        unsigned int timestamp = DecodeTimestamp(buffer);

        stompMessage->action = action;
        stompMessage->value = value;
        stompMessage->timestamp = timestamp;
    }

    return stompMessage;
}

struct StompMessage **ReadStomps(int *files[N_STOMPS]) {
    struct StompMessage **messages = malloc(N_STOMPS * sizeof(struct StompMessage *));
    for (int i = 1; i < N_STOMPS; i++) {
        if (files[i] == -1) {
            free(messages[i]); // Free memory before setting to NULL
            messages[i] = NULL;
            continue;
        }

        messages[i] = ProcessFile(files[i]);
    }

    return messages;
}