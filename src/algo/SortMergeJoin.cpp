/*
 * SortMergeJoin.cpp
 *
 *  Created on: 2014-8-7
 *      Author: casa
 */

#include "SortMergeJoin.h"
#include "../table.h"

#include <cstdlib>

SortMergeJoin::SortMergeJoin(const libconfig::Config& cfg) {

}

SortMergeJoin::~SortMergeJoin() {

}

unsigned long cTuples(PageCursor* t) {
	unsigned long ret = 0;

	int tupnum;
	Page* page;
	void* tup;

	t->reset();

	while (page = t->readNext()) {
		tupnum = 0;
		while (tup = page->getTupleOffset(tupnum++)) {
			++ret;
		}
	}

	t->reset();

	return ret;
}

void SortMergeJoin::range_partition(Table *&t) {
	unsigned long tuple_count=cTuples(t);
	unsigned long count=0;
	p_array=(void ***)malloc(THREADSIO*sizeof(void *));

	/* here we divided the whole table into 4 partitions which has
	 * the continued physical memory address */

	LinkedTupleBuffer *ltb=t->getRoot();
	while(ltb!=0){
		TupleBuffer::Iterator itr=ltb->createIterator();
		void *tuple_p=itr.next();
		while(tuple_p!=0){
			p_array[count]=tuple_p;
			tuple_p=itr.next();
			count++;
		}
		ltb=ltb->getNext();
	}
}

void SortMergeJoin::sort_table(int threadid) {

}

void SortMergeJoin::merge_tables() {

}
