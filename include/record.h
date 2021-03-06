#ifndef RECORD_H
#define RECORD_H

#include <unistd.h>
#include "mmap.h"

/** Record stream information. */
typedef int (*record_func_t)(void *, void *);
typedef int (*apply_func_t)(void *, int, void *);
/** Comparator function pointer.
 * The comparator routine is expected to return an integer less than, equal
 * to, or greater than zero if the first object is less than, equal to, or 
 * greater than the second object.
 */
typedef int (*comparator_t)(const void *, const void *);

long get_num_records(const char *file, int size);
int append_record(const char *file, const void *record, int size);
int get_record(char *filename, void *rptr, int size, int id);
int get_records(const char *filename, void *rptr, int size, int id,
		int number);
int apply_record(const char *file, apply_func_t func, int size,
			void *arg, bool copy, bool reverse, bool lock);
int search_record(const char *file, void *rptr, int size, record_func_t func,
		void *arg);
int substitute_record(char *filename, void *rptr, int size, int id);
int delete_record(const char *file, int size, int id,
		record_func_t check, void *arg);
int delete_range(char *filename, int id1, int id2);
int insert_record(const char *file, int size, record_func_t check, void *arg);

#endif // RECORD_H
