// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../predix_sdk/common_type.h"



extern void qinit(stmyqueue *q, unsigned char* pdata, unsigned int size);
extern void qclear(stmyqueue *q);
extern void qquit(stmyqueue *q);
// for circular queue
extern int enqueues(stmyqueue *q,unsigned char * pdata, int count);
extern int enqueue(stmyqueue *q,unsigned char data);
extern int is_qfull(stmyqueue *q);
extern int is_qempty(stmyqueue *q);
extern int dequeue(stmyqueue *q, unsigned char * ptgt);
extern int qpeek(stmyqueue *q, unsigned char * ptgt, int spos = 0);
extern int qpeeks(stmyqueue *q, unsigned char * ptgt, int size);
extern int qpop(stmyqueue *q);
extern int qpops(stmyqueue *q, int size);
extern int dequeues(stmyqueue *q, unsigned char * tgt, int size);
extern  int qcount(stmyqueue *q);
extern int q_search_header(stmyqueue *q, unsigned char * tgt, int tgtsize /*max 16*/);
extern int search_header_in_data(unsigned char *tgt, int tgt_size, unsigned char * header, int h_size);
extern  unsigned char crxtags[6];

//-----------------------------------
void qprint(stmyqueue *q);
