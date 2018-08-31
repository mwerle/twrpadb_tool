#ifndef SIMPLE_CRC32_H
#define SIMPLE_CRC32_H

/* Simple public domain implementation of the standard CRC32 checksum.
 * Outputs the checksum for each file given as a command line argument.
 * Invalid file names and files that cause errors are silently skipped.
 * The program reads from stdin if it is called with no arguments. */

/*
Copied from http://home.thep.lu.se/~bjorn/crc/
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Calculates the Cyclic Redundancy Check (CRC32) over some data.
 *
 * Usage:
 * uint32_t crc = 0;
 * for( read some data ) {
 *  crc32(myData, myDataSize, &crc);
 * }
 * // crc32 in 'crc'
 * ...
 */
void crc32(const void *data, size_t n_bytes, uint32_t* crc);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_CRC32_H */