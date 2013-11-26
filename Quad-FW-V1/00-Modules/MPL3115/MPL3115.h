#include "System.h"
#include "I2C\I2C.h"
#include "Vector\Vector.h"

//=====================================================
#ifndef __MPL3115_H
#define __MPL3115_H
//=====================================================
#define	MPL_OK		 I2CRC_OK
//---------------------------------
#define MPL_ABSY	 I2CRC_MAX + 1
#define MPL_NRDY	 I2CRC_MAX + 2
#define MPL_NACT	 I2CRC_MAX + 3
#define MPL_NOTINIT	 I2CRC_MAX + 4
//=====================================================

//=====================================================
// MPL Data Sample
//=====================================================
typedef	struct
	{
	ulong	TS;		// Timestamp of the cycle
	//-----------------------------------------------
	// Relative (to Ground) altitude (in meters)
	//-----------------------------------------------
	float	Alt;
	}	MPLSample;

//=====================================================
// MPL functions
//=====================================================
uint	MPLInit(byte OSR);
uint	MPLReset(byte OSR);
uint	MPLSetGround();
//-----------------------------------------------------
// Management information functions
//-----------------------------------------------------
uint	MPLGetSTAT(byte*	 mplSTAT);
uint	MPLGetID(byte*	 mplID);
uint	MPLGetMODE(byte*	 mplMODE);
uint	MPLGetINTSrc(byte*	 mplINTSrc);
uint	MPLGetCTRL1(byte*	 mplCTRL);
uint	MPLGetCTRL2(byte*	 mplCTRL);
uint	MPLGetCTRL3(byte*	 mplCTRL);
uint	MPLGetCTRL4(byte*	 mplCTRL);
uint	MPLGetCTRL5(byte*	 mplCTRL);
//-----------------------------------------------------
// Synchronous interface
//-----------------------------------------------------
uint	MPLReadSample(MPLSample* pSample);

//-----------------------------------------------------
// Asynchronous interface
//-----------------------------------------------------
uint	MPLAsyncStart();
uint	MPLAsyncStop();

uint	MPLAsyncReadIfReady(MPLSample* pSample);
uint	MPLAsyncReadWhenReady(MPLSample* pSample);

static inline uint	MPLAsyncRead(MPLSample* pSample)
	{ return MPLAsyncReadIfReady(pSample); }

//=====================================================
#endif
