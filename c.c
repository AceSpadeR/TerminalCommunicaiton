// filename:  c.c Austin Spader
/* This program acts as device (consumer), responding to commands from 
 * the driver (producer) which interfaces with the user. 
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		
#include <sys/stat.h>	
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include "messages.h"
#include <stdbool.h>


// Global variables that must be outside the main program
sem_t *data_ready, *cmd_ready;  // these are the semaphores for control
	

// function prototypes for string management and the user menu
void clrnonprt(char a[508]);
int dispmenu(void);					// display and return choice


// Main program displays a menu for user operation.
int main() {
    int cmd_rdy_val, 
		data_rdy_val;		// place to save the semaphore values
	int menuchoice = 0;
    char *line = NULL;		// NULL allocates memory in getline
    size_t len=0;			// getline length, and receive buffer length
	int fd; 				// file descriptor for the data mmio file
	struct stat file_info;  // used to check the size of the file
	off_t file_size;		// used to ensure the file is large enough
	Buf* buffer;			// pointer to shared MMIO
	int count;				// size of array
    float average;			// AVG
		
	// first we need to set up the semaphores, the two processes share 
	// the same semaphores because of the name.  Both initialized to zero.
	// sem_open is used to create a semaphore or link to an existing one
	if ((data_ready = sem_open(SEM_DATA_RDY, O_CREAT, 0666, 0)) == SEM_FAILED) {
		perror("Error opening data_ready semaphore\n");
		exit(1);
	}
	if ((cmd_ready = sem_open(SEM_CMD_RDY, O_CREAT, 0666, 0)) == SEM_FAILED) {
		perror("Error opening cmd_ready semaphore\n");
		exit(1);
	}

	// Now set up the two files for mmio, first the data one to write to
    if ((fd = open(MMIOFILE, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0666)) == -1) {
        perror("File openning failed.");
        return EXIT_FAILURE;
    }
    
    // gather file status information to check the size of the file
    if (fstat(fd, &file_info) == -1) {
        close(fd);
        perror("fstat failed.");
        return EXIT_FAILURE;
    }    
	// get the size of the file
	file_size = file_info.st_size;

    // Set the file size using ftruncate, it should be the size allocated
    // space. Writing to a file region via MMIO which does not actually
    // exist will generate a sigbus fault. (one message size would be okay).
    if (file_size < MMIOSIZE) // file will hold one data message
		if (ftruncate(fd, MMIOSIZE) == -1) {
			close(fd);
			perror("ftruncate failed.");
			return EXIT_FAILURE;
		};

    // Map the file into memory
    buffer = (Buf*)mmap(NULL, MSGSIZE*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        close(fd);
        perror("mmap");
        return -1;
    }

    do {  
		
		menuchoice = dispmenu();
		switch (menuchoice) {
			case 1:
				if (sem_getvalue(data_ready, &data_rdy_val) == 0) {
					if (data_rdy_val == 0) { // can send new command
						// send a text message 
						printf("Enter msg to send:\n\t");
						getline(&line, &len, stdin);	// read a line 
						strncpy(buffer[1]->message.txtmsg, line, len); 
						clrnonprt(buffer[1]->message.txtmsg); // clear \n 
						buffer[1]->msgtype = CHR_ARRAY; // set message type
						buffer[1]->msgsize = strlen(buffer[1]->message.txtmsg);
						free(line);  // free allocation from "getline"
						line = NULL;
						sem_post(data_ready); // mark command as sent 
					} else {
						printf("\tUnread data in buffer, can't write \n\n");
					}
				} else {
					printf("\nUnable to get semaphore value, cmd_ready\n\\n");
				}
				break;
			case 2: 
				if (sem_getvalue(data_ready, &data_rdy_val) == 0) {
					if (data_rdy_val == 0) { // can send new command
							// Get array size
							count = 0;
							printf("Enter the number of integers to send (1 - 127): ");
							getline(&line, &len, stdin);
							sscanf(line, "%d", &count);
							while (count < 1 || count > 127) {
								printf("Too large... Enter number (1-127): ");
								getline(&line, &len, stdin);
								sscanf(line, "%d", &count);
							}

							// Enter the numbers
							printf("Enter %d integers:\n", count);
							for (int i = 0; i < count; i++) {
								getline(&line, &len, stdin);
								sscanf(line, "%d", &buffer[1]->message.nums[i]);
							}

							buffer[1]->msgtype = INT_ARRAY;
							buffer[1]->msgsize = count;
							free(line);  // Free the allocated
							line = NULL;
							sem_post(data_ready); // mark command as sent
					} else {
						printf("\tUnread data in buffer, can't write \n\n");
					}
				} else {
					printf("\nUnable to get semaphore value, cmd_ready\n\\n");
				} 
				break;
			case 3: 
				if (sem_getvalue(data_ready, &data_rdy_val) == 0) {
					if (data_rdy_val == 0) { // can send new command
						// Get array size
						count = 0;
						printf("Enter the number of floats to send (1 - 127): ");
						getline(&line, &len, stdin);
						sscanf(line, "%d", &count);
						while (count < 1 || count > 127) {
							printf("Too large... Enter number (1-127): ");
							getline(&line, &len, stdin);
							sscanf(line, "%d", &count);
						}

						// Enter the numbers
						printf("Enter %d floats:\n", count);
						for (int i = 0; i < count; i++) {
							getline(&line, &len, stdin);
							sscanf(line, "%f", &buffer[1]->message.floats[i]);
						}
						buffer[1]->msgtype = FLT_ARRAY;
						buffer[1]->msgsize = count;
						free(line);  // Free the allocated
						line = NULL;;
						sem_post(data_ready); // mark command as sent 
					} else {
						printf("\tUnread data in buffer, can't write \n\n");
					}
				} else {
					printf("\nUnable to get semaphore value, cmd_ready\n\\n");
				} 
				break;
			case 4: 
				//printf("case 4\n");
				if (sem_getvalue(cmd_ready, &cmd_rdy_val) == 0) {
					//int sem_value;
					//sem_getvalue(cmd_ready, &sem_value);
					//printf("cmd_ready semaphore value: %d\n", sem_value);
					if (cmd_rdy_val > 0) { // there is data to read
						switch (buffer[0]->msgtype) {
							case CHR_ARRAY:  // Print out the text
								printf("The message received is:\n%s\n", 
								buffer[0]->message.txtmsg);
								sem_wait(cmd_ready);   // mark message read
								break;
							case INT_ARRAY: // average the array values
                                count = buffer[0]->msgsize;
							    int sumi = 0;
							    for(int i = 0; i < count; i++) {
	                                    sumi+= buffer[0]->message.nums[i]; // nums
                                    }
                                    
                                    average = (float)sumi / count;
                                    // Display Average
                                    printf("Received integer array. Average: %.2f\n", average);
                                sem_wait(cmd_ready);  // mark message read 
								break;
							case FLT_ARRAY:	// average the array values
                                count = buffer[0]->msgsize;
								float sumf = 0;
                                // Gets the sum of each
                                for(int i = 0; i < count; i++) {
                                    sumf+= buffer[0]->message.floats[i]; // floats
                                }
                                average = sumf / count;
                                // Display Average
                                printf("Received float array. Average: %.2f\n", average);
                                sem_wait(cmd_ready);  // mark message read
								break;
						}
					} else {
						printf("\tNo data to read from device\n");
					}
	 			}
			case 5: // exit the program, nothing to do
				break;
			default:
				printf("Errorneous value returned from menu\n");
			}
	} while (menuchoice != 5);
	
	printf("Exiting program now\n");
    // Unmap the memory
    if (munmap(buffer, MSGSIZE) == -1) {
        perror("munmap");
    }
	
	// clean up file and semaphores
	close(fd);
	sem_close(data_ready);
	sem_close(cmd_ready);
	sem_unlink(SEM_CMD_RDY);
	sem_unlink(SEM_DATA_RDY);

    return 0;
} // end of main program

// clear the ending non-printable of a string to NULL
void clrnonprt(char a[508]) {
	int i;
	bool nochange = true;
	
	for (i=strlen(a); i >= 0 && nochange; i--) 
		if (nochange = a[i] < 32) a[i] = 0; // change last non-printable to NULL
}


// This displays the menu and returns the menu choice
// NOTE: This is NOT robust.. I was in a hurry
int dispmenu(void) {
	int choice = 0;
    char *line = NULL;		// NULL allocates memory
    size_t len=0;	// getline length, and receive buffer length
	
	while(choice < 1 || choice > 5) {
		printf("\n\nPlease choose from the following options.\n\n"
			   "\t1 - Send a text messaage\n"
			   "\t2 - Send an array of integers\n"
			   "\t3 - Send an array of floats\n"
			   "\t4 - Read data from other process\n"
			   "\t5 - Exit the program\n\n"
			   "\t\tChoice? ");
		getline(&line, &len, stdin);
		sscanf(line, "%d", &choice);
	}
	free(line);
	return choice;
}
