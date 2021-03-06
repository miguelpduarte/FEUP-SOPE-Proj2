#ifndef _CLIENT_H_
#define _CLIENT_H_

/**
  * Prints the correct program usage
  *
  * @param stream Stream in which the usage is printed on
  */
void print_usage(FILE* stream);

/**
  * Parses the time out value for the program to run
  *
  * @param time_out_str String representing time out value
  *
  * @return Returns the u_int parsed time out value, or exits with failure value on error (-4)
  */
u_int parse_time_out_val(char* time_out_str);

/**
  * Parses the clients number of wanted seats
  *
  * @param num_wanted_seats_str String representing clients number of wanted seats
  *
  * @return Returns the u_int parsed number of wanted seats, or exits with failure value
  *         on error (-4 if invalid parameter, -1 on number out of range)
  */
u_int parse_num_wanted_seats(char* num_wanted_seats_str);

/**
  * Parses the prefered seat client list
  *
  * @param pref_seat_list String with the prefered seat list, inputed by the user
  * @param parsed_pref_seat_list Pointer to the array containing the parsed prefered seat list
  * @param num_wanted_seats Number of wanted seats entered by the user
  *
  * @return Returns the size of the prefered seat list entered by the user, or exits with failure
  *         on error (-2 if number of prefered seats out of range, -3 on invalid seat identifier)
  */
u_int parse_pref_seat_list(char* pref_seat_list, u_int* parsed_pref_seat_list, u_int num_wanted_seats);

/**
 * Waits for a response from the server, for time_out second
 *
 * @param time_out Number of seconds for wait time
 * @param fifo_name Name of fifo to read the answer from
 */
void get_server_response(u_int time_out, char fifo_name[WIDTH_FIFO_NAME]);

#endif  // _CLIENT_H_
