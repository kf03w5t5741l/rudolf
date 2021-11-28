#pragma once

#include <stdint.h>

/**
  * @brief Retrieve puzzle input for given year and day
  * 
  * @param year
  * @param day
  * @return Pointer to char buffer containing puzzle input
  */ 
char* rudolf_get_input(int year, int day);

/**
 * @brief Split a string into an array of substrings with given delimiters
 * 
 * @param dest Where to store pointer to array of substrings
 * @param input Pointer to input string
 * @param delimiters Pointer to const string with delimiters
 * @param count Where to store array size
 * @returns Array of substrings
 */
int rudolf_split(
    char*** dest,
    char* input,
    const char* delimiters,
    size_t* count
);

typedef struct timed_t {
    int64_t value;
    double time;
} timed_t;

/**
 * @brief Measure execution time of a int64_t function with an input_t parameter
 * 
 * @param fn Pointer to function
 * @param fn Function parameter
 * @return double Execution time in seconds
 */
timed_t* rudolf_time_fn(int64_t (*fn)(char*), char* input);
