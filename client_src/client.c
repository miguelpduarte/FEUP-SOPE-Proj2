#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "defs.h"
#include "client.h"
#include "request_message.h"
#include "alarm.h"
#include "clog.h"
#include "parser.h"

void fifo_unlink() {
    char pid[WIDTH_PID+1];
    char fifo_name[WIDTH_FIFO_NAME] = "ans";
    snprintf(pid, WIDTH_PID+1, "%0" MACRO_STRINGIFY(WIDTH_PID) "d", getpid());
    strncat(fifo_name, pid, WIDTH_PID);
    unlink(fifo_name);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(stderr);
        exit(ERR);
    }

    // Parse parameters
    u_int time_out = parse_time_out_val(argv[1]);
    u_int num_wanted_seats = parse_num_wanted_seats(argv[2]);
    u_int* parsed_pref_seat_list = (u_int*) malloc(MAX_ROOM_SEATS);
    u_int num_pref_seats = parse_pref_seat_list(argv[3], parsed_pref_seat_list, num_wanted_seats);

    // Parse pid to wanted length and create FIFO
    char pid[WIDTH_PID+1];
    char fifo_name[WIDTH_FIFO_NAME] = "ans";
    snprintf(pid, WIDTH_PID+1, "%0" MACRO_STRINGIFY(WIDTH_PID) "d", getpid());
    strncat(fifo_name, pid, WIDTH_PID);

    if (mkfifo(fifo_name, 0660) != 0) {
        fprintf(stderr, "Error: failed to create fifo.\n");
        exit(FIFO_CREATION_ERROR);
    }

    atexit(fifo_unlink);

    // Create request message
    RequestMessage msg = create_request_message(getpid(), num_wanted_seats, parsed_pref_seat_list, num_pref_seats);

    // Broadcast message to server
    broadcast_message(msg);

    free(parsed_pref_seat_list);

    // Wait for the server message
    get_server_response(time_out, fifo_name);

    unlink(fifo_name);

    return 0;
}

void get_server_response(u_int time_out, char fifo_name[WIDTH_FIFO_NAME]) {
    // Prepare the alarm
    setup_alarm();
    alarm(time_out);

    // Open fifo for reading
    FILE* answer_fifo = fopen(fifo_name, "r");
    if (answer_fifo == NULL) {
        fprintf(stderr, "Error: Failed to open client fifo for reading server response.\n");
        exit(CLIENT_FIFO_OPENING_ERROR);
    }

    // Perform reading
    char* server_answer;
    size_t len = 0;
    ssize_t read_ret = getline(&server_answer, &len, answer_fifo);
    if (read_ret < 0) {
        fprintf(stderr, "Error: Failed to read server answer.\n");
        exit(READ_SERVER_ANS_ERROR);
    }

    // Message received, cancel the alarm
    alarm(0);

    // Parse the server message
    char** parsed_message_str;
    size_t message_len;
    if (split_string(server_answer, " ", &parsed_message_str, &message_len) != 0) {
        fprintf(stderr, "Error: Failed to parse server answer.\n");
        exit(PARSE_SERVER_ANS_ERROR);
    }

    if (parse_int(parsed_message_str[0]) < 0) {
        // Server error status
        int parsed_message[] = {parse_int(parsed_message_str[0])};
        writeinLog(parsed_message);
    }
    else {
        // Server success message
        int* parsed_message = (int*) malloc((message_len+1)*sizeof(int));
        parsed_message[0] = message_len;

        int i;
        for (i=0 ; i<message_len ; i++) {
            parsed_message[i+1] = parse_int(parsed_message_str[i]);
        }

        writeinLog(parsed_message);
        free (parsed_message);
    }

    // Free allocated memory and close fifo
    free(server_answer);
    int i;
    for (i=0 ; i<message_len ; i++) {
        free(parsed_message_str[i]);
    }
    free(parsed_message_str);
    fclose(answer_fifo);
}

u_int parse_time_out_val(char* time_out_str) {
    u_int time_out = parse_unsigned_int(time_out_str);
    if (time_out == UINT_MAX || time_out == 0) {
        fprintf(stderr, "Error: The timeout value must be a positive integer value.\n");
        print_usage(stderr);
        exit(ERR);  // Parameter error
    } else {
        return time_out;
    }
}

u_int parse_num_wanted_seats(char* num_wanted_seats_str) {
    u_int num_wanted_seats = parse_unsigned_int(num_wanted_seats_str);

    if (num_wanted_seats == UINT_MAX) {
        fprintf(stderr, "Error: The number of wanted seats must be a positive integer value.\n");
        print_usage(stderr);
        exit(ERR);  // Parameter error
    } else {
        return num_wanted_seats;
    }
}

u_int parse_pref_seat_list(char* pref_seat_list, u_int* parsed_pref_seat_list, u_int num_wanted_seats) {
    char* seat;
    char* rest = strdup(pref_seat_list);
    u_int num_pref_seats = 0;

    // Count the number of prefered seats entered by the user
    while( (seat = strtok_r(rest, " ", &rest)) ) {
        num_pref_seats++;
    }

    // Parse the prefered seats list
    parsed_pref_seat_list = (u_int *) realloc(parsed_pref_seat_list, num_pref_seats*sizeof(u_int));
    u_int parsed_seats_counter = 0;
    rest = strdup(pref_seat_list);

    while( (seat = strtok_r(rest, " ", &rest)) ) {
        parsed_pref_seat_list[parsed_seats_counter] = parse_unsigned_int(seat);

        // Verify if value is a number
        if (parsed_pref_seat_list[parsed_seats_counter] == UINT_MAX) {
            fprintf(stderr, "Error: The seat identifiers must be positive integer values.\n");
            print_usage(stderr);
            exit(ERR);  // Invalid number of prefered seats identifiers
        }

        parsed_seats_counter++;
    }

    return num_pref_seats;
}

void print_usage(FILE* stream) {
    fprintf(stream, "usage: client <time_out> <num_wanted_seats> <pref_seat_list>\n");
}
