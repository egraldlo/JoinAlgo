/*
 * SortMergeJoin.h
 *
 *  Created on: 2014-8-7
 *      Author: casa
 */

#ifndef SORTMERGEJOIN_H_
#define SORTMERGEJOIN_H_
#include <libconfig.h++>
#include "../table.h"

#define THREADSIO 4

class SortMergeJoin {
public:
	SortMergeJoin(const libconfig::Config& cfg);
	virtual ~SortMergeJoin();

public:
	void range_partition(Table *&t);
	void sort_table(int threadid);
	void merge_tables();

private:
	void ***p_array;
};

#endif /* SORTMERGEJOIN_H_ */
