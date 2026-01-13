// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once


//quicksort a singly linked list, returns a new start
//functions must be defined:
//T* T::next() - returns the next element
//void T::set_next(T*) - sets the next element

/*
//predicate example
struct lq
{
	bool operator()(const qtst& e1, const qtst& e2) const
	{
		return e1.key<=e2.key;
	};
};
*/

template <typename T, typename Predicate>
T* qsort_list(	T* first,	//from which element should we sort?
				T* last,	//next to last
				Predicate compare) //less
{
	if (first->next()==last)return first;//one element...

	//sort stack - can sort up to 2^64 elements
	//stores the task to sort between first and last, EXCLUDING THEMSELVES
	//if first==0, then the beginning of the sorted element must be taken from ret
	//i.e., if first==0, we conditionally assume first->next()=ret, or 0->next()=ret
	struct {T *first, *last;} *st,stack[64];

	T* ret=first;//returned element

	st=stack;

	st->first=nullptr;
	st->last=last;

	while(st>=stack) {
		
		first=st->first;
		last=st->last;

		T *f,*i;
		
		if (first)	f=first->next();
		else		{
			f=ret;
			ret=nullptr;
		}

		//find the base element in the middle of the list and cut it out from there
		unsigned n=0;
		for (i=f;i!=last;i=i->next())n++;
		n=n>>1;//middle element
		ASSUME(n>0);n--;
		for (i=f;n>0;i=i->next())n--;
		ASSUME(i!=last);
		T* base=i->next();
		ASSUME(base != first && base != last);
		i->set_next(base->next());
		base->set_next(last);
		if (first)first->set_next(base);
		
		unsigned n1=0,n2=0;//which list is longer

		while(f!=last){
			T* nxt=f->next();
			if (compare(*f,*base)){//f<base, insert between first and base
				if (first){
					f->set_next(first->next());
					first->set_next(f);
				}else{					
					if (ret)f->set_next(ret);
					else 	f->set_next(base);
					ret=f;
				}
				n1++;
			}else{//f>=base, insert between base and last
				f->set_next(base->next());
				base->set_next(f);
				n2++;
			}
			f=nxt;
		}
		if (!ret)ret=base;

		if (n1>n2){	//the list of elements smaller than base turned out to be longer, so we put it first
			if (n1>1)	st->last=base;
			else		st--;
			
			if (n2>1){
				st++;
				st->first=base;
				st->last=last;	
			}
		}else{	//the list of values greater than base turned out to be longer, so we put it first
			if (n2>1)	st->first=base;
			else		st--;

			if (n1>1){
				st++;
				st->first=first;
				st->last=base;
			}
		}
	}

#ifndef NDEBUG//self-test
	for (T* f=ret;f->next();f=f->next()){
		ASSUME(compare(*ret,*ret->next()) || !compare(*ret->next(),*ret));
	};
#endif
	
	return ret;
}