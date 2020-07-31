#include <stdint.h>
#ifndef __PAGE_H__
#define __PAGE_H__
typedef uint64_t pagenum_t;

typedef struct
{
	int64_t key;
	char value[120];
}record_t;

typedef struct
{
	int64_t key;
	pagenum_t child;
}branch_t;

typedef struct header
{
	pagenum_t free;
	pagenum_t root;
	pagenum_t num;
	char reserved[4072];
} header_page_t;

typedef struct page_t
{
	union
	{
		pagenum_t next_free;
		pagenum_t parent;
	};
	int is_leaf;
	int num_keys;
	int reserved[26];
	union
	{
		pagenum_t leftmost_child;
		pagenum_t right_sibling;
	};
	union
	{
		branch_t branches[248];
		record_t records[31];
	};
} page_t;

int db;
header_page_t* header;

int open_table(char* pathname);
pagenum_t file_alloc_page();
void file_free_page(pagenum_t pagenum);
void file_read_page(pagenum_t pagenum, page_t * dest);
void file_write_page(pagenum_t pagenum, const page_t* src);
#endif
