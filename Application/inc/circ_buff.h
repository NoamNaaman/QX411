#ifndef __CIRCULAR_BUFFER_API_H__
#define __CIRCULAR_BUFFER_API_H__

  
/****************************************************************************/
/*				TYPEDEFS										*/
/****************************************************************************/
typedef struct CircularBuffer
{
    unsigned char *pucReadPtr;
    unsigned char *pucWritePtr;
    unsigned char *pucBufferStartPtr;
    unsigned long ulBufferSize;
    unsigned char *pucBufferEndPtr;
		unsigned int  mark;
}tCircularBuffer;

#ifndef TRUE
#define TRUE                    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif

//*****************************************************************************
//
// Define a boolean type, and values for true and false.
//
//*****************************************************************************
typedef unsigned int tboolean;

/****************************************************************************/
/*		        FUNCTION PROTOTYPES							*/
/****************************************************************************/
tCircularBuffer* CreateCircularBuffer(unsigned long ulBufferSize);
void DestroyCircularBuffer(tCircularBuffer *pCircularBuffer);
unsigned char* GetReadPtr(tCircularBuffer *pCircularBuffer);
unsigned char* GetWritePtr(tCircularBuffer *pCircularBuffer);
long FillBuffer(tCircularBuffer *pCircularBuffer,
                       unsigned char *pucBuffer, unsigned int uiBufferSize);
void UpdateReadPtr(tCircularBuffer *pBuffer, unsigned int uiDataSize);
void UpdateWritePtr(tCircularBuffer *pCircularBuffer,
                           unsigned int uiPacketSize);
long ReadBuffer(tCircularBuffer *pCircularBuffer,
                       unsigned char *pucBuffer, unsigned int uiDataSize);

unsigned char PeekByte(tCircularBuffer *pCircularBuffer, unsigned int index);
unsigned short PeekShort(tCircularBuffer *pCircularBuffer, unsigned int index);
	
long PeekBuffer(tCircularBuffer *pCircularBuffer, unsigned char *pucBuffer,
																								unsigned int uiDataSize);
long FillZeroes(tCircularBuffer *pCircularBuffer,
					   unsigned int uiPacketSize);
unsigned int GetBufferSize(tCircularBuffer *pCircularBuffer);
unsigned int GetBufferEmptySize(tCircularBuffer *pCircularBuffer);
tboolean IsBufferEmpty(tCircularBuffer *pCircularbuffer);
tboolean IsBufferSizeFilled(tCircularBuffer *pCircularBuffer,
                             unsigned long ulThresholdHigh);
tboolean IsBufferVacant(tCircularBuffer *pCircularBuffer,
                               unsigned long ulThresholdLow);
tboolean IsBufferOverflow(tCircularBuffer *pCircularBuffer, unsigned long ulSize);

#ifdef __cplusplus
}
#endif

#endif // __CIRCULAR_BUFFER_API_H__

