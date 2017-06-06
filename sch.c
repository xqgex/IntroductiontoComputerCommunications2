#include <errno.h>	/* ERANGE, errno */
#include <limits.h>	/* LONG_MAX, LONG_MIN */
#include <stdio.h>	/* FILE, printf, fprintf, sprintf, stderr, fopen, fclose */
#include <stdlib.h>	/* EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol */
#include <string.h>	/* strncmp, strerror_r */

#define USAGE_OPERANDS_MISSING_MSG	"Missing operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define USAGE_OPERANDS_SURPLUS_MSG	"Too many operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define ERROR_EXIT_MSG			"Exiting...\n"
#define F_ERROR_ENQUEUE_FAILED_MSG	"[Error] Program failed to enqueue packed ID: %ld\n"
#define F_ERROR_FUNCTION_SPRINTF_MSG	"[Error] sprintf() failed with an error\n"
#define F_ERROR_FUNCTION_STRTOL_MSG	"[Error] strtol() failed with an error: %s\n"
#define F_ERROR_INPUT_CLOSE_MSG		"[Error] Close input file: %s\n"
#define F_ERROR_INPUT_FILE_MSG		"[Error] Input file '%s': %s\n"
#define F_ERROR_IP_INVALID_MSG		"[Error] The ip address %s is not a valid IPv4\n"
#define F_ERROR_LENGTH_INVALID_MSG	"[Error] The length '%s' is not a number between 64 to 16384\n"
#define F_ERROR_OUTPUT_CLOSE_MSG	"[Error] Close output file: %s\n"
#define F_ERROR_OUTPUT_FILE_MSG		"[Error] Output file '%s': %s\n"
#define F_ERROR_QUANTUM_INVALID_MSG	"[Error] The quantum '%s' is not non negative integer\n"
#define F_ERROR_PKTID_INVALID_MSG	"[Error] The packet ID '%s' is not a valid long integer\n"
#define F_ERROR_PORT_INVALID_MSG	"[Error] The port '%s' is not a number between 0 to 65535\n"
#define F_ERROR_TIME_INVALID_MSG	"[Error] The time '%s' is not a valid long integer\n"
#define F_ERROR_TYPE_MSG		"[Error] The only valid types are RR and DRR\n"
#define F_ERROR_WEIGHT_INVALID_MSG	"[Error] The weight '%s' is not a positive integer\n"

FILE* IN_FILE = NULL;		/* The input file */
FILE* OUT_FILE = NULL;		/* The output file */
long CLOCK = LONG_MIN;		/* The current time */
typedef struct DataStructure {
	struct Packets* head;	/* Pointer to the head of the round double linked list */
	int count;		/* The total number of packets, Need to be updated in every insert & delete */
} *structure; 
typedef struct Packets {
	long pktID;		/* Unique ID (long int [-9223372036854775808,9223372036854775807]) */
	long Time;		/* Arrival time (long int [-9223372036854775808,9223372036854775807]) */
	char Sadd[15];		/* Source ip address (string Ex. '192.168.0.1') */
	int Sport;		/* Source port (int [0,65535]) */
	char Dadd[15];		/* Destination ip address (string Ex. '192.168.0.1') */
	int Dport;		/* Destination port (int [0,65535]) */
	int length;		/* Packet length (int [64,16384]) */
	int weight;		/* Flow weight (int [1,2147483647]) */
	int start_byte;		/* From were shold next transmition Start*/
	struct Packets* next;	/* Pointer to next packet in the list */
	struct Packets* prev;	/* Pointer to prev packet in the list */
	struct Packets* up;	/* Pointer to upper packet in the queue */
	struct Packets* down;	/* Pointer to lower packet in the queue */
} *packet; 

/* int program_end(int error) { }
 *
 * Receive exit code,
 * Close gracefully everything,
 * Return exit code,
 */
int program_end(int error) {
	char errmsg[256];
	int res = 0;
	if ((IN_FILE != NULL)&&(fclose(IN_FILE) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_INPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((OUT_FILE != NULL)&&(fclose(OUT_FILE) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_OUTPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((error != 0) || (res != 0)) {
		fprintf(stderr, ERROR_EXIT_MSG);
		if (error != 0) { /* If multiple error occurred, Print the error that called 'program_end' function. */
			res = error;
		}
	}
	return res;
}
/* validate_IPv4(const char *s) { }
 *
 * Receive string,
 * Return 0 if input is a valid IPv4 address,
 * return -1 otherwise
 */
int validate_IPv4(const char *s) { /* http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c#answer-14181669 */
	char tail[16];
	int c,i;
	int len = strlen(s);
	unsigned int d[4];
	if (len < 7 || 15 < len) {
		return -1;
	}
	tail[0] = 0;
	c = sscanf(s,"%3u.%3u.%3u.%3u%s",&d[0],&d[1],&d[2],&d[3],tail);
	if (c != 4 || tail[0]) {
		return -1;
	}
	for (i=0;i<4;i++) {
		if (d[i] > 255) {
			return -1;
		}
	}
	return 0;
}
/* int convert_strin2long(char *input, int *output) { }
 * 
 * Receive:	input string
 * 		pointer where to save the output int
 * 		limitation (included).on the int range
 * 		message to print in case of an error
 * Convert the string to int
 * Return 0 if succeed,
 * Return errno if error occurred.
 */
int convert_strin2long(char *input, long *output, long from, long to, char *error_string) { /* https://linux.die.net/man/3/strtol */
	/* Function variables */
	char errmsg[256];	/* The message to print in case of an error */
	char output_char[10];	/* variable for sprintf() */
	char *endptr;		/* variable for strtol() */
	/* Function body */
	*output = strtol(input, &endptr, 10); /* If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE. */
	if ((errno == ERANGE && (*output == LONG_MAX || *output == LONG_MIN)) || (errno != 0 && *output == 0)) {
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return errno;
	} else if ((endptr == input) || (*output < from) || (*output > to)) { /* (Empty string) or (Not in range) */
		fprintf(stderr, error_string, input);
		return EXIT_FAILURE;
	} else if (sprintf(output_char, "%ld", *output) < 0) { /* sprintf(), If an output error is encountered, a negative value is returned. */
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strncmp(output_char, input, 10) != 0) { /* Contain invalid chars */
		fprintf(stderr, error_string, input);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/* packet read_packet(int* total_num) { }
 * 
 * Receive nothing
 * Read one line from the input file and parse her
 * Return packet object if read succeed,
 * Return NULL in case EOF reached or if an error occurred.
 */
packet read_packet(int default_weight) {
	/* Function variables */
	char line[105];	/* 105 == pktID[20]+Time[20]+Sadd[15]+Sport[5]+Dadd[15]+Dport[5]+length[5]+weight[11]+spaces[7]+"\r\n"[2] */
	char *word;	/* Each string splited by whitespace */
	char *newline;
	int count;	/* Count how many words was in the current line */
	int res = 0;	/* Temporary variable to store function response */
	long temp = 0;	/* Temporary variable */
	/* Function body */
	packet pk = malloc(sizeof(struct Packets));
	if ((fgets(line, sizeof(line), IN_FILE))&&(strlen(line) > 26)) { /* return s on success, and NULL on error or when end of file occurs while no characters have been read. */
		count = 0; /* Init the counter */
		word = strtok(line," ");
		while (word != NULL) {
			/* Remove newline chars */
			newline = strchr(word, '\r');
			if (newline) {
				word[newline-word] = '\0';
			}
			newline = strchr(word, '\n');
			if (newline) {
				word[newline-word] = '\0';
			}
			/* Parse the current word */
			if ((count == 0)&&(strlen(word) <= 20)) {
				if ((res = convert_strin2long(word, &temp, LONG_MIN, LONG_MAX, F_ERROR_PKTID_INVALID_MSG)) > 0) {
					return NULL; /* Error occurred, Exit */
				} else {
					pk->pktID = temp;
				}
			} else if ((count == 1)&&(strlen(word) <= 20)) {
				if ((res = convert_strin2long(word, &temp, LONG_MIN, LONG_MAX, F_ERROR_TIME_INVALID_MSG)) > 0) {
					return NULL; /* Error occurred, Exit */
				} else {
					pk->Time = temp;
				}
			} else if ((count == 2)&&(strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return NULL;
				} else {
					strncpy(pk->Sadd, word, 15);
				}
			} else if ((count == 3)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return NULL; /* Error occurred, Exit */
				} else {
					pk->Sport = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 4)&&(strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return NULL;
				} else {
					strncpy(pk->Dadd, word, 15);
				}
			} else if ((count == 5)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return NULL; /* Error occurred, Exit */
				} else {
					pk->Dport = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 6)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 1, 16384, F_ERROR_LENGTH_INVALID_MSG)) > 0) { /* TODO TODO TODO Change from 1 to 64 TODO TODO TODO !!!!!!!!!!!!! */
					return NULL; /* Error occurred, Exit */
				} else {
					pk->length = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 7)&&(strlen(word) <= 11)) {
				if ((res = convert_strin2long(word, &temp, 1, INT_MAX, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
					return NULL; /* Error occurred, Exit */
				} else if (temp) { /* There was a value for weight in the input file */
					pk->weight = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				} else {
					pk->weight = default_weight; /* Use the default weight */
				}
			} else {
				return NULL; /* Invalid input, Length is too long */
			}
			count += 1; /* Inc the counter */
			word = strtok(NULL, " ");
		}
		if (count < 7) {
			return NULL; /* Invalid input, Length is too short */
		}
		printf("pktID='%ld', Time='%ld', Sadd='%s', Sport='%d', Dadd='%s', Dport='%d', length='%d', weight='%d'\n", pk->pktID, pk->Time, pk->Sadd, pk->Sport, pk->Dadd, pk->Dport, pk->length, pk->weight); /* TODO DEBUG XXX DELME XXX XXX */
		pk->start_byte = 0; /* From were should next transmition Start */
		pk->next = NULL; /* Pointer to next packet in the list */
		pk->prev = NULL; /* Pointer to prev packet in the list */
		pk->up = NULL; /* Pointer to upper packet in the queue */
		pk->down = NULL; /* Pointer to lower packet in the queue */
		return pk;
	} else {
		return NULL; /* EOF */
	}
} 
/* int send_packet(packet pk) { }
 * 
 * Receive ??? XXX ??? XXX
 * ??? XXX ??? XXX
 * Return ??? XXX ??? XXX
 */
int send_packet(packet pk) { /* TODO TODO TODO  Write to output file */
	pk = pk; /* TODO XXX DELME XXX TODO */
	return 0; /* TODO TODO TODO Return 0 on success & 1 on failure */
}
/* int same_flow(packet pacA, packet pacB) { }
 * 
 * Receive two packets
 * Check if they belong to the same flow
 * Return 1 if both packets have the same source and destination (IP&port)
 * Return 0 o.w
 */
int same_flow(packet pacA, packet pacB) {
	if ((pacA->Sport != pacB->Sport)||(pacA->Dport != pacB->Dport) ||(pacA->Sadd != pacB->Sadd) || (pacA->Dadd != pacB->Dadd)) {
		return 0;
	}
	return 1;
}
/* int enqueue(packet pk, packet* head_of_line) { }
 * 
 * Receive ??? XXX ??? XXX
 * ??? XXX ??? XXX
 * Return ??? XXX ??? XXX
 */
int enqueue(packet pk,packet* head_of_line) { /* TODO TODO TODO Add packet to our data structure */
	packet pac = malloc(sizeof(struct Packets));
	int flag = 0;
	printf("F_0\n"); /* TODO XXX DEBUG DELME XXX */
	while (flag <= 1) {
		printf("F_0.5\n"); /* TODO XXX DEBUG DELME XXX */
		if (&pac == head_of_line) { /* Complets a single round over the list. */
			flag += 1;
			printf("F_1\n"); /* TODO XXX DEBUG DELME XXX */
		}
		printf("F_1.5\n"); /* TODO XXX DEBUG DELME XXX */
		if (same_flow(pac,pk)) { /* Found a flow, that pk belongs to. */
			printf("F_2\n"); /* TODO XXX DEBUG DELME XXX */
			pk->prev = pac->prev;
			pk->next = pac->next;
			pk->prev->next = pk;
			pk->next->prev = pk;
			pk->down = pac;
			pac->up = pk;
			pk->next = NULL;
			pk->prev = NULL;
			pk->total_num_packets++;
			return 0;
		}
	}
	/* Havn't found a flow that pk belongs to, create a new one. */
	printf("F_3\n"); /* TODO XXX DEBUG DELME XXX */
	if(head_of_line == NULL){ /* TODO test for failure */
		printf("F_4\n"); /* TODO XXX DEBUG DELME XXX */
		head_of_line = &pk; /* pk is new head */
		pk->prev = pk; /* And his self prev and next */
		pk->next = pk;
	} else {
		printf("F_5\n"); /* TODO XXX DEBUG DELME XXX */
		pk->prev = (*head_of_line)->prev; /* Update pk prev */
		pk->next = *head_of_line; /* Update pk next */
		(*head_of_line)->prev = pk; /* Insert before head */
		pk->prev->next = pk; /* pk's prev, next */
	}
	printf("F_6\n"); /* TODO XXX DEBUG DELME XXX */
	pk->total_num_packets++; /* increse counter */
	return 0; /* TODO TODO TODO Return 0 on success & 1 on failure */
}

/* int dequeue(packet pk, packet* head_of_line) { }
 * 
 * Receive ??? XXX ??? XXX
 * ??? XXX ??? XXX
 * Return ??? XXX ??? XXX
 */
int dequeue(packet pk, packet* head_of_line) { /* TODO TODO TODO Remove packet from our data structure */
	if((pk->next!=NULL)&&(pk->prev!=NULL)){
	pk->next->prev = pk->prev;
	pk->prev->next = pk->next;
	}
	if (*head_of_line == pk) {
		*head_of_line = NULL;
	}
	if(pk->up != NULL ){
		pk->up->down = NULL;
	}
	free(pk);
	return 0; /* TODO TODO TODO Return 0 on success & 1 on failure */
}
/* void print(packet* head_of_line) { }
 * 
 * Print the packets in line
 * For debugging and support use only!!!
 */
void print(packet* head_of_line) {
	packet pkt = *head_of_line;
	printf("╔════════════════════════════════════════════════════════════════════════╗\n");
	printf("║ Current time=%20ld Number of waiting packets=%10i ║\n", CLOCK, *(pkt->total_num_packets));
	printf("╠═════════════════════════╤═══════════════════════════╤══════════════════╣\n");
	printf("║ ID=%20ld │ Time=%20ld │ Size=%5i/%5i ║\n", pkt->pktID, pkt->Time, pkt->start_byte, pkt->length);
	printf("╚═════════════════════════╧═══════════════════════════╧══════════════════╝\n");
	/*╔══════════════════════════════════╤═══════════════════════╤════════════════╗*/
	/*║               Col1               │         Col2          │      Col3      ║*/
	/*╠══════════════════════════════════╪═══════════════════════╪════════════════╣*/
	/*║                                  │                       │                ║*/
	/*║                                  │                       │                ║*/
	/*║                                  │                       │                ║*/
	/*╚══════════════════════════════════╧═══════════════════════╧════════════════╝*/
}
/* int main(int argc, char *argv[]) { }
 * 
 * Receive command line arguments
 * Simulate round robin algorithm
 * Return the program exit code
 */
int main(int argc, char *argv[]) {
	/* Function variables */
	char errmsg[256];		/* The message to print in case of an error */
	int input_quantum = 0;		/* The quantum */
	int input_weight = 0;		/* The weight */
	int res = 0;			/* Temporary variable to store function response */
	long temp = 0;			/* Temporary variable */
	int total_num_packets = 0;	/* TODO */
	packet* head_of_line = NULL;	/* TODO */
	packet last_packet = malloc(sizeof(struct Packets));
	/* Check correct call structure */
	if (argc != 6) {
		if (argc < 6) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0]);
		}
		else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0]);
		}
		return EXIT_FAILURE;
	}
	/* Check type => argv[1] */
	if ((strncmp(argv[1], "RR", 2) != 0)&&(strncmp(argv[1], "DRR", 3) != 0)) {
		fprintf(stderr, F_ERROR_TYPE_MSG);
		return program_end(EXIT_FAILURE);
	}
	/* Check input file => argv[2] */
	if ((IN_FILE = fopen(argv[2], "rb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_INPUT_FILE_MSG, argv[2], errmsg);
		return program_end(errno);
	}
	/* Check output file => argv[3] */
	if ((OUT_FILE = fopen(argv[3], "wb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_OUTPUT_FILE_MSG, argv[3], errmsg);
		return program_end(errno);
	}
	/* Check weight => argv[4] */
	if ((res = convert_strin2long(argv[4], &temp, 1, INT_MAX, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
		return program_end(res); /* Error occurred, Exit */
	} else {
		input_weight = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
	}
	/* Check quantum => argv[5] */
	if ((res = convert_strin2long(argv[5], &temp, 0, INT_MAX, F_ERROR_QUANTUM_INVALID_MSG)) > 0) {
		return program_end(res); /* Error occurred, Exit */
	} else {
		input_quantum = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
	}
	input_quantum = input_weight; /* TODO XXX DELME XXX TODO */
	/* Start the clock :) */
	/* Weighted Round Robin
	 * 1) If total weights is W, it takes W rounds to complete a cycle.
	 * 2) Each flow i transmits w[i] packets in a cycle.
	 * 
	 * Deficit Round Robin
	 * 1) Each flow has a credit counter.
	 * 2) Credit counter is increased by “quantum” with every cycle.
	 * 3) A packet is sent only if there is enough credit.
	 */
	while ((last_packet = read_packet(input_weight)) != NULL) {
		printf("F_-3\n"); /* TODO XXX DEBUG DELME XXX */
		if (CLOCK < last_packet->Time) { /* The new packet is from the future... */
			printf("F_-2\n"); /* TODO XXX DEBUG DELME XXX */
/*			if (send_packet(last_packet)) {*/
/*				fprintf(stderr, F_ERROR_XXXXXX_FAILED_MSG);*/
/*				return program_end(EXIT_FAILURE); / Error occurred in send_packet() /*/
/*			}*/
			CLOCK = last_packet->Time;
		} else { /* The packet is not from the future? How sad... */
			printf("F_-1\n"); /* TODO XXX DEBUG DELME XXX */
			if (enqueue(last_packet, head_of_line)) { /* Add the new packet to the data structure and keep reading for more packets with the same time */
				fprintf(stderr, F_ERROR_ENQUEUE_FAILED_MSG, last_packet->pktID);
				return program_end(EXIT_FAILURE); /* Error occurred in enqueue() */
			}
			print(head_of_line); /* TODO XXX DELME XXX TODO */
			return 0; /* TODO XXX DELME XXX TODO */
		}
	}
	/* Exit */
	return program_end(EXIT_SUCCESS);
}
