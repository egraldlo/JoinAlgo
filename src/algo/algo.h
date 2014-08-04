/*
    Copyright 2011, Spyros Blanas.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MYALGO__
#define __MYALGO__

#include <vector>
#include <iostream>
using namespace std;
#include <libconfig.h++>
#include "../table.h"
#include "../hash.h"
#include "../lock.h"
#include "../Barrier.h"
#include "../partitioner.h"
#include "hashtable.h"
#include <c++/4.4.4/cstdlib>

/* 1 is inner! */

class BaseAlgo {
	public:
		BaseAlgo(const libconfig::Setting& cfg);
		virtual ~BaseAlgo() { }

		/**
		 * General initialization function, guaranteed to be called by only *one* thread.
		 * Copies arguments to private space, generates output and s1 schema.
		 * (build schema is just {int, s1 schema}).
		 */
		virtual void init(
			Schema* schema1, vector<unsigned int> select1, unsigned int jattr1,
			Schema* schema2, vector<unsigned int> select2, unsigned int jattr2);

		virtual void destroy();

		virtual void build(SplitResult t, int threadid) = 0; 
		virtual PageCursor* probe(SplitResult t, int threadid) = 0;

	protected:
		/* s1, s2 are the two inputs, sout the output, sbuild the way the
		 * hash table has been built.
		 */
		Schema* s1, * s2, * sout, * sbuild;
		vector<unsigned int> sel1, sel2;
		unsigned int ja1, ja2, size, s1cols;
};

/**
 * Non thread-safe class
 */
class NestedLoops : public BaseAlgo {
	public:
		NestedLoops(const libconfig::Setting& cfg);
		virtual ~NestedLoops() { }
		virtual void destroy();
		virtual void build(SplitResult t, int threadid);
		virtual PageCursor* probe(SplitResult t, int threadid);
	private:
		/**
		 * Joins matching tuples in two buckets, calling \a joinPageTup for
		 * each tuple in \a b2.
		 * \a b1 conforms to \a sbuild and \a b2 to \a s2.
		 * @param output Output table to append joined tuples.
		 * @param p1 Page as first input to join.
		 * @param p2 Page as other input to join.
		 */
		void joinPagePage1(WriteTable* output, Page* p1, Page* p2);

		/**
		 * Joins matching tuples in bucket with single tuple. 
		 * \a bucket conforms to \a sbuild and \a tuple to \a s2.
		 * @param output Output table to append joined tuples.
		 * @param page Page as first input to join.
		 * @param tuple Tuple as other input to join.
		 */
		void joinPageTup(WriteTable* output, Page* page, void* tuple);

		/**
		 * Joins matching tuples in bucket with single tuple. 
		 * \a bucket conforms to \a sbuild and \a tuple to \a s2.
		 * @param output Output table to append joined tuples.
		 * @param page Page as first input to join.
		 * @param tuple Tuple as other input to join.
		 * @deprecated by \ref joinPageTup
		 */
		void joinBucketTup(WriteTable* output, Bucket* bucket, void* tuple) {
			joinPageTup(output, bucket, tuple);
		}

		void prepareBuild(WriteTable* build, PageCursor* orig);
		PageCursor* t1; 	// storage for first table
};

class HashBase : public BaseAlgo {
	public:
		HashBase(const libconfig::Setting& cfg);
		virtual ~HashBase();
		virtual void init(
			Schema* schema1, vector<unsigned int> select1, unsigned int jattr1,
			Schema* schema2, vector<unsigned int> select2, unsigned int jattr2);
		virtual void destroy();
		virtual void build(SplitResult t, int threadid) = 0;
		virtual PageCursor* probe(SplitResult t, int threadid) = 0;
	protected:
		HashFunction* _hashfn;
		HashTable hashtable;
		int nthreads;
		int outputsize;
#ifdef OUTPUT_AGGREGATE
		int* aggregator;
		static const int AGGLEN=512;
#endif
};

class StoreCopy : public HashBase {
	public:
		StoreCopy(const libconfig::Setting& cfg) : HashBase(cfg) {}
		virtual ~StoreCopy() {}

		virtual void init(
			Schema* schema1, vector<unsigned int> select1, unsigned int jattr1,
			Schema* schema2, vector<unsigned int> select2, unsigned int jattr2);
		virtual void destroy();
		
		virtual void build(SplitResult t, int threadid) = 0;
		virtual PageCursor* probe(SplitResult t, int threadid) = 0;

	protected:
		void buildCursor(PageCursor* t, int threadid, bool atomic);

		WriteTable* probeCursor(PageCursor* t, int threadid, bool atomic, WriteTable* ret = NULL);

	private:
		template <bool atomic>
		void realbuildCursor(PageCursor* t, int threadid);

		template <bool atomic>
		WriteTable* realprobeCursor(PageCursor* t, int threadid, WriteTable* ret = NULL);
};

class StorePointer : public HashBase {
	public:
		StorePointer(const libconfig::Setting& cfg) : HashBase(cfg) {}
		virtual ~StorePointer() {}

		virtual void init(
			Schema* schema1, vector<unsigned int> select1, unsigned int jattr1,
			Schema* schema2, vector<unsigned int> select2, unsigned int jattr2);
		virtual void destroy();

		virtual void build(SplitResult t, int threadid) = 0;
		virtual PageCursor* probe(SplitResult t, int threadid) = 0;

	protected:
		void buildCursor(PageCursor* t, int threadid, bool atomic);

		WriteTable* probeCursor(PageCursor* t, int threadid, bool atomic, WriteTable* ret = NULL);

	private:
		template <bool atomic>
		void realbuildCursor(PageCursor* t, int threadid);

		template <bool atomic>
		WriteTable* realprobeCursor(PageCursor* t, int threadid, WriteTable* ret = NULL);
};


template <typename Super>
class BuildIsPart : public Super {
	public:
		BuildIsPart(const libconfig::Setting& cfg) : Super(cfg) {}
		virtual ~BuildIsPart() {}
		virtual void build(SplitResult tin, int threadid){
			for (unsigned i=threadid; i<tin->size(); i+=Super::nthreads) {
				PageCursor* t = (*tin)[i];
				Super::buildCursor(t, threadid, false);
			}
		}
};

template <typename Super>
class BuildIsNotPart : public Super {
	public:
		BuildIsNotPart(const libconfig::Setting& cfg) : Super(cfg) {}
		virtual ~BuildIsNotPart() {}
		virtual void build(SplitResult tin, int threadid){
			PageCursor* t = (*tin)[0];
			Super::buildCursor(t, threadid, true);
		}
};


template <typename Super>
class ProbeIsPart : public Super {
	public:
		ProbeIsPart(const libconfig::Setting& cfg) : Super(cfg) {}
		virtual ~ProbeIsPart() {}
		virtual PageCursor* probe(SplitResult tin, int threadid){
			WriteTable* ret = NULL;
			for (int i=threadid; i<tin->size(); i+=Super::nthreads) {
				PageCursor* t = (*tin)[i];
				ret = Super::probeCursor(t, threadid, false, ret);
			}

			return ret;
		}
};

template <typename Super>
class ProbeIsNotPart : public Super {
	public:
		ProbeIsNotPart(const libconfig::Setting& cfg) : Super(cfg) {}
		virtual ~ProbeIsNotPart() {}
		virtual PageCursor* probe(SplitResult tin, int threadid){
			PageCursor* t = (*tin)[0];
			return Super::probeCursor(t, threadid, true);
		}
};

/** 
 * Work-stealing prober. 
 * Works only with non-partitioned build, otherwise will lose data. 
 */
template <typename Super>
class ProbeSteal : public Super {
	public:
		ProbeSteal(const libconfig::Setting& cfg) : Super(cfg) {}
		virtual ~ProbeSteal() {}
		virtual PageCursor* probe(SplitResult tin, int threadid){
			WriteTable* ret = NULL;
			for (int i=threadid; i<tin->size(); i+=Super::nthreads) {
				PageCursor* t = (*tin)[i];
				ret = Super::probeCursor(t, threadid, false, ret);
			}

			for (int i=0; i<tin->size(); ++i) {
				PageCursor* t = (*tin)[i];
				ret = Super::probeCursor(t, threadid, true, ret);
			}

			return ret;
		}
};

class FlatMemoryJoiner : public HashBase {
	public:
		FlatMemoryJoiner(const libconfig::Config& cfg);
		virtual ~FlatMemoryJoiner();

		virtual void init(
			Schema* schema1, vector<unsigned int> select1, unsigned int jattr1,
			Schema* schema2, vector<unsigned int> select2, unsigned int jattr2);
		virtual void destroy();
		virtual void build(SplitResult ignored, int threadid);
		virtual PageCursor* probe(SplitResult ignored, int threadid);

		void custominit(Table* tbuild, Table* tprobe);

	protected:
		RadixPartitioner partitioner;
		Page* probetable;
		vector<unsigned int>* phistogram;
		unsigned long totaltuples;
		vector<WriteTable*> result;
};

struct Snode{
	void *data;
	Snode *next;
};

class SortMergeJoiner{
public:
	SortMergeJoiner(const libconfig::Config& cfg){
		cout<<"creating sortmerge join! "<<endl;
		count=0;
		c=new Comparator();
		ColumnSpec cl(CT_LONG,0);
		ColumnSpec cr(CT_LONG,0);
		c->init(cl,8,cr,8);
	};
	~SortMergeJoiner(){};

	Snode* findMiddle(Snode *head){
		Snode *fast=head;
		Snode *slow=head;
		while(fast->next!=0&&fast->next->next!=0){
			fast=fast->next->next;
			slow=slow->next;
		}
		return slow;
	}

	int CompareTwoTuple(Snode *left, Snode* right){
		void *l=reinterpret_cast<char *>(left->data);
		void *r=reinterpret_cast<char *>(right->data);
		if(c->lessthan(l,r)){
			return 0;
		}
		if(c->equal(l,r)){
			return 1;
		}
		return 2;
	}

	Snode* mergeTwoList(Snode *left, Snode *right){
		Snode *ret=(Snode *)malloc(sizeof(Snode));
		ret->data=0;ret->next=0;
		Snode *r=ret;
		while(left!=0&&right!=0){
			/* compare the two tuple by using the schema. */
			if(CompareTwoTuple(left,right)==0){
				ret->next=left;
				ret=left;
				left=left->next;
			}
			else{
				ret->next=right;
				ret=right;
				right=right->next;
			}
		}
		while(left!=0){
			ret->next=left;
			ret=left;
			left=left->next;
		}
		while(right!=0){
			ret->next=right;
			ret=right;
			right=right->next;
		}
		return r->next;
	}

	/*
	 * quick and slow pointer to find the middle node and merge sort.
	 * */

	void cmsort(int i, int threadid){
		Snode *s=head[i][threadid]->next;
		head[i][threadid]=cmsortc(s);
	}

	Snode* cmsortc(Snode *root){
		if(root==0||root->next==0)
			return root;
		Snode *first=root;
		Snode *middle=findMiddle(root);
		Snode *second=middle->next;
		middle->next=0;
		Snode *l1=cmsortc(first);
		Snode *l2=cmsortc(second);
		return mergeTwoList(l1,l2);
	}

	void sort(Table *&t, Table *&t1){
		Snode *p[4];
		for(unsigned i=0;i<4;i++){
			head[0][i]=(Snode *)malloc(sizeof(Snode));
			head[0][i]->data=0;head[0][i]->next=0;
			p[i]=head[0][i];
		}
		int i=0;Snode *tmp;
		LinkedTupleBuffer *ltb=t->getRoot();
		while(ltb!=0){
			TupleBuffer::Iterator itr=ltb->createIterator();
			void *src=itr.next();
			while(src!=0){
				Snode *n=(Snode *)malloc(sizeof(Snode));
				n->data=(char *)malloc(t->schema()->getTupleSize());
				t->schema()->copyTuple(n->data,src);
				i=(*(long *)(src+8))%4;
				n->next=0;
				tmp=p[i];
				tmp->next=n;
				tmp=tmp->next;
				src=itr.next();
				p[i]=tmp;
			}
			ltb=ltb->getNext();
		}

		for(unsigned i=0;i<4;i++){
			head[1][i]=(Snode *)malloc(sizeof(Snode));
			head[1][i]->data=0;head[1][i]->next=0;
			p[i]=head[1][i];
		}
		int i1=0;Snode *tmp1;
		LinkedTupleBuffer *ltb1=t1->getRoot();
		while(ltb1!=0){
			TupleBuffer::Iterator itr1=ltb1->createIterator();
			void *src1=itr1.next();
			while(src1!=0){
				Snode *n1=(Snode *)malloc(sizeof(Snode));
				n1->data=(char *)malloc(t1->schema()->getTupleSize());
				t1->schema()->copyTuple(n1->data,src1);
				i1=(*(long *)(src1+8))%4;
				n1->next=0;
				tmp1=p[i1];
				tmp1->next=n1;
				tmp1=tmp1->next;
				src1=itr1.next();
				p[i]=tmp1;
			}
			ltb1=ltb1->getNext();
		}

//		return head->next;
	};

	void print(Table *t1,Table *t2){
//		left=sort(t1);
//		right=sort(t2);
//		Snode *s=left;
//		while(s!=0){
//			cout<<"sorted: "<<*(long *)(s->data+8)<<endl;
//			s=s->next;
//		}
//
//		s=right;
//		while(s!=0){
//			cout<<"sorted: "<<*(long *)(s->data+8)<<endl;
//			s=s->next;
//		}
	}

	void merge(int threadid){
		cout<<"merge phase!"<<endl;
		Snode *sl=head[0][threadid];
		Snode *sr=head[1][threadid];
		Snode *l_fake_buffer=0;
		Snode *r_fake_buffer=0;
		while(sl&&sr){
			if(CompareTwoTuple(sl,sr)==1){
				construct(sl,sr);
				l_fake_buffer=sl->next;
				r_fake_buffer=sr->next;
				while(l_fake_buffer!=0){
					if(CompareTwoTuple(l_fake_buffer,sr)==1){
						construct(l_fake_buffer,sr);
						l_fake_buffer=l_fake_buffer->next;
					}
					else{
						break;
					}
				}
				while(r_fake_buffer!=0){
					if(CompareTwoTuple(sl,r_fake_buffer)==1){
						construct(sl,r_fake_buffer);
						r_fake_buffer=r_fake_buffer->next;
					}
					else{
						break;
					}
				}
				sl=sl->next;
				sr=sr->next;
			}
			else if(CompareTwoTuple(sl,sr)==0){
				sl=sl->next;
			}
			else{
				sr=sr->next;
			}
		}
	};

	void construct(Snode *sl, Snode *sr){
		cout<<count++<<":         "<<*(long *)sl->data<<"|"<<*(long *)(sl->data+8)<<"|"<<*(long *)(sr->data)<<"|"<<*(long *)(sr->data+8)<<endl;


//		getchar();
	}

	Snode *left,*right;
private:
	/* s1, s2 are the two inputs, sout the output, sbuild the way the
	 * hash table has been built.
	 */
	Schema* s1, * s2, * sout;
	unsigned count;
	Comparator *c;
	Snode *head[2][4];//hard code
	/*
	 * select attribute on the table
	 * */
	vector<unsigned int> sel1, sel2;
	/*
	 * join operator linked in which attributes
	 * */
	unsigned int ja1, ja2, size, s1cols;

};

//#include "build.inl"
//#include "probe.inl"

#endif
