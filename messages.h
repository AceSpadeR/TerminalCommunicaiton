// file: messages.h
// definition of the message types for mmio communication
// and constants and type definitions
//
#define INT_ARRAY 0
#define FLT_ARRAY 1
#define CHR_ARRAY 3

#define MMIOCMD "/tmp/mmio_cmd1.bin"
#define MMIODAT "/tmp/mmio_dat1.bin"
#define MMIOFILE "/tmp/mmio.bin"
#define MMIOSIZE 4096
#define MSGSIZE 512
#define SEM_DATA_RDY "/sem-data-ready"  // semaphore data is ready to read
#define SEM_CMD_RDY "/sem-cmd-ready"	// semaphore command is ready to read

// Union is used to allow us to access information of different types
union Data {
	int nums[127];		// buffer of 512 bytes, but need  msgtype & msgsize
	char txtmsg[508];	// so each of the union members is 508 bytes or
	float floats[127];	// less file offset is 0 or 512 for write or read
};

// messages are 512 bytes with one write and one read message in the memory
typedef struct Msg { 	// define a type for all messaages
	short msgtype;   	// type code for what message structure is
	short msgsize;		// size in bytes of the message
	union Data message;	// location for message data numeric/text
} Msg;

typedef Msg Buf[2];

