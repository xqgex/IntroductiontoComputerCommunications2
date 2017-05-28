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
#define F_ERROR_OUTPUT_CLOSE_MSG	"[Error] Close output file: %s\n"
#define F_ERROR_OUTPUT_FILE_MSG		"[Error] Output file '%s': %s\n"
#define F_ERROR_QUANTUM_INVALID_MSG	"[Error] The quantum must be non negative integer\n"
#define F_ERROR_TYPE_MSG		"[Error] The only valid types are RR and DRR\n"
#define F_ERROR_WEIGHT_INVALID_MSG	"[Error] The weight must be a positive integer\n"

int program_end(int error, FILE* in_fd, FILE* out_fd) {
	char errmsg[256];
	int res = 0;
	if ((in_fd != NULL)&&(fclose(in_fd) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_INPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((out_fd != NULL)&&(fclose(out_fd) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
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
int main(int argc, char *argv[]) {
	/* Function variables */
	char errmsg[256];			/* The message to print in case of an error */
	char input_quantum_char[10];		/* The quantum (type == string) */
	char input_weight_char[10];		/* The weight (type == string) */
	char* endptr_QUANTUM;			/* strtol() for 'input_quantum' */
	char* endptr_WEIGHT;			/* strtol() for 'input_weight' */
	FILE* in_fd = NULL;			/* The input file */
	FILE* out_fd = NULL;			/* The output file */
	int input_quantum = 0;			/* The quantum (type == int) */
	int input_weight = 0;			/* The weight (type == int) */
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
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	/* Check input file => argv[2] */
	if ((in_fd = fopen(argv[2], "rb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_INPUT_FILE_MSG, argv[2], errmsg);
		return program_end(errno, in_fd, out_fd);
	}
	/* Check output file => argv[3] */
	if ((out_fd = fopen(argv[3], "wb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_OUTPUT_FILE_MSG, argv[3], errmsg);
		return program_end(errno, in_fd, out_fd);
	}
	/* Check weight => argv[4] */
	input_weight = strtol(argv[4], &endptr_WEIGHT, 10); /* If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE. */
	if ((errno == ERANGE && (input_weight == (int)LONG_MAX || input_weight == (int)LONG_MIN)) || (errno != 0 && input_weight == 0)) {
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return program_end(errno, in_fd, out_fd);
	}
	else if ((endptr_WEIGHT == argv[4]) || (input_weight < 1)) { /* (Empty string) or (Not positive) */
		fprintf(stderr, F_ERROR_WEIGHT_INVALID_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	else if (sprintf(input_weight_char, "%d", input_weight) < 0) { /* sprintf(), If an output error is encountered, a negative value is returned. */
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	else if (strncmp(input_weight_char, argv[4], 10) != 0) { /* Contain invalid chars */
		fprintf(stderr, F_ERROR_WEIGHT_INVALID_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	/* Check quantum => argv[5] */
	input_quantum = strtol(argv[5], &endptr_QUANTUM, 10); /* If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE. */
	if ((errno == ERANGE && (input_quantum == (int)LONG_MAX || input_quantum == (int)LONG_MIN)) || (errno != 0 && input_quantum == 0)) {
		strerror_r(errno, errmsg, 255);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return program_end(errno, in_fd, out_fd);
	}
	else if ((endptr_QUANTUM == argv[5]) || (input_quantum < 0)) { /* (Empty string) or (Negative) */
		fprintf(stderr, F_ERROR_QUANTUM_INVALID_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	else if (sprintf(input_quantum_char, "%d", input_quantum) < 0) { /* sprintf(), If an output error is encountered, a negative value is returned. */
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	else if (strncmp(input_quantum_char, argv[5], 10) != 0) { /* Contain invalid chars */
		fprintf(stderr, F_ERROR_QUANTUM_INVALID_MSG);
		return program_end(EXIT_FAILURE, in_fd, out_fd);
	}
	/* TODO TODO TODO */
	// test push 
	/* TODO TODO TODO */
	return program_end(EXIT_SUCCESS, in_fd, out_fd);
}
