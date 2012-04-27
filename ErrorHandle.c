/**
 * Useful error handling function.
 *
 * Author: Kenneth Chik 2012.
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * Print user defined error.
 */
void userErrorMsg(const char *msg) {
    printf("%s\n",msg);
}

/**
 * Print user defined error and exit program.
 */
void userErrorExit(const char *msg) {
    fprintf(stderr,"%s\n",msg);
    exit(1);
}

/**
 * Print system error.
 */
void systemErrorMsg(const char *msg) {
    perror(msg);
}

/**
 * Print system error and exit program.
 */
void systemErrorExit(const char *msg) {
    perror(msg);
    exit(1);
}
