#include "page.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int open_table(char* pathname)
{
	db = open(pathname, O_SYNC | O_CREAT | O_RDWR, 0777);
	if ( db < 0 )
	{
		return -1;
	}
	header = (header_page_t*)malloc(4096);
	memset(header, 0, 4096);
	file_read_page(0, (page_t*)header);

	if ( header->num == 0 )
	{
		header->free = 0;
		header->root = 0;
		header->num = 1;
		file_write_page(0, (page_t*)header);
	}
	return db;
}
pagenum_t file_alloc_page()
{
	file_read_page(0, (page_t*)header);

	page_t * page = (page_t*)malloc(sizeof(page_t));

	pagenum_t alloc_page = header->free;
	if ( !alloc_page )
	{
		alloc_page = header->num++;
		file_write_page(0, (page_t*)header);
		return alloc_page;
	}
	file_read_page(alloc_page, page);
	pagenum_t free_to_be = page->next_free;

	header->free = free_to_be;
	file_write_page(0, (page_t*)header);
	return alloc_page;
}
void file_free_page(pagenum_t pagenum)
{
	file_read_page(0, (page_t*)header);
	page_t * page = (page_t *)malloc(sizeof(page_t));
	file_read_page(pagenum, page);
	page->next_free = header->free;
	header->free = pagenum;
	file_write_page(0, (page_t*)header);
	file_write_page(pagenum, page);
}
void file_read_page(pagenum_t pagenum, page_t * dest)
{
	int flag = pread(db, dest, 4096, pagenum * 4096); // read from pagenum*4096 in db
}
void file_write_page(pagenum_t pagenum, const page_t* src)
{
	int flag = pwrite(db, src, 4096, pagenum * 4096);
}
