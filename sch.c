#include <errno.h>	/* ERANGE, errno */
#include <limits.h>	/* LONG_MAX, LONG_MIN */
#include <stdio.h>	/* FILE, printf, fprintf, sprintf, stderr, fopen, fclose */
#include <stdlib.h>	/* EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol */
#include <string.h>	/* strncmp, strerror_r */

#define USAGE_OPERANDS_MISSING_MSG	"Missing operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define USAGE_OPERANDS_SURPLUS_MSG	"Too many operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define ERROR_EXIT_MSG			"Exiting...\n"
#define F_ERROR_FUNCTION_SPRINTF_MSG	"[Error] sprintf() failed with an error\n"
#define F_ERROR_FUNCTION_STRTOL_MSG	"[Error] strtol() failed with an error: %s\n"
#define F_ERROR_INPUT_CLOSE_MSG		"[Error] Close input file: %s\n"
#define F_ERROR_INPUT_FILE_MSG		"[Error] Input file '%s': %s\n"
#define F_ERROR_IP_INVALID_MSG		"[Error] The ip address %s is not a valis IPv4\n"
#define F_ERROR_LENGTH_INVALID_MSG	"[Error] The length must be a number between 64 to 16384\n"
#define F_ERROR_OUTPUT_CLOSE_MSG	"[Error] Close output file: %s\n"
#define F_ERROR_OUTPUT_FILE_MSG		"[Error] Output file '%s': %s\n"
#define F_ERROR_QUANTUM_INVALID_MSG	"[Error] The quantum must be non negative integer\n"
#define F_ERROR_PORT_INVALID_MSG	"[Error] The port must be a number between 0 to 65535\n"
#define F_ERROR_TYPE_MSG		"[Error] The only valid types are RR and DRR\n"
#define F_ERROR_WEIGHT_INVALID_MSG	"[Error] The weight must be a positive integer\n"

FILE* IN_FILE = NULL;			/* The input file */
FILE* OUT_FILE = NULL;			/* The output file */
typedef struct Packets {
	char pktID[20];		/* Unique ID (long int [-9223372036854775808,9223372036854775807]) */
	char Time[20];		/* Arrival time (long int [-9223372036854775808,9223372036854775807]) */
	char Sadd[15];		/* Source ip address (string Ex. '192.168.0.1') */
	int Sport;		/* Source port (int [0,65535]) */
	char Dadd[15];		/* Destination ip address (string Ex. '192.168.0.1') */
	int Dport;		/* Destination port (int [0,65535]) */
	int length;		/* Packet length (int [64,16384]) */
	int weight;		/* Flow weight (int [1,2147483647]) */
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
/* int convert_strin2int(char *input, int *output) { }
 * 
 * Receive:	input string
 * 		pointer where to save the output int
 * 		limitation (included).on the int range
 * 		message to print in case of an error
 * Convert the string to int
 * Return 0 if succeed,
 * Return errno if error occurred.
 */
int convert_strin2int(char *input, int *output, int from, int to, char *error_string) {
	/* Function variables */
	char errmsg[256];	/* The message to print in case of an error */
	char output_char[10];	/* variable for sprintf() */
	char *endptr;		/* variable for strtol() */
	/* Function body */
	*output = strtol(input, &endptr, 10); /* If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE. */
	if ((errno == ERANGE && (*output == (int)LONG_MAX || *output == (int)LONG_MIN)) || (errno != 0 && *output == 0)) {
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return errno;
	} else if ((endptr == input) || (*output < from) || (*output > to)) { /* (Empty string) or (Not in range) */
		fprintf(stderr, error_string);
		return EXIT_FAILURE;
	} else if (sprintf(output_char, "%d", *output) < 0) { /* sprintf(), If an output error is encountered, a negative value is returned. */
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strncmp(output_char, input, 10) != 0) { /* Contain invalid chars */
		fprintf(stderr, error_string);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/* int read_packet() { }
 * 
 * Receive nothing
 * Read one line from the input file and parse her
 * Return packet object if read succeed,
 * Return NULL in case EOF reached or if an error occurred.
 */
int read_packet() {
	/* Function variables */
	char line[105];	/* 105 == pktID[20]+Time[20]+Sadd[15]+Sport[5]+Dadd[15]+Dport[5]+length[5]+weight[11]+spaces[7]+"\r\n"[2] */
	char *word;	/* Each string splited by whitespace */
	char *newline;
	int count;	/* Count how many words was in the current line */
	int res = 0;	/* Temporary variable to store function response */
	/* Function body */
	packet pk = malloc(sizeof(packet));
	if (fgets(line, sizeof(line), IN_FILE)) { /* return s on success, and NULL on error or when end of file occurs while no characters have been read. */
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
				strncpy(pk->pktID, word, 20);
			} else if ((count == 1)&&(strlen(word) <= 20)) {
				strncpy(pk->Time, word, 20);
			} else if ((count == 2)&&(strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return 0;
				} else {
					strncpy(pk->Sadd, word, 15);
				}
			} else if ((count == 3)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2int(word, &pk->Sport, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				}
			} else if ((count == 4)&&(strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return 0;
				} else {
					strncpy(pk->Dadd, word, 15);
				}
			} else if ((count == 5)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2int(word, &pk->Dport, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				}
			} else if ((count == 6)&&(strlen(word) <= 5)) {
				if ((res = convert_strin2int(word, &pk->length, 64, 16384, F_ERROR_LENGTH_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				}
			} else if ((count == 7)&&(strlen(word) <= 11)) {
				if ((res = convert_strin2int(word, &pk->weight, 1, 2147483647, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				}
			} else {
				printf("Died at %d - %s\n", count, word); /* TODO DEBUG XXX XXX XXX */
				/* free(pk); // Error in `./sch': free(): invalid next size (fast) *//* TODO DEBUG XXX XXX XXX */
				return 0; /* Invalid input, Length is too long */
			}
			count += 1; /* Inc the counter */
			word = strtok(NULL, " ");
		}
		if (count < 7) {
			printf("Died with %d\n", count); /* TODO DEBUG XXX XXX XXX */
			return 0; /* Invalid input, Length is too short */
		}
		printf("pktID='%s', Time='%s', Sadd='%s', Sport='%d', Dadd='%s', Dport='%d', length='%d', weight='%s'\n", pk->pktID, pk->Time, pk->Sadd, pk->Sport, pk->Dadd, pk->Dport, pk->length, pk->weight); /* TODO DEBUG XXX XXX XXX */
		return 1;
	} else {
		return 0; /* EOF */
	}
} 

int send_packet() {
	/* TODO */
	return 1;
}

int main(int argc, char *argv[]) {
	/* Function variables */
	char errmsg[256];			/* The message to print in case of an error */
	int input_quantum = 0;			/* The quantum */
	int input_weight = 0;			/* The weight */
	int more_to_send = 1;			/* Flag for send loop */
	int more_to_read  = 1;			/* Flag for acsept loop */
	int res = 0;				/* Temporary variable to store function response */
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
	if ((res = convert_strin2int(argv[4], &input_weight, 1, 2147483647, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
		return program_end(res); /* Error occurred, Exit */
	}
	/* Check quantum => argv[5] */
	if ((res = convert_strin2int(argv[5], &input_quantum, 0, 2147483647, F_ERROR_QUANTUM_INVALID_MSG)) > 0) {
		return program_end(res); /* Error occurred, Exit */
	}
	/* TODO TODO TODO */
	while (more_to_send && more_to_read) {
		more_to_read = read_packet();
		more_to_send = send_packet();
	}
	/* TODO TODO TODO */
	return program_end(EXIT_SUCCESS);
}
