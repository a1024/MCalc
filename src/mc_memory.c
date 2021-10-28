//mc_memory.h - Memory operations & C vector
//Copyright (C) 2021  Ayman Wagih Mohsen
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include"mc.h"
#include<stdio.h>
#include<stdlib.h>//malloc
#include<string.h>//memcpy
#include<stdarg.h>//va_list
#include<intrin.h>
static const char file[]=__FILE__;

//memory debugger
void	mem_print(const void *buf, int bytesize)
{
	int k;
	char *pb=(char*)buf;
	if(bytesize)
	{
		printf("%02X", (int)pb[0]&0xFF);
		for(k=1;k<bytesize;++k)
			printf("-%02X", (int)pb[k]&0xFF);
	}
	printf("\n");
}
void	print_byte_bin(char b)
{
	int k;
	for(k=7;k>=0;--k)
		printf("%d", b>>k&1);
}
void	mem_print_bin(const void *buf, int bytesize)
{
	int k;
	char *pb=(char*)buf;
	if(bytesize)
	{
		print_byte_bin(pb[0]);
		for(k=1;k<bytesize;++k)
		{
			printf("-");
			print_byte_bin(pb[k]);
			if(!((k+1)%10))
				printf("\n");
		}
	}
}
#ifdef DEBUG_MEMORY
#define	POINTERS_MAX	0x10000
typedef enum
{
	POINTER_FREED,
	POINTER_RELOCATED,
	POINTER_ACTIVE,
} PointerStatus;
const char* pointer_status2str(PointerStatus status)
{
	switch(status)
	{
#define	CASE(TYPE)	case TYPE:return #TYPE;
		CASE(POINTER_FREED)
		CASE(POINTER_RELOCATED)
		CASE(POINTER_ACTIVE)
#undef	CASE
	}
	return "<UNDEFINED>";
}
typedef struct
{
	union
	{
		const void *p;
		const char *bytes;
	};
	size_t bytesize;
	PointerStatus status;
} Pointer;
Pointer	pointers[POINTERS_MAX]={0};
int		npointers=0;
int		pointers_find(const void *p)
{
	int k;

	for(k=0;k<npointers;++k)
		if(pointers[k].p==p)
			return k;
	return npointers;
	//return -1;
}
void	pointers_action_alloc(const void *p_new, size_t bytesize)
{
	Pointer *pointer;
	int idx=pointers_find(p_new);

	if(idx>=POINTERS_MAX)
	{
		printf("\n\nPOINTERS.ALLOC: Need to track over %d pointers\n\n", POINTERS_MAX);
		_getch();
		abort();
	}

	pointer=pointers+idx;
	pointer->p=p_new;
	pointer->bytesize=bytesize;
	pointer->status=POINTER_ACTIVE;

	npointers+=idx==npointers;
}
void	pointers_action_realloc(const void *p_old, const void *p_new, size_t bytesize_new)
{
	Pointer *pointer;
	int idx1=pointers_find(p_old), idx2=pointers_find(p_new);
	if(idx1==npointers)
	{
		printf("\n\nPOINTERS.REALLOC: p_old=0x%p not found, p_new=0x%p\n\n", p_old, p_new);
		_getch();
		abort();
	}
	if(p_new==p_old)
		pointers[idx1].bytesize=bytesize_new;
	else
	{
		if(idx2<npointers&&pointers[idx2].status==POINTER_ACTIVE)
		{
			printf("\n\nPOINTERS.REALLOC: p_old=0x%p, p_new=0x%p was active\n\n", p_old, p_new);
			_getch();
			abort();
		}
		if(!p_new)
		{
			printf("\n\nPOINTERS.REALLOC: p_old=0x%p, p_new=0x%p failed\n\n", p_old, p_new);
			_getch();
			abort();
		}
		pointer=pointers+idx1;
		//pointer->bytesize=0;
		pointer->status=POINTER_RELOCATED;

		pointer=pointers+idx2;
		pointer->p=p_new;
		pointer->bytesize=bytesize_new;
		pointer->status=POINTER_ACTIVE;

		npointers+=idx2==npointers;
	}
}
int		pointers_action_free(const void *p_old)
{
	int idx=pointers_find(p_old);
	if(idx==npointers)
	{
		printf("\n\nPOINTERS.FREE: p_old=0x%p not found\n\n", p_old);
		return -1;
		//_getch();
		//abort();
	}
	if(pointers[idx].status!=POINTER_ACTIVE)
	{
		printf("\n\nPOINTERS.FREE: p_old=0x%p status=%s\n\n", p_old, pointer_status2str(pointers[idx].status));
		return -1;
		//_getch();
		//abort();
	}
	//pointers[idx].bytesize=0;
	pointers[idx].status=POINTER_FREED;
	return 0;
}
void	pointers_action_access(const void *p)
{
	Pointer *pointer;
	int k, found=0;
	static int access_count=0;
	
	printf("\nPOINTERS.ACCESS: 0x%p\n", p);
	for(k=0;k<npointers;++k)//find in range
	{
		pointer=pointers+k;
		if((char*)p>=pointer->bytes&&(char*)p<pointer->bytes+pointer->bytesize)//found
		{
			found=1;
			printf("\tp=0x%p, %d bytes status=%s\n", pointer->p, pointer->bytesize, pointer_status2str(pointer->status));
			if(pointer->status!=POINTER_ACTIVE)
				_getch();
		}
	}
	if(!found)
		printf("\tnot found\n", p);
	++access_count;
}
#if 0
void	pointers_add(void *p)
{
	if(npointers>=POINTERS_MAX)
	{
		printf("\n\nPOINTERS: Need to track over %d pointers\n\n", POINTERS_MAX);
		_getch();
		return;
	}
	pointers[npointers]=p;
	++npointers;
}
void	pointers_remove(void *p)
{
	int idx=pointers_find(p);
	if(idx==-1)
	{
		printf("\n\nPOINTERS.REMOVE: 0x%p not found\n\n", p);
		_getch();
		return;
	}
	memmove(pointers+idx, pointers+idx+1, (npointers-1-idx)*sizeof(void*));
	--npointers;
}
void	pointers_replace(void *p_old, void *p_new)
{
	int idx=pointers_find(p_old), k;
	if(idx==-1)
	{
		printf("\n\nPOINTERS.REPLACE: p_old=0x%p not found, p_new=0x%p\n\n", p_old, p_new);
		_getch();
		return;
	}
	pointers[idx]=p_new;
}
#endif

int		syscall_count=0, emergency_flag=0;
void*	d_alloc		(const char *file, int line, unsigned long bytesize)
{
	void *p;
	++syscall_count;
	printf("%s(%d): #%d malloc 16 + %ld", get_filename(file), line, syscall_count, bytesize-16);
	p=malloc(bytesize);
	printf(" -> 0x%p\n", p);

	pointers_action_alloc(p, bytesize);
	//pointers_add(p);//

	return p;
}
void*	d_realloc	(const char *file, int line, void *p, unsigned long bytesize)
{
	void *p2;
	++syscall_count;
	printf("%s(%d): #%d realloc 16 + %ld, 0x%p", get_filename(file), line, syscall_count, bytesize-16, p);
	p2=realloc(p, bytesize);
	printf(" -> 0x%p\n", p2);

	pointers_action_realloc(p, p2, bytesize);
	//pointers_replace(p, p2);//

	if(p2)
		return p2;
	return p;
}
int		d_free		(const char *file, int line, void *p)
{
	static int *prev_p=0;//
	int status=0;

	++syscall_count;
	printf("%s(%d): #%d free 0x%p", get_filename(file), line, syscall_count, p);
	if(prev_p==p)//
	{
		printf("\n\nWarning: attempting to free 0x%p second time in a row.\n", prev_p);
		_getch();
	}

	status=pointers_action_free(p);
	if(status<0)
	{
		printf("Expecting a crash at free()\n");
		_getch();
	}
	//	return status;
	//pointers_remove(p);//

	//if(syscall_count==245)//
	//{
	//	printf("CRASH marker1\n");
	//	_getch();
	//}
	if(p)
		free(p);
	//if(syscall_count==245)//
	//	printf("CRASH marker2\n");
	prev_p=p;//
	printf("\n");

	return status;
}
void d_memcpy	(const char *file, int line, void *dst, const void *src, int bytesize)
{
	++syscall_count;
	printf("%s(%d): #%d memcpy 0x%p := 0x%p, %d before:\n\t", get_filename(file), line, syscall_count, dst, src, bytesize);

	pointers_action_access(src);//
	pointers_action_access(dst);//

	mem_print(dst, bytesize);

	memcpy(dst, src, bytesize);

	printf("\t...after:\n\t");
	mem_print(dst, bytesize);
}
void d_memmove	(const char *file, int line, void *dst, const void *src, int bytesize)
{
	++syscall_count;
	printf("%s(%d): #%d memmove 0x%p := 0x%p, %d before:\n\t", get_filename(file), line, syscall_count, dst, src, bytesize);

	pointers_action_access(src);//
	pointers_action_access(dst);//

	mem_print(dst, bytesize);

	memmove(dst, src, bytesize);

	printf("\t...after:\n\t");
	mem_print(dst, bytesize);
}
void d_memset	(const char *file, int line, void *dst, int val, int bytesize)
{
	++syscall_count;
	printf("%s(%d): #%d memset 0x%p := %d, %d before:\n\t", get_filename(file), line, syscall_count, dst, val, bytesize);

	pointers_action_access(dst);//

	mem_print(dst, bytesize);

	memset(dst, val, bytesize);

	printf("\t...after:\n\t");
	mem_print(dst, bytesize);
}
#include		"mc_redirect_calls.h"
#endif


//data structures for c
int		strcmp_advance(const char *s1, const char *label, int *advance)//returns zero if label matches
{
	if(advance)
	{
		*advance=0;
		for(;*s1&&*label&&*s1==*label;++s1, ++label, ++*advance);
	}
	else
		for(;*s1&&*label&&*s1==*label;++s1, ++label);
	return *label!=0;
//	return (*s1>*label)-(*s1<*label);
}

//c vector v4: struct header
//c vector v3: TODO: recursive destructor: store pointer positions of struct in array at start of header
//TODO: vector header is a struct
void	mem_test(const char *file, int line, const char *msg, ...)
{
	void *p;
	if(msg)
		vprintf(msg, (char*)(&msg+1));
	p=malloc(16);
	if(!p)
	{
		printf("Memtest failed\n");
		_getch();
		exit(1);
	}
	free(p);
}
void	mem_shiftback(void *dst, const void *src, int bytesize)//shift left
{
	const char *c1=(const char*)src;
	char *c2=(char*)dst;
	int k;
	if(dst!=src)
		for(k=0;k<bytesize;++k)
			c2[k]=c1[k];
}
void	mem_shiftforward(const void *src, void *dst, int bytesize)//shift right
{
	const char *c1=(const char*)src;
	char *c2=(char*)dst;
	int k;
	if(src!=dst)
		for(k=bytesize-1;k>=0;--k)
			c2[k]=c1[k];
}
void	mem_rotate(void *begin, void *newbegin, const void *end)//https://www.cplusplus.com/reference/algorithm/rotate/
{
	char *first=(char*)begin, *middle=(char*)newbegin, *last=(char*)end,
		*next=middle, temp;
	while(first!=next)
	{
		temp=*first, *first=*next, *next=temp;
		++first, ++next;
		if(next==last)
			next=middle;
		else if(first==middle)
			middle=next;
	}
}
void	memfill(void *dst, const void *src, int dstbytes, int srcbytes)//repeating pattern
{
#if 1//#1
	int copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
#endif
#if 0
	char *p1, *p2, *end;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	memcpy(dst, src, srcbytes);//initialize first pattern
	p1=(char*)dst, p2=p1+srcbytes, end=start+dstbytes;
	while(start<end)//fill pattern
		*p2++=*p1++;
#endif
#if 0
	char *p1, *p2, *end;
	const int sizelong=sizeof(long);
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	memcpy(dst, src, srcbytes);//initialize first pattern
	p1=(char*)dst, p2=p1+srcbytes, end=start+dstbytes;
	__stosd((unsigned long*)p2, *(unsigned long*)p1, (dstbytes-srcbytes)/sizelong);
	p2=p1+(dstbytes&~(sizelong-1));
	while(p2<end)//fill pattern
		*p2++=*p1++;
#endif
}
//void	mem_fillpattern2(const char *file, int line, void *p, int bytesize, int esize, const void *data)
//void	mem_fillpattern(void *p, int bytesize, int esize, const void *data)
//{
//	char *pc=(char*)p;
//	int k;
//	memcpy(pc, data, esize);//initialize first pattern
//	for(k=esize;k<bytesize;++k)//fill pattern
//		pc[k]=pc[k-esize];
//}
CVecHeader*			v_getptr		(void *pv)
{
	CVecHeader *ph=*(CVecHeader**)pv;
	ASSERT(ph, "v_getptr: NULL pointer\n");
	if(ph)
		return ph-1;
#ifdef DEBUG_CVECTOR
	printf("[%p] = NULL\n", pv);
#endif
	return 0;
}
CVecHeader const*	v_getptr_const	(const void *pv)
{
	CVecHeader const *ph=*(CVecHeader const**)pv;
	if(ph)
	{
#ifdef DEBUG_CVECTOR
	printf("v_getptr[%p] = 0x%p\n", pv, ph);
#endif
		return ph-1;
	}
#ifdef DEBUG_CVECTOR
	printf("[%p] = NULL\n", pv);
#endif
	ASSERT(ph, "v_getptr_const: NULL pointer\n");
	return 0;
}
#ifdef DEBUG_CVECTOR
//void	str_print(const char *str, int size)
//{
//	int k;
//	for(k=0;k<size;++k)
//		putc(str[k], stdout);
//}
void	v_print_payload_pre(CVecHeader const *h, const char *msg, ...)
{
	const char *type, *name;
	if(msg)
		vprintf(msg, (char*)(&msg+1));
	if(h)
	{
		type=(char*)(h->payload+h->ploc_size), name=type+h->type_size;

	//	str_print(type, h->type_size);
		printf("%.*s", h->type_size, type);
		printf("(%d)*  ", (int)h->esize);
		printf("%s", name);
		printf("(%d) = ", h->count);
		printf("%p...\n", h);
	//	printf("%*s * (%h) %s(%d)...\n", h->type_size, type, h->esize, name, h->count);//ellipsis
		if(strlen(name)!=h->name_size)//
			ASSERT(strlen(name)==h->name_size, "Corrupt vector payload\n");
	}
	else
		printf("NULL...\n");
}
void	v_print_payload_pre_nl(CVecHeader const *h, const char *msg, ...)
{
	const char *type, *name;
	if(msg)
		vprintf(msg, (char*)(&msg+1));
	if(h)
	{
		type=(char*)(h->payload+h->ploc_size), name=type+h->type_size;
		
	//	str_print(type, h->type_size);
		printf("%.*s", h->type_size, type);
		printf("(%d)*  ", h->esize);
		printf("%s", name);
		printf("(%d)\n\n", h->count);
	//	printf("%*s*(%h) %s(%d)\n\n", h->type_size, type, h->esize, name, h->count);//extra newline
	}
	else
		printf("NULL\n\n");
}
#else
#define	v_print_payload_pre(...)
#define	v_print_payload_pre_nl(...)
#endif
void	v_construct(void *pv, int count, int esize, const void *elem, const int *pointerlocations	CVEC_DEBUG_ARGS)
{
	int bytesize=count*esize, payloadbytes;
	CVecHeader *ph;
#ifdef DEBUG_CVECTOR
	int byteidx;
#endif
#ifdef DEBUG_CVECTOR
	printf("Constructing %s(%d)* %s(%d)...\n", type, esize, name, count);
#endif
	ASSERT(bytesize>=0, "Requested negative size array\nv_construct(count=%d, esize=%d)\n", count, esize);
	ph=(CVecHeader*)malloc(sizeof(CVecHeader)+bytesize);

	ph->count=count;//write header
	ph->esize=esize;
	ph->ploc_size=0;
	if(pointerlocations)
		for(;pointerlocations[ph->ploc_size];++ph->ploc_size);
	payloadbytes=ph->ploc_size*sizeof(int);
#ifdef DEBUG_CVECTOR
	ph->type_size=type?strlen(type):0;
	ph->name_size=name?strlen(name):0;
	payloadbytes+=ph->type_size+ph->name_size+1;//with null terminator
#endif
	ph->unused=0;//clear padding in header struct
	if(payloadbytes)
	{
		ph->payload=(int*)malloc(payloadbytes);
		if(ph->ploc_size)
			memcpy(ph->payload, pointerlocations, ph->ploc_size*sizeof(int));//write payload
#ifdef DEBUG_CVECTOR
		byteidx=ph->ploc_size*sizeof(int);
		memcpy(ph->cpayload+byteidx, type, ph->type_size);
		byteidx+=ph->type_size;
		memcpy(ph->cpayload+byteidx, name, ph->name_size+1);
#endif
	}
	else
		ph->payload=0;

	*(void**)pv=ph+1;//assign pointer

	if(elem)
		memfill(*(void**)pv, elem, bytesize, esize);//initialize
	else//struct may contain pointers that better be initialized to NULL
		memset(*(void**)pv, 0, bytesize);

	v_print_payload_pre_nl(ph, "Done ");//
}
void	v_move(void *pdst, void *psrc)
{
	if(pdst!=psrc)
	{
		v_destroy(pdst);
		*(int**)pdst=*(int**)psrc;
		*(int**)psrc=0;
	}
}
void	v_destroy(void *pv)
{
	CVecHeader *ph=v_getptr(pv);
	v_print_payload_pre(ph, "Destroying ");//
	if(ph)
	{
#ifdef DEBUG_MEMORY
		printf("v_destroy(): 0x%p, esize=%d, count=%d, payload=0x%p, data=0x%p\n", ph, ph->esize, ph->count, ph->payload, ph->data);
#endif
		if(ph->payload)
			free(ph->payload);
		free(ph);
	}

#ifdef DEBUG_CVECTOR
	printf("Done\n\n");
#endif
}
void	v_insert(void *pv, int idx, const void *begin, int count)
{
	CVecHeader *ph=v_getptr(pv);
	int bytesize=ph->count*ph->esize, bytepos=idx*ph->esize, bytediff=count*ph->esize;
	char *start;
	ASSERT(ph, "v_insert: NULL pointer\n");
	v_print_payload_pre(ph, "Inserting %d at %d to ", count, idx);//
	
	ASSERT(bytesize+bytediff>=0, "Requested negative size array\nv_insert(idx=%d, count=%d)", idx, count);
	ph=(CVecHeader*)realloc(ph, sizeof(CVecHeader)+bytesize+bytediff);
	ph->count+=count;

	*(void**)pv=start=(char*)(ph+1);
	mem_shiftforward(start+bytepos, start+bytepos+bytediff, bytesize-bytepos);
	bytesize+=bytediff;

	if(begin)
		memcpy(start+bytepos, begin, bytediff);
	else//struct may contain pointers that better be initialized to NULL
		memset(start+bytepos, 0, bytediff);
	v_print_payload_pre_nl(ph, "Done ");//
}
void	v_erase(void *pv, int idx, int count)
{
	CVecHeader *ph=v_getptr(pv);
	int bytesize, bytepos, bytediff;
	char *start;

#ifdef DEBUG_MEMORY
	if(emergency_flag)
		printf("v_erase(idx=%d, rem=%d) - before: 0x%p, count=%d\n", idx, count, ph, ph->count);
#endif

	ASSERT(ph, "v_erase: NULL pointer\n");
	bytesize=ph->count*ph->esize, bytepos=idx*ph->esize, bytediff=count*ph->esize;
	v_print_payload_pre(ph, "Erasing %d at %d from ", count, idx);//
	
	start=(char*)(ph+1);
	mem_shiftback(start+bytepos, start+bytepos+bytediff, bytesize-bytediff-bytepos);
	
	ASSERT(bytesize>=bytediff, "v_erase %d from %d-size vector, at %d", bytediff, bytesize, idx);
	ph=(CVecHeader*)realloc(ph, sizeof(CVecHeader)+bytesize-bytediff);
	ph->count-=count;
	*(void**)pv=ph+1;
	v_print_payload_pre_nl(ph, "Done ");//

#ifdef DEBUG_MEMORY
	if(emergency_flag)
		printf("v_erase() - after: 0x%p, count=%d\n", ph, ph->count);
#endif
}
void	v_assign(void *pdst, const void *psrc)
{
	int bytesize, payloadbytes;
	CVecHeader *hdst;
	CVecHeader const *hsrc;

	hsrc=v_getptr_const(psrc);
	ASSERT(hsrc, "v_assign: NULL pointer\n");
	v_print_payload_pre(hsrc, "Assigning ");//
	v_print_payload_pre(*(void**)pdst?v_getptr(pdst):0, "To ");//
	bytesize=hsrc->count*hsrc->esize;
	if(*(void**)pdst)
	{
		hdst=v_getptr(pdst);
		v_resize(pdst, hsrc->count, 0);
		memcpy(*(void**)pdst, *(void**)psrc, bytesize);
#ifdef DEBUG_CVECTOR
		hdst=v_getptr(pdst);
#endif
	}
	else//duplicate everything
	{
		hdst=(CVecHeader*)malloc(sizeof(CVecHeader)+bytesize);
		memcpy(hdst, hsrc, sizeof(CVecHeader)+bytesize);//copy array

#ifdef DEBUG_CVECTOR
		payloadbytes=hsrc->ploc_size*sizeof(int)+hsrc->type_size+hsrc->name_size+1;
#else
		payloadbytes=hsrc->ploc_size*sizeof(int);
#endif
		if(payloadbytes)
		{
			hdst->payload=(int*)malloc(payloadbytes);
			memcpy(hdst->payload, hsrc->payload, payloadbytes);//copy payload
		}
		else
			hdst->unused=0;

		*(CVecHeader**)pdst=hdst+1;
	}
#ifdef DEBUG_MEMORY
	printf("v_assign():\n");
	printf("\tsrc: 0x%p, esize=%d, count=%d, payload=0x%p, data=0x%p\n", hsrc, hsrc->esize, hsrc->count, hsrc->payload, hsrc->data);
	printf("\tdst: 0x%p, esize=%d, count=%d, payload=0x%p, data=0x%p\n", hdst, hdst->esize, hdst->count, hdst->payload, hdst->data);
#endif
	v_print_payload_pre(hdst, "Done ");//
}
void	v_check(const char *file, int line, const void *pv, int idx)
{
	CVecHeader const* ph;
	if(!pv)
	{
		printf("\nCRASH at %s(%d): pv=0x%p\n", get_filename(file), line, pv);
		exit(1);
	}
	ph=v_getptr_const(pv);
	if(idx<0||idx>ph->count)
	{
		printf("\nCRASH at %s(%d): count=%d, idx=%d, esize=%d\n", get_filename(file), line, ph->count, idx, (int)ph->esize);
		exit(1);
	}
}

//derived functions
void	v_push_back(void *pv, const void *data)
{
	CVecHeader *ph=v_getptr(pv);
	ASSERT(ph, "v_push_back: NULL pointer\n");
	v_insert(pv, ph->count, data, 1);
}
void	v_pop_back(void *pv)
{
	v_erase(pv, v_size(pv)-1, 1);
}
void	v_append(void *pv, const void *begin, int count)
{
	CVecHeader *ph=v_getptr(pv);
	ASSERT(ph, "v_append: NULL pointer\n");
	v_insert(pv, ph->count, begin, count);
}
void	v_resize(void *pv, int count, const void *data)
{
	CVecHeader *ph=v_getptr(pv);
	int diff, bytesize, bytediff;

	ASSERT(ph, "v_resize: NULL pointer\n");
	diff=count-ph->count;
#ifdef DEBUG_MEMORY
	printf("v_resize() - before:\n\told=0x%p, esize=%d, count=%d->%d,\n\tpayload=0x%p, data=0x%p\n", ph, ph->esize, ph->count, count, ph->payload, ph->data);
	if(emergency_flag)
		emergency_flag=emergency_flag;
#endif
#ifdef DEBUG_CVECTOR
	v_print_payload_pre(ph, "Resizing by %d ", diff);//
#endif
	if(diff>0)
	{
		bytesize=ph->count*ph->esize, bytediff=diff*ph->esize;
		v_insert(pv, ph->count, 0, diff);
		ph=v_getptr(pv);//realloc changes pointer
		if(data)
			memfill(*(char**)pv+bytesize, data, bytediff, ph->esize);
	}
	else if(diff<0)
		v_erase(pv, ph->count+diff, -diff);
#ifdef DEBUG_MEMORY
	bytesize=(int)ph;
	ph=v_getptr(pv);
	printf("v_resize() - after:\n\t0x%p->0x%p, esize=%d, count=%d->%d (required: %d),\n\tpayload=0x%p, data=0x%p\n", (void*)bytesize, ph, ph->esize, ph->count-diff, ph->count, count, ph->payload, ph->data);
#endif
	//diff=0;//
}
void	v_fill(void *pv, const void *elem)
{
	CVecHeader *ph=v_getptr(pv);
	ASSERT(ph, "v_fill: NULL pointer\n");

	memfill(ph+1, elem, ph->count*ph->esize, ph->esize);
}
void	v_clear(void *pv)
{
	v_resize(pv, 0, 0);
}
void	v_print(const void *pv, const char *msg, ...)
{
	va_list args;
	CVecHeader const *ph=v_getptr_const(pv);
	char *start=(char*)(ph+1);
	int bytesize=ph->count*ph->esize, k;

	if(msg)
	{
		va_start(args, msg);
		vprintf(msg, args);
		va_end(args);
		printf("\n");
	}
	v_print_payload_pre(ph, "Printing ");
	printf("Has %d pointers\n", ph->ploc_size);
	printf("\nContents:\n");
	for(k=0;k<bytesize;k+=ph->esize)
	{
		mem_print(start+k, ph->esize);
	}
	printf("\n");
}
int		v_size(const void *pv)
{
	CVecHeader const *ph=v_getptr_const(pv);
	return ph->count;
}