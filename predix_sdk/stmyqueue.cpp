// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "stmyqueue.h"
#include <stdio.h>
#include <windows.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



unsigned char crxtags[6] = { 0xFD, 0xFF, 0x01, 0x00, 0xFB , 0xFF };
stmyqueue udpqueue;
stmyqueue tcpqueuedevice;
stmyqueue tcpqueuesdk;
unsigned char udp_q_data[MY_QUEUE_SIZE + 4];//데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요
unsigned char tcp_q_data_sdk[MY_QUEUE_SIZE + 4];//데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요
unsigned char tcp_q_data_device[MY_QUEUE_SIZE + 4];//데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요


void qclear(stmyqueue* q) { q->qhead = q->qtail = 0; q->qdatasize = 0; q->qfreesize = q->qcapacity - 1; }//front rear 겹치는거 체크용 1개 비워두니까
void qinit(stmyqueue* q, unsigned char* pdata, unsigned int size) { q->qdata = pdata; q->qcapacity = size; qclear(q); }
void qquit(stmyqueue* q) { qclear(q); q->qdata = NULL; q->qcapacity = 0; }


int search_header_in_data(unsigned char* tgt, int tgt_size, unsigned char* header, int h_size)
{
	int header_pos = -1;
	if (tgt_size < h_size)
	{
		return header_pos;
	}
	//int hcount = 0;
	int is_header = 0;//fw단에는 bool이 없어서 int를 썼음
	for (int i = 0; !is_header && i < tgt_size; i++)
	{
		if (tgt[i] != header[0])
			continue;
		is_header = 1;
		for (int j = 1; is_header && j < h_size; j++)
		{
			if (tgt[i + j] != tgt[j])
				is_header++;
		}
		if (is_header == h_size)
		{
			header_pos = i;
		}
		else
		{
			is_header = 0;
		}
	}
	if (header_pos != -1)
	{
		return tgt[header_pos + 6];// network_command
	}
	else
		return header_pos;
}


int q_search_header(stmyqueue* q, unsigned char* tgt, int tgtsize/*max 16*/)
{
	int header_pos = -1;
	if (q->qdatasize < tgtsize)
	{
		return header_pos;
	}
	//int hcount = 0;
	unsigned char buffer[16];
	int is_header = 0;
	for (int i = 0; !is_header && i < q->qdatasize; i++)
	{
		if (qpeek(q, &buffer[0], i) < 1)
			break;
		if (buffer[0] != tgt[0])
			continue;
		is_header = 1;
		for (int j = 1; is_header && j < tgtsize; j++)
		{
			if (qpeek(q, &buffer[j], i + j) < 1)
			{
				is_header = 0;
				break;
			}
			if (buffer[j] != tgt[j])
				is_header = 0;
		}
		if (is_header)
			header_pos = i;
	}
	return header_pos;
}


int enqueues(stmyqueue* q, unsigned char* src, int srcsize)
{
	if (srcsize < 0 || !q->qdata)
		return -99;
	if (is_qfull(q))
		return -1;
	int count = q->qfreesize < srcsize ? q->qfreesize : srcsize;
	if (q->qtail + count < q->qcapacity)
	{//linear
		memcpy(&(q->qdata[q->qtail + 1]), src, count);
		q->qtail = (q->qtail + count) % q->qcapacity; // 선형구간이므로   %안해되 되나 그냥 흐름상 일단 똑같이 해놓음
		q->qdatasize += count;
		q->qfreesize -= count;
		return 1;
	}
	else
	{//rotate
		int linear_count = q->qcapacity - q->qtail - 1;
		if (linear_count != 0)
		{
			memcpy(&(q->qdata[q->qtail + 1]), src, linear_count);
			q->qtail = (q->qtail + linear_count) % q->qcapacity;
			q->qdatasize += linear_count;
			q->qfreesize -= linear_count;
			src += linear_count;
		}
		linear_count = count - linear_count;
		memcpy(&(q->qdata[(q->qtail + 1) % q->qcapacity]), src, linear_count);
		q->qtail = (q->qtail + linear_count) % q->qcapacity;
		q->qdatasize += linear_count;
		q->qfreesize -= linear_count;

		return count;
	}
}


int enqueue(stmyqueue* q, unsigned char data)
{
	if (is_qfull(q))
		return -1;
	q->qtail = (q->qtail + 1) % q->qcapacity;//1칸 비워두기 위해 먼저 올림
	q->qdata[q->qtail] = data;
	q->qdatasize++;
	q->qfreesize--;
	return 1;
}


int is_qfull(stmyqueue* q)
{
	if (((q->qtail + 1) % q->qcapacity) == q->qhead)
		return 1;
	else
		return 0;
}


int qcount(stmyqueue* q)
{
	if (q->qtail == q->qhead)//is_qempty()
		return 0;
	else if (q->qtail > q->qhead)
		return (q->qtail - q->qhead);
	else
		return (q->qtail + q->qcapacity) - q->qhead;
}


int is_qempty(stmyqueue* q)
{
	if (q->qhead == q->qtail)
		return 1;
	else
		return 0;
}


int qpops(stmyqueue* q, int popsize)
{
	if (popsize < 0 || !q->qdata)
		return -99;
	if (is_qempty(q))
		return -1;
	int count = q->qdatasize < popsize ? q->qdatasize : popsize;
	q->qhead = (q->qhead + count) % q->qcapacity;
	q->qdatasize -= count;
	q->qfreesize += count;
	return count;
}


int dequeues(stmyqueue* q, unsigned char* tgt, int tgtsize)
{
	if (tgtsize < 0 || !q->qdata)
		return -99;
	if (is_qempty(q))
		return -1;
	int count = tgtsize > q->qdatasize ? q->qdatasize : tgtsize;

	if (q->qhead + count < q->qcapacity)
	{//linear
		memcpy(tgt, &(q->qdata[q->qhead + 1]), count);

		q->qhead = (q->qhead + count) % q->qcapacity;
		q->qdatasize -= count;
		q->qfreesize += count;
	}
	else
	{
		int linear_count = q->qcapacity - q->qhead - 1;

		if (linear_count != 0)
		{
			memcpy(tgt, &(q->qdata[q->qhead + 1]), linear_count);

			q->qhead = (q->qhead + linear_count) % q->qcapacity;
			q->qdatasize -= linear_count;
			q->qfreesize += linear_count;
			tgt += linear_count;
		}

		linear_count = count - linear_count;
		memcpy(tgt, &(q->qdata[(q->qhead + 1) % q->qcapacity]), linear_count);
		q->qhead = (q->qhead + linear_count) % q->qcapacity;
		q->qdatasize -= linear_count;
		q->qfreesize += linear_count;
	}
	return count;
}


int qpeeks(stmyqueue* q, unsigned char* tgt, int tgtsize)
{
	if (tgtsize < 0 || !q->qdata)
		return -99;
	if (is_qempty(q))
		return -1;
	int count = tgtsize > q->qdatasize ? q->qdatasize : tgtsize;

	if (q->qhead + count < q->qcapacity)
	{//linear
		memcpy(tgt, &(q->qdata[q->qhead + 1]), count);
	}
	else
	{
		int linear_count = q->qcapacity - q->qhead - 1;
		int next_qhead = q->qhead;
		if (linear_count != 0)
		{
			next_qhead = (q->qhead + linear_count) % q->qcapacity;
			memcpy(tgt, &(q->qdata[q->qhead + 1]), linear_count);
			tgt += linear_count;
		}
		linear_count = count - linear_count;
		memcpy(tgt, &(q->qdata[(next_qhead + 1) % q->qcapacity]), linear_count);
	}
	return count;
}


int qpop(stmyqueue* q)
{
	if (is_qempty(q))
		return -1;
	q->qhead = (q->qhead + 1) % q->qcapacity;
	q->qdatasize--;
	q->qfreesize++;
	return 1;
}


int dequeue(stmyqueue* q, unsigned char* tgt)
{
	if (is_qempty(q))
		return -1;
	q->qhead = (q->qhead + 1) % q->qcapacity;
	*tgt = q->qdata[q->qhead];
	q->qdatasize--;
	q->qfreesize++;
	return 1;
}


int qpeek(stmyqueue* q, unsigned char* tgt, int spos)
{
	if (is_qempty(q))
		return -1;
	*tgt = q->qdata[(q->qhead + 1 + spos) % q->qcapacity];
	return 1;
}


void qprint(stmyqueue* q)
{
	char temp0[MAX_PATH] = { "", };
	memset(temp0, 0, sizeof(temp0));
	for (int i = 0; i < q->qcapacity; i++)
		temp0[i] = '0';
	char temp2[MAX_PATH] = { "", };
	memset(temp2, 0, sizeof(temp2));
	for (int i = 0; i < q->qcapacity; i++)
		if (q->qdata[i] == 0)
			temp2[i] = '0';
		else
			temp2[i] = q->qdata[i];

	unsigned char temp[MAX_PATH] = { "", };
	memset(temp, 0, sizeof(temp));
	for (int i = 0; i < q->qdatasize; i++)
		qpeek(q, &temp[i], i);

	char temp3[MAX_PATH] = { "", };
	sprintf_s(temp3, MAX_PATH, "capacity %d data %d free %d , head %d, tail %d\n%s\n%s\n%s\n", q->qcapacity - 1, q->qdatasize, q->qfreesize, q->qhead, q->qtail, temp0, temp, temp2);
	//::MessageBox(0,temp3,"qprint", MB_TOPMOST);
	MYLOGOLDA(temp3);
}
