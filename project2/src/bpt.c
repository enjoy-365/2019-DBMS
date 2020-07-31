/*
*  bpt.c
*/
#define Version "1.14"
/*
*
*  bpt:  B+ Tree Implementation
*  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
*  All rights reserved.
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*  this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright notice,
*  this list of conditions and the following disclaimer in the documentation
*  and/or other materials provided with the distribution.

*  3. Neither the name of the copyright holder nor the names of its
*  contributors may be used to endorse or promote products derived from this
*  software without specific prior written permission.

*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.

*  Author:  Amittai Aviram
*    http://www.amittai.com
*    amittai.aviram@gmail.edu or afa13@columbia.edu
*  Original Date:  26 June 2010
*  Last modified: 17 June 2016
*
*  This implementation demonstrates the B+ tree data structure
*  for educational purposes, includin insertion, deletion, search, and display
*  of the search path, the leaves, or the whole tree.
*
*  Must be compiled with a C99-compliant C compiler such as the latest GCC.
*
*  Usage:  bpt [order]
*  where order is an optional argument
*  (integer MIN_ORDER <= order <= MAX_ORDER)
*  defined as the maximal number of pointers in any node.
*
*/

#include "bpt.h"
#include "page.h"
#include <string.h>
#include <inttypes.h>
// GLOBALS.

/* The order determines the maximum and minimum
* number of entries (keys and pointers) in any
* node.  Every node has at most order - 1 keys and
* at least (roughly speaking) half that number.
* Every leaf has as many pointers to data as keys,
* and every internal node has one more pointer
* to a subtree than the number of keys.
* This global variable is initialized to the
* default value.
*/
int order = LEAF_ORDER;

/* The queue is used to print the tree in
* level order, starting from the root
* printing each entire rank on a separate
* line, finishing with the leaves.
*/
//node * queue = NULL;

/* The user can toggle on and off the "verbose"
* property, which causes the pointer addresses
* to be printed out in hexadecimal notation
* next to their corresponding keys.
*/
bool verbose_output = false;


// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/* Copyright and license notice to user at startup.
*/
void license_notice(void)
{
	printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
		   "http://www.amittai.com\n", Version);
	printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
		   "type `show w'.\n"
		   "This is free software, and you are welcome to redistribute it\n"
		   "under certain conditions; type `show c' for details.\n\n");
}


/* Routine to print portion of GPL license to stdout.
*/
void print_license(int license_part)
{
	int start, end, line;
	FILE * fp;
	char buffer[0x100];

	switch ( license_part )
	{
	case LICENSE_WARRANTEE:
		start = LICENSE_WARRANTEE_START;
		end = LICENSE_WARRANTEE_END;
		break;
	case LICENSE_CONDITIONS:
		start = LICENSE_CONDITIONS_START;
		end = LICENSE_CONDITIONS_END;
		break;
	default:
		return;
	}

	fp = fopen(LICENSE_FILE, "r");
	if ( fp == NULL )
	{
		perror("print_license: fopen");
		exit(EXIT_FAILURE);
	}
	for ( line = 0; line < start; line++ )
		fgets(buffer, sizeof(buffer), fp);
	for ( ; line < end; line++ )
	{
		fgets(buffer, sizeof(buffer), fp);
		printf("%s", buffer);
	}
	fclose(fp);
}


/* First message to the user.
*/
void usage_1(void)
{
	printf("B+ Tree of Order %d.\n", order);
	printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
		   "5th ed.\n\n"
		   "To build a B+ tree of a different order, start again and enter "
		   "the order\n"
		   "as an integer argument:  bpt <order>  ");
	printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
	printf("To start with input from a file of newline-delimited integers, \n"
		   "start again and enter the order followed by the filename:\n"
		   "bpt <order> <inputfile> .\n");
}


/* Second message to the user.
*/
void usage_2(void)
{
	printf("Enter any of the following commands after the prompt > :\n"
		   "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
		   "\tf <k>  -- Find the value under key <k>.\n"
		   "\tp <k> -- Print the path from the root to key k and its associated "
		   "value.\n"
		   "\tr <k1> <k2> -- Print the keys and values found in the range "
		   "[<k1>, <k2>\n"
		   "\td <k>  -- Delete key <k> and its associated value.\n"
		   "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
		   "same order.\n"
		   "\tt -- Print the B+ tree.\n"
		   "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
		   "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
		   "leaves.\n"
		   "\tq -- Quit. (Or use Ctl-D.)\n"
		   "\t? -- Print this help message.\n");
}


/* Brief usage note.
*/
void usage_3(void)
{
	printf("Usage: ./bpt [<order>]\n");
	printf("\twhere %d <= order <= %d .\n", MIN_ORDER, MAX_ORDER);
}


/* Helper function for printing the
* tree out.  See print_tree.
*/

Queue makequeue()
{
	Queue new_queue = (Queue)malloc(sizeof(struct Node));
	new_queue->next = NULL;
	return new_queue;
}

void enqueue(pagenum_t new_node, Queue queue)
{
	Queue c;
	c = queue;
	while ( c->next != NULL )
	{
		c = c->next;
	}
	c->next = (Queue)malloc(sizeof(struct Node));
	c->next->next = NULL;
	c->next->num = new_node;
}


/* Helper function for printing the
* tree out.  See print_tree.
*/
pagenum_t dequeue(Queue queue)
{
	Queue q = queue->next;
	pagenum_t res;
	res = q->num;
	queue->next = queue->next->next;
	free(q);
	return res;
}


/* Prints the bottom row of keys
* of the tree (with their respective
* pointers, if the verbose_output flag is set.
*/
void print_leaves(void)
{
	int i;
	file_read_page(0, (page_t*)header);
	pagenum_t start = header->root;

	if ( !start )
	{
		printf("Empty Tree.\n");
		return;
	}
	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(start, page);
	pagenum_t now = start;
	while ( page->is_leaf != 1 )
	{
		now = page->leftmost_child;
		file_read_page(page->leftmost_child, page);
	}

	while ( now )
	{
		file_read_page(now, page);
		for ( i = 0; i < page->num_keys; i++ )
		{
			printf("%"PRId64" ", page->records[i].key);
		}
		printf(" | ");
		now = page->right_sibling;
	}
	printf("\n");
}


/* Utility function to give the height
* of the tree, which length in number of edges
* of the path from the root to any leaf.
*/
/*int height( node * root ) {
int h = 0;
node * c = root;
while (!c->is_leaf) {
c = c->pointers[0];
h++;
}
return h;
}
*/

/* Utility function to give the length in edges
* of the path from any node to the root.
*/
int path_to_root(pagenum_t target)
{
	int length = 0;

	file_read_page(0, (page_t*)header);
	pagenum_t root_page = header->root;

	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(target, page);

	pagenum_t parent_page = page->parent;
	if ( !parent_page )
	{
		return 0;
	}
	while ( parent_page != root_page )
	{
		file_read_page(parent_page, page);
		parent_page = page->parent;
		length++;
	}

	return length + 1;
}


/* Prints the B+ tree in the command
* line in level (rank) order, with the
* keys in each node and the '|' symbol
* to separate nodes.
* With the verbose_output flag set.
* the values of the pointers corresponding
* to the keys also appear next to their respective
* keys, in hexadecimal notation.
*/
void print_tree(void)
{

	int i = 0;
	int rank = 0;
	int new_rank = 0;


	Queue queue = NULL;

	file_read_page(0, (page_t*)header);

	if ( header->root == 0 )
	{
		printf("Empty tree.\n");
		return;
	}

	queue = makequeue();

	enqueue(header->root, queue);
	page_t* page = (page_t*)malloc(sizeof(page_t));

	while ( queue->next != NULL )
	{
		pagenum_t now = dequeue(queue);

		file_read_page(now, page);

		new_rank = path_to_root(now);
		if ( new_rank != rank )
		{
			rank = new_rank;
			printf("\n");
		}
		if ( !page->is_leaf )
		{
			enqueue(page->leftmost_child, queue);
		}

		for ( int i = 0; i < page->num_keys; i++ )
		{
			if ( page->is_leaf )
			{
				printf("%"PRId64" ", page->records[i].key);
			}
			else
			{
				printf("%"PRId64" ", page->branches[i].key);
				enqueue(page->branches[i].child, queue);
			}
		}
		printf(" | ");
	}
	printf("\n");

	/*if (verbose_output)
	printf("(%lx)", (unsigned long)n);
	for (i = 0; i < n->num_keys; i++) {
	if (verbose_output)
	printf("%lx ", (unsigned long)n->pointers[i]);
	printf("%d ", n->keys[i]);
	}
	if (!n->is_leaf)
	for (i = 0; i <= n->num_keys; i++)
	enqueue(n->pointers[i]);
	if (verbose_output) {
	if (n->is_leaf)
	printf("%lx ", (unsigned long)n->pointers[order - 1]);
	else
	printf("%lx ", (unsigned long)n->pointers[n->num_keys]);
	}
	printf("| ");
	}*/
}


/* Traces the path from the root to a leaf, searching
* by key.  Displays information about the path
* if the verbose flag is set.
* Returns the leaf containing the given key.
*/
pagenum_t find_leaf(int64_t key)
{
	int i = 0;
	pagenum_t pagenum = 0;

	file_read_page(0, (page_t*)header);
	pagenum_t root_page = header->root;
	if ( root_page == 0 )
	{
		printf("Empty tree.\n");
		return 0;
	}
	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(root_page, page);
	pagenum = root_page;
	while ( !page->is_leaf )
	{
		i = page->num_keys - 1;
		while ( i >= 0 )
		{
			if ( page->branches[i].key > key ) i--;
			else break;
		}
		if ( i == -1 )
		{
			pagenum = page->leftmost_child;
			file_read_page(pagenum, page);
		}
		else
		{
			pagenum = page->branches[i].child;
			file_read_page(pagenum, page);
		}
	}
	return pagenum;
}

/* Finds and returns the record to which
* a key refers.
*/
int db_find(int64_t key, char * ret_val)
{
	int i = 0;

	pagenum_t finded_leafpage = find_leaf(key);

	page_t* page = (page_t*)malloc(sizeof(page_t));

	if ( finded_leafpage == 0 ) return 1;

	file_read_page(finded_leafpage, page);

	for ( i = 0; i < page->num_keys; i++ )
	{
		if ( page->records[i].key == key )
		{
			strcpy(ret_val, page->records[i].value);
			return 0;
		}
	}
	return 1;
}

/* Finds the appropriate place to
* split a node that is too big into two.
*/
int cut(int length)
{
	if ( length % 2 == 0 )
		return length / 2;
	else
		return length / 2 + 1;
}


// INSERTION

/* Creates a new record to hold the value
* to which a key refers.
*/
record_t * make_record(int64_t key, char* value)
{
	record_t * new_record = (record_t *)malloc(sizeof(record_t));
	if ( new_record == NULL )
	{
		perror("Record creation.");
		exit(EXIT_FAILURE);
	}
	else
	{
		new_record->key = key;
		strcpy(new_record->value, value);
	}
	return new_record;
}


/* Creates a new general node, which can be adapted
* to serve as either a leaf or an internal node.
*/
page_t * make_node(void)
{
	page_t * new_node = (page_t*)malloc(sizeof(page_t));

	if ( new_node == NULL )
	{
		perror("Node creation.");
		exit(EXIT_FAILURE);
	}

	memset(new_node, 0, sizeof(page_t));
	return new_node;
}

/* Creates a new leaf by creating a node
* and then adapting it appropriately.
*/
page_t * make_leaf(void)
{
	page_t * leaf = make_node();
	leaf->is_leaf = true;
	return leaf;
}


/* Helper function used in insert_into_parent
* to find the index of the parent's pointer to
* the node to the left of the key to be inserted.
*/
int get_left_index(pagenum_t parent_num, pagenum_t left_num)
{

	int left_index = 0;
	page_t* parent = (page_t*)malloc(sizeof(page_t));
	file_read_page(parent_num, parent);
	if ( parent->leftmost_child == left_num ) return 0;
	while ( left_index < parent->num_keys &&
		   parent->branches[left_index].child != left_num )
	{
		left_index++;
	}
	return left_index + 1;
}

/* Inserts a new pointer to a record and its corresponding
* key into a leaf.
* Returns the altered leaf.
*/
int insert_into_leaf(pagenum_t leaf_page, record_t * pointer)
{

	int i, insertion_point;
	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(leaf_page, page);

	insertion_point = 0;
	while ( insertion_point < page->num_keys && page->records[insertion_point].key < pointer->key )
		insertion_point++;

	for ( i = page->num_keys; i > insertion_point; i-- )
	{
		page->records[i].key = page->records[i - 1].key;
		strcpy(page->records[i].value, page->records[i - 1].value);
	}
	page->records[insertion_point].key = pointer->key;
	strcpy(page->records[insertion_point].value, pointer->value);
	page->num_keys++;
	file_write_page(leaf_page, page);
	return 0;
}


/* Inserts a new key and pointer
* to a new record into a leaf so as to exceed
* the tree's order, causing the leaf to be split
* in half.
*/
int insert_into_leaf_after_splitting(pagenum_t leaf_page, record_t * pointer)
{

	page_t * new_leaf;
	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(leaf_page, page);

	record_t temp_records[order];

	int insertion_index, split, i, j;
	int64_t new_key;

	new_leaf = make_leaf();
	pagenum_t new_leaf_num = file_alloc_page();

	insertion_index = 0;
	while ( insertion_index < order - 1 && page->records[insertion_index].key < pointer->key )
		insertion_index++;

	for ( i = 0, j = 0; i < page->num_keys; i++, j++ )
	{
		if ( j == insertion_index ) j++;
		temp_records[j].key = page->records[i].key;
		strcpy(temp_records[j].value, page->records[i].value);
	}

	temp_records[insertion_index].key = pointer->key;
	strcpy(temp_records[insertion_index].value, pointer->value);

	page->num_keys = 0;

	split = cut(order - 1);

	for ( i = 0; i <= split; i++ )
	{
		page->records[i].key = temp_records[i].key;
		strcpy(page->records[i].value, temp_records[i].value);
		page->num_keys++;
	}

	for ( i = split + 1, j = 0; i < order; i++, j++ )
	{
		new_leaf->records[j].key = temp_records[i].key;
		strcpy(new_leaf->records[j].value, temp_records[i].value);
		new_leaf->num_keys++;
	}

	new_leaf->right_sibling = page->right_sibling;
	page->right_sibling = new_leaf_num;

	new_leaf->parent = page->parent;
	new_key = new_leaf->records[0].key;

	file_write_page(new_leaf_num, new_leaf);
	file_write_page(leaf_page, page);

	return insert_into_parent(page->parent, leaf_page, new_key, new_leaf_num);
}


/* Inserts a new key and pointer to a node
* into a node into which these can fit
* without violating the B+ tree properties.
*/
int insert_into_node(pagenum_t parent_num,
					 int my_index, int64_t key, pagenum_t right_num)
{
	int i;

	page_t* parent = (page_t*)malloc(sizeof(page_t));
	file_read_page(parent_num, parent);

	for ( i = (parent->num_keys - 1); i >= my_index; i-- )
	{
		parent->branches[i + 1].key = parent->branches[i].key;
		parent->branches[i + 1].child = parent->branches[i].child;
	}
	parent->branches[my_index].child = right_num;
	parent->branches[my_index].key = key;
	parent->num_keys++;

	file_write_page(parent_num, parent);
	return 0;
}


/* Inserts a new key and pointer to a node
* into a node, causing the node's size to exceed
* the order, and causing the node to split into two.
*/
int insert_into_node_after_splitting(pagenum_t old_parent_num, int my_index,
									 int64_t key, pagenum_t right_num)
{

	int i, j, split;
	int64_t k_prime;

	page_t* new_node = make_node();
	pagenum_t new_node_num = file_alloc_page();

	page_t* old_parent = (page_t*)malloc(sizeof(page_t));
	file_read_page(old_parent_num, old_parent);

	/* First create a temporary set of keys and pointers
	* to hold everything in order, including
	* the new key and pointer, inserted in their
	* correct places.
	* Then create a new node and copy half of the
	* keys and pointers to the old node and
	* the other half to the new.
	*/

	branch_t temp_branches[250];

	for ( i = 0, j = 0; i < old_parent->num_keys; i++, j++ )
	{
		if ( j == my_index ) j++;
		temp_branches[j].key = old_parent->branches[i].key;
		temp_branches[j].child = old_parent->branches[i].child;
	}

	temp_branches[my_index].key = key;
	temp_branches[my_index].child = right_num;

	/* Create the new node and copy
	* half the keys and pointers to the
	* old and half to the new.
	*/
	split = cut(INTERNAL_ORDER);

	old_parent->num_keys = 0;

	for ( i = 0; i < split; i++ )
	{
		old_parent->branches[i].key = temp_branches[i].key;
		old_parent->branches[i].child = temp_branches[i].child;
		old_parent->num_keys++;
	}
	//old_parent->right_sibling = new_node_num;

	k_prime = temp_branches[split].key;
	new_node->leftmost_child = temp_branches[split].child;
	for ( ++i, j = 0; i <= old_parent->num_keys; i++, j++ )
	{
		new_node->branches[j].key = temp_branches[i].key;
		new_node->branches[j].child = temp_branches[i].child;
		new_node->num_keys++;
	}

	pagenum_t grand_parent = old_parent->parent;
	new_node->parent = grand_parent;

	page_t* child = (page_t*)malloc(sizeof(page_t));

	file_read_page(new_node->leftmost_child, child);

	child->parent = new_node_num;
	file_write_page(new_node->leftmost_child, child);

	for ( i = 0; i < new_node->num_keys; i++ )
	{
		file_read_page(new_node->branches[i].child, child);
		child->parent = new_node_num;
		file_write_page(new_node->branches[i].child, child);
	}

	/* Insert a new key into the parent of the two
	* nodes resulting from the split, with
	* the old node to the left and the new to the right.
	*/


	file_write_page(old_parent_num, old_parent);
	file_write_page(new_node_num, new_node);

	return insert_into_parent(grand_parent, old_parent_num, k_prime, new_node_num);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
* Returns the root of the tree after insertion.
*/
int insert_into_parent(pagenum_t parent_num, pagenum_t left_num, int64_t key, pagenum_t right_num)
{

	int my_index;

	page_t* parent = (page_t*)malloc(sizeof(page_t));

	/* Case: new root. */

	if ( parent_num == 0 )
		return insert_into_new_root(left_num, key, right_num);

	/* Case: leaf or node. (Remainder of
	* function body.)
	*/

	/* Find the parent's pointer to the left
	* node.
	*/

	file_read_page(parent_num, parent);
	my_index = get_left_index(parent_num, left_num);


	/* Simple case: the new key fits into the node.
	*/

	if ( parent->num_keys < 248 )
		return insert_into_node(parent_num, my_index, key, right_num);

	/* Harder case:  split a node in order
	* to preserve the B+ tree properties.
	*/

	return insert_into_node_after_splitting(parent_num, my_index, key, right_num);
}


/* Creates a new root for two subtrees
* and inserts the appropriate key into
* the new root.
*/
int insert_into_new_root(pagenum_t left, int64_t key, pagenum_t right)
{

	file_read_page(0, (page_t*)header);
	page_t * new_root = make_node();
	pagenum_t root_num = header->root = file_alloc_page();

	new_root->branches[0].key = key;
	new_root->branches[0].child = right;
	new_root->leftmost_child = left;
	new_root->num_keys++;

	page_t* left_page = (page_t*)malloc(sizeof(page_t));
	page_t* right_page = (page_t*)malloc(sizeof(page_t));
	file_read_page(left, left_page);
	file_read_page(right, right_page);
	left_page->parent = root_num;
	right_page->parent = root_num;

	file_write_page(root_num, new_root);
	file_write_page(0, (page_t*)header);
	file_write_page(left, left_page);
	file_write_page(right, right_page);

	return 0;
}



/* First insertion:
* start a new tree.
*/
int start_new_tree(record_t* pointer)
{

	file_read_page(0, (page_t*)header);
	page_t* root = make_leaf();
	pagenum_t root_num = header->root = file_alloc_page();

	root->records[0].key = pointer->key;
	strcpy(root->records[0].value, pointer->value);
	root->num_keys++;
	file_write_page(root_num, root); // update db_root_page
	file_write_page(0, (page_t*)header); // update header
	free(root);
	return 0;
}



/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/
int db_insert(int64_t key, char* value)
{

	record_t * pointer;
	pagenum_t leaf_page;

	file_read_page(0, (page_t*)header);

	/* The current implementation ignores
	* duplicates.
	*/
	char* temp = (char*)malloc(120);
	if ( !db_find(key, temp) )
		return 1;

	/* Create a new record for the
	* value.
	*/
	pointer = make_record(key, value);

	/* Case: the tree does not exist yet.
	* Start a new tree.
	*/

	if ( header->root == 0 )
		return start_new_tree(pointer);


	/* Case: the tree already exists.
	* (Rest of function body.)
	*/

	leaf_page = find_leaf(key);
	page_t* page = (page_t*)malloc(sizeof(page_t));
	file_read_page(leaf_page, page);

	/* Case: leaf has room for key and pointer.
	*/

	if ( page->num_keys < order - 1 )
	{
		return insert_into_leaf(leaf_page, pointer);
	}


	/* Case:  leaf must be split.
	*/

	return insert_into_leaf_after_splitting(leaf_page, pointer);
}




// DELETION.

/* Utility function for deletion.  Retrieves
* the index of a node's nearest neighbor (sibling)
* to the left if one exists.  If not (the node
* is the leftmost child), returns -1 to signify
* this special case.
*/
int get_neighbor_index(node * n)
{

	int i;

	/* Return the index of the key to the left
	* of the pointer in the parent pointing
	* to n.
	* If n is the leftmost child, this means
	* return -1.
	*/
	for ( i = 0; i <= n->parent->num_keys; i++ )
		if ( n->parent->pointers[i] == n )
			return i - 1;

	// Error state.
	printf("Search for nonexistent pointer to node in parent.\n");
	printf("Node:  %#lx\n", (unsigned long)n);
	exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer)
{

	int i, num_pointers;

	// Remove the key and shift other keys accordingly.
	i = 0;
	while ( n->keys[i] != key )
		i++;
	for ( ++i; i < n->num_keys; i++ )
		n->keys[i - 1] = n->keys[i];

	// Remove the pointer and shift other pointers accordingly.
	// First determine number of pointers.
	num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
	i = 0;
	while ( n->pointers[i] != pointer )
		i++;
	for ( ++i; i < num_pointers; i++ )
		n->pointers[i - 1] = n->pointers[i];


	// One key fewer.
	n->num_keys--;

	// Set the other pointers to NULL for tidiness.
	// A leaf uses the last pointer to point to the next leaf.
	if ( n->is_leaf )
		for ( i = n->num_keys; i < order - 1; i++ )
			n->pointers[i] = NULL;
	else
		for ( i = n->num_keys + 1; i < order; i++ )
			n->pointers[i] = NULL;

	return n;
}


node * adjust_root(node * root)
{

	node * new_root;

	/* Case: nonempty root.
	* Key and pointer have already been deleted,
	* so nothing to be done.
	*/

	if ( root->num_keys > 0 )
		return root;

	/* Case: empty root.
	*/

	// If it has a child, promote 
	// the first (only) child
	// as the new root.

	if ( !root->is_leaf )
	{
		new_root = root->pointers[0];
		new_root->parent = NULL;
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else
		new_root = NULL;

	free(root->keys);
	free(root->pointers);
	free(root);

	return new_root;
}


/* Coalesces a node that has become
* too small after deletion
* with a neighboring node that
* can accept the additional entries
* without exceeding the maximum.
*/
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime)
{

	int i, j, neighbor_insertion_index, n_end;
	node * tmp;

	/* Swap neighbor with node if node is on the
	* extreme left and neighbor is to its right.
	*/

	if ( neighbor_index == -1 )
	{
		tmp = n;
		n = neighbor;
		neighbor = tmp;
	}

	/* Starting point in the neighbor for copying
	* keys and pointers from n.
	* Recall that n and neighbor have swapped places
	* in the special case of n being a leftmost child.
	*/

	neighbor_insertion_index = neighbor->num_keys;

	/* Case:  nonleaf node.
	* Append k_prime and the following pointer.
	* Append all pointers and keys from the neighbor.
	*/

	if ( !n->is_leaf )
	{

		/* Append k_prime.
		*/

		neighbor->keys[neighbor_insertion_index] = k_prime;
		neighbor->num_keys++;


		n_end = n->num_keys;

		for ( i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++ )
		{
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
			n->num_keys--;
		}

		/* The number of pointers is always
		* one more than the number of keys.
		*/

		neighbor->pointers[i] = n->pointers[j];

		/* All children must now point up to the same parent.
		*/

		for ( i = 0; i < neighbor->num_keys + 1; i++ )
		{
			tmp = (node *)neighbor->pointers[i];
			tmp->parent = neighbor;
		}
	}

	/* In a leaf, append the keys and pointers of
	* n to the neighbor.
	* Set the neighbor's last pointer to point to
	* what had been n's right neighbor.
	*/

	else
	{
		for ( i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++ )
		{
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
		}
		neighbor->pointers[order - 1] = n->pointers[order - 1];
	}

	root = delete_entry(root, n->parent, k_prime, n);
	free(n->keys);
	free(n->pointers);
	free(n);
	return root;
}


/* Redistributes entries between two nodes when
* one has become too small after deletion
* but its neighbor is too big to append the
* small node's entries without exceeding the
* maximum
*/
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index,
						  int k_prime_index, int k_prime)
{

	int i;
	node * tmp;

	/* Case: n has a neighbor to the left.
	* Pull the neighbor's last key-pointer pair over
	* from the neighbor's right end to n's left end.
	*/

	if ( neighbor_index != -1 )
	{
		if ( !n->is_leaf )
			n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
		for ( i = n->num_keys; i > 0; i-- )
		{
			n->keys[i] = n->keys[i - 1];
			n->pointers[i] = n->pointers[i - 1];
		}
		if ( !n->is_leaf )
		{
			n->pointers[0] = neighbor->pointers[neighbor->num_keys];
			tmp = (node *)n->pointers[0];
			tmp->parent = n;
			neighbor->pointers[neighbor->num_keys] = NULL;
			n->keys[0] = k_prime;
			n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
		}
		else
		{
			n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
			neighbor->pointers[neighbor->num_keys - 1] = NULL;
			n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			n->parent->keys[k_prime_index] = n->keys[0];
		}
	}

	/* Case: n is the leftmost child.
	* Take a key-pointer pair from the neighbor to the right.
	* Move the neighbor's leftmost key-pointer pair
	* to n's rightmost position.
	*/

	else
	{
		if ( n->is_leaf )
		{
			n->keys[n->num_keys] = neighbor->keys[0];
			n->pointers[n->num_keys] = neighbor->pointers[0];
			n->parent->keys[k_prime_index] = neighbor->keys[1];
		}
		else
		{
			n->keys[n->num_keys] = k_prime;
			n->pointers[n->num_keys + 1] = neighbor->pointers[0];
			tmp = (node *)n->pointers[n->num_keys + 1];
			tmp->parent = n;
			n->parent->keys[k_prime_index] = neighbor->keys[0];
		}
		for ( i = 0; i < neighbor->num_keys - 1; i++ )
		{
			neighbor->keys[i] = neighbor->keys[i + 1];
			neighbor->pointers[i] = neighbor->pointers[i + 1];
		}
		if ( !n->is_leaf )
			neighbor->pointers[i] = neighbor->pointers[i + 1];
	}

	/* n now has one more key and one more pointer;
	* the neighbor has one fewer of each.
	*/

	n->num_keys++;
	neighbor->num_keys--;

	return root;
}


/* Deletes an entry from the B+ tree.
* Removes the record and its key and pointer
* from the leaf, and then makes all appropriate
* changes to preserve the B+ tree properties.
*/
node * delete_entry(node * root, node * n, int key, void * pointer)
{

	int min_keys;
	node * neighbor;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	n = remove_entry_from_node(n, key, pointer);

	/* Case:  deletion from the root.
	*/

	if ( n == root )
		return adjust_root(root);


	/* Case:  deletion from a node below the root.
	* (Rest of function body.)
	*/

	/* Determine minimum allowable size of node,
	* to be preserved after deletion.
	*/

	min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

	/* Case:  node stays at or above minimum.
	* (The simple case.)
	*/

	if ( n->num_keys >= min_keys )
		return root;

	/* Case:  node falls below minimum.
	* Either coalescence or redistribution
	* is needed.
	*/

	/* Find the appropriate neighbor node with which
	* to coalesce.
	* Also find the key (k_prime) in the parent
	* between the pointer to node n and the pointer
	* to the neighbor.
	*/

	neighbor_index = get_neighbor_index(n);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = n->parent->keys[k_prime_index];
	neighbor = neighbor_index == -1 ? n->parent->pointers[1] :
		n->parent->pointers[neighbor_index];

	capacity = n->is_leaf ? order : order - 1;

	/* Coalescence. */

	if ( neighbor->num_keys + n->num_keys < capacity )
		return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

	/* Redistribution. */

	else
		return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
*/
node * delete(node * root, int key)
{

	node * key_leaf;
	record * key_record;

	key_record = find(root, key, false);
	key_leaf = find_leaf(root, key, false);
	if ( key_record != NULL && key_leaf != NULL )
	{
		root = delete_entry(root, key_leaf, key, key_record);
		free(key_record);
	}
	return root;
}


void destroy_tree_nodes(node * root)
{
	int i;
	if ( root->is_leaf )
		for ( i = 0; i < root->num_keys; i++ )
			free(root->pointers[i]);
	else
		for ( i = 0; i < root->num_keys + 1; i++ )
			destroy_tree_nodes(root->pointers[i]);
	free(root->pointers);
	free(root->keys);
	free(root);
}


node * destroy_tree(node * root)
{
	destroy_tree_nodes(root);
	return NULL;
}

