/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncRateCtrl.h
    \brief    Rate control manager class
*/

#ifndef __TENCRATECTRL__
#define __TENCRATECTRL__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComDataCU.h"

#include <vector>
#include <algorithm>
#include <deque>

using namespace std;

//! \ingroup TLibEncoder
//! \{

#include "../TLibEncoder/TEncCfg.h"
#include <list>
#include <cassert>

const Int g_RCInvalidQPValue = -999;
const Int g_RCSmoothWindowSize = 40;
const Int g_RCMaxPicListSize = 32;
const Double g_RCWeightPicTargetBitInGOP    = 0.9;
const Double g_RCWeightPicRargetBitInBuffer = 1.0 - g_RCWeightPicTargetBitInGOP;
const Int g_RCIterationNum = 20;
const Double g_RCWeightHistoryLambda = 0.5;
const Double g_RCWeightCurrentLambda = 1.0 - g_RCWeightHistoryLambda;
const Int g_RCLCUSmoothWindowSize = 4;
const Double g_RCAlphaMinValue = 0.05;
const Double g_RCAlphaMaxValue = 500.0;
const Double g_RCBetaMinValue  = -3.0;
const Double g_RCBetaMaxValue  = -0.1;

#define ALPHA     6.7542;
#define BETA1     1.2517
#define BETA2     1.7860

struct TRCLCU
{
  Int m_actualBits;
  Int m_QP;     // QP of skip mode is set to g_RCInvalidQPValue
  Int m_targetBits;
  Double m_lambda;
  Double m_bitWeight;
  Int m_numberOfPixel;//图像边界可能LCU只有一部分，所以像素个数不同
  Double m_costIntra;
  Int m_targetBitsLeft;
};

struct TRCParameter
{
  Double m_alpha;
  Double m_beta;
};


class LeastSquare{
public:
	double a, b;
	deque<double> x;
	deque<double> y;
	int num;
	LeastSquare()
	{
		a = 1.0;
		b = 0;
		num = 5;
	}
	void update(double xx, double yy)
	{
		assert(x.size() == y.size());
		if (x.size() >= num)
		{
			x.pop_front();
			y.pop_front();
		}

		x.push_back(xx);
		y.push_back(yy);

		double t1 = 0, t2 = 0, t3 = 0, t4 = 0;
		for (int i = 0; i<x.size(); ++i)
		{
			t1 += x[i] * x[i];
			t2 += x[i];
			t3 += x[i] * y[i];
			t4 += y[i];
		}
		a = (t3*x.size() - t2*t4) / (t1*x.size() - t2*t2);
		b = (t1*t4 - t2*t3) / (t1*x.size() - t2*t2);
		cout << endl << a << " " << b << endl;
	}

	double getY(const double x) const
	{

		return a*x + b;
	}

	void print() const
	{
		cout << "y = " << a << "x + " << b << "\n";
	}

};


class TEncRCSeq
{
public:
  TEncRCSeq();
  ~TEncRCSeq();

public:
  Void create( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int numberOfLevel, Bool useLCUSeparateModel, Int adaptiveBit );
  Void destroy();
  Void initBitsRatio( Int bitsRatio[] );
  Void initGOPID2Level( Int GOPID2Level[] );
  Void initPicPara( TRCParameter* picPara  = NULL );    // NULL to initial with default value
  Void initLCUPara( TRCParameter** LCUPara = NULL );    // NULL to initial with default value
  Void updateAfterPic ( Int bits );
  Void setAllBitRatio( Double basicLambda, Double* equaCoeffA, Double* equaCoeffB );

public:
  Int  getTotalFrames()                 { return m_totalFrames; }
  Int  getTargetRate()                  { return m_targetRate; }
  Int  getFrameRate()                   { return m_frameRate; }
  Int  getGOPSize()                     { return m_GOPSize; }
  Int  getPicWidth()                    { return m_picWidth; }
  Int  getPicHeight()                   { return m_picHeight; }
  Int  getLCUWidth()                    { return m_LCUWidth; }
  Int  getLCUHeight()                   { return m_LCUHeight; }
  Int  getNumberOfLevel()               { return m_numberOfLevel; }
  Int  getAverageBits()                 { return m_averageBits; }
  Int  getLeftAverageBits()             { assert( m_framesLeft > 0 ); return (Int)(m_bitsLeft / m_framesLeft); }
  Bool getUseLCUSeparateModel()         { return m_useLCUSeparateModel; }

  Int  getNumPixel()                    { return m_numberOfPixel; }
  Int64  getTargetBits()                { return m_targetBits; }
  Int  getNumberOfLCU()                 { return m_numberOfLCU; }
  Int* getBitRatio()                    { return m_bitsRatio; }
  Int  getBitRatio( Int idx )           { assert( idx<m_GOPSize); return m_bitsRatio[idx]; }
  Int* getGOPID2Level()                 { return m_GOPID2Level; }
  Int  getGOPID2Level( Int ID )         { assert( ID < m_GOPSize ); return m_GOPID2Level[ID]; }
  TRCParameter*  getPicPara()                                   { return m_picPara; }
  TRCParameter   getPicPara( Int level )                        { assert( level < m_numberOfLevel ); return m_picPara[level]; }
  Void           setPicPara( Int level, TRCParameter para )     { assert( level < m_numberOfLevel ); m_picPara[level] = para; }
  TRCParameter** getLCUPara()                                   { return m_LCUPara; }
  TRCParameter*  getLCUPara( Int level )                        { assert( level < m_numberOfLevel ); return m_LCUPara[level]; }
  TRCParameter   getLCUPara( Int level, Int LCUIdx )            { assert( LCUIdx  < m_numberOfLCU ); return getLCUPara(level)[LCUIdx]; }
  Void           setLCUPara( Int level, Int LCUIdx, TRCParameter para ) { assert( level < m_numberOfLevel ); assert( LCUIdx  < m_numberOfLCU ); m_LCUPara[level][LCUIdx] = para; }

  Int  getFramesLeft()                  { return m_framesLeft; }
  Int64  getBitsLeft()                  { return m_bitsLeft; }

  Double getSeqBpp()                    { return m_seqTargetBpp; }
  Double getAlphaUpdate()               { return m_alphaUpdate; }
  Double getBetaUpdate()                { return m_betaUpdate; }

  Int    getAdaptiveBits()              { return m_adaptiveBit;  }
  Double getLastLambda()                { return m_lastLambda;   }
  Void   setLastLambda( Double lamdba ) { m_lastLambda = lamdba; }


public:
	Int64 m_windowsBits;
	Int m_windowsL;
	Int m_leftFrames;
	LeastSquare least2x;
	
	Double m_complex;
	Int m_num;
	Double m_last_complex;

private:
  Int m_totalFrames;
  Int m_targetRate;
  Int m_frameRate;
  Int m_GOPSize;
  Int m_picWidth;
  Int m_picHeight;
  Int m_LCUWidth;
  Int m_LCUHeight;
  Int m_numberOfLevel;
  Int m_averageBits;

  Int m_numberOfPixel;
  Int64 m_targetBits;
  Int m_numberOfLCU;
  Int* m_bitsRatio;
  Int* m_GOPID2Level;
  TRCParameter*  m_picPara;
  TRCParameter** m_LCUPara;  //这个变量第一维是图像在GOP中的level，第二维是LCU序号


  
  Double m_seqTargetBpp;
  Double m_alphaUpdate;
  Double m_betaUpdate;
  Bool m_useLCUSeparateModel;


  Int m_adaptiveBit;
  Double m_lastLambda;  //每次framelevel为1时更新
public:
  Int m_framesLeft;//序列的剩余帧数
 Int64 m_bitsLeft;//序列的剩余比特数
};

class TEncRCGOP
{
public:
  TEncRCGOP();
  ~TEncRCGOP();

public:
  Void create( TEncRCSeq* encRCSeq, Int numPic );
  Void destroy();
  Void updateAfterPicture( Int bitsCost );

private:
  Int  xEstGOPTargetBits( TEncRCSeq* encRCSeq, Int GOPSize );
  Void   xCalEquaCoeff( TEncRCSeq* encRCSeq, Double* lambdaRatio, Double* equaCoeffA, Double* equaCoeffB, Int GOPSize );
  Double xSolveEqua( Double targetBpp, Double* equaCoeffA, Double* equaCoeffB, Int GOPSize );

public:
  TEncRCSeq* getEncRCSeq()        { return m_encRCSeq; }
  Int  getNumPic()                { return m_numPic;}
  Int  getTargetBits()            { return m_targetBits; }
  Int  getPicLeft()               { return m_picLeft; }
  Int  getBitsLeft()              { return m_bitsLeft; }
  Int  getTargetBitInGOP( Int i ) { return m_picTargetBitInGOP[i]; }

private:
  TEncRCSeq* m_encRCSeq;
  Int* m_picTargetBitInGOP;//GOP中每帧图像的目标比特数
  Int m_numPic;//当前GOP的图像帧数
  Int m_targetBits;//当前GOP的目标比特数
  Int m_picLeft;//尚未编码的图像数
  Int m_bitsLeft;//剩余比特数
};

class TEncRCPic
{
public:
  TEncRCPic();
  ~TEncRCPic();

public:
  Void create( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP, Int frameLevel, list<TEncRCPic*>& listPreviousPictures );
  Void destroy();

  Int    estimatePicQP    ( Double lambda, list<TEncRCPic*>& listPreviousPictures );
  Int    getRefineBitsForIntra(Int orgBits);
  Double calculateLambdaIntra(Double alpha, Double beta, Double MADPerPixel, Double bitsPerPixel);
  Double estimatePicLambda( list<TEncRCPic*>& listPreviousPictures, SliceType eSliceType);

  Void   updateAlphaBetaIntra(Double *alpha, Double *beta);

  Double getLCUTargetBpp(SliceType eSliceType);
  Double getLCUEstLambdaAndQP(Double bpp, Int clipPicQP, Int *estQP);
  Double getLCUEstLambda( Double bpp );
  Int    getLCUEstQP( Double lambda, Int clipPicQP );

  Void updateAfterLCU( Int LCUIdx, Int bits, Int QP, Double lambda, Bool updateLCUParameter = true );
  Void updateAfterPicture( Int actualHeaderBits, Int actualTotalBits, Double averageQP, Double averageLambda, SliceType eSliceType);

  Void addToPictureLsit( list<TEncRCPic*>& listPreviousPictures );
  Double calAverageQP();
  Double calAverageLambda();

private:
  Int xEstPicTargetBits( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP );
  Int xEstPicHeaderBits( list<TEncRCPic*>& listPreviousPictures, Int frameLevel );

public:
  TEncRCSeq*      getRCSequence()                         { return m_encRCSeq; }
  TEncRCGOP*      getRCGOP()                              { return m_encRCGOP; }

  Int  getFrameLevel()                                    { return m_frameLevel; }
  Int  getNumberOfPixel()                                 { return m_numberOfPixel; }
  Int  getNumberOfLCU()                                   { return m_numberOfLCU; }
  Int  getTargetBits()                                    { return m_targetBits; }
  Int  getEstHeaderBits()                                 { return m_estHeaderBits; }
  Int  getLCULeft()                                       { return m_LCULeft; }
  Int  getBitsLeft()                                      { return m_bitsLeft; }
  Int  getPixelsLeft()                                    { return m_pixelsLeft; }
  Int  getBitsCoded()                                     { return m_targetBits - m_estHeaderBits - m_bitsLeft; }
  Int  getLCUCoded()                                      { return m_numberOfLCU - m_LCULeft; }
  TRCLCU* getLCU()                                        { return m_LCUs; }
  TRCLCU& getLCU( Int LCUIdx )                            { return m_LCUs[LCUIdx]; }
  Int  getPicActualHeaderBits()                           { return m_picActualHeaderBits; }
  Void setTargetBits( Int bits )                          { m_targetBits = bits; m_bitsLeft = bits;}
  Void setTotalIntraCost(Double cost)                     { m_totalCostIntra = cost; }
  Void getLCUInitTargetBits();

  Int  getPicActualBits()                                 { return m_picActualBits; }
  Int  getPicActualQP()                                   { return m_picQP; }
  Double getPicActualLambda()                             { return m_picLambda; }
  Int  getPicEstQP()                                      { return m_estPicQP; }
  Void setPicEstQP( Int QP )                              { m_estPicQP = QP; }
  Double getPicEstLambda()                                { return m_estPicLambda; }
  Void setPicEstLambda( Double lambda )                   { m_picLambda = lambda; }

private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;

  Int m_frameLevel;
  Int m_numberOfPixel;
  Int m_numberOfLCU;
  Int m_targetBits;
  Int m_estHeaderBits;
  Int m_estPicQP;
  Double m_estPicLambda;

  Int m_LCULeft;
  Int m_bitsLeft;
  Int m_pixelsLeft;

  TRCLCU* m_LCUs;
  Int m_picActualHeaderBits;    //在TEncRCPic::updateAfterPicture中更新 only SH and potential APS
  Double m_totalCostIntra;
  Double m_remainingCostIntra;
  Int m_picActualBits;          //在TEncRCPic::updateAfterPicture中更新 the whole picture, including header
  Int m_picQP;                  // in integer form
  Double m_picLambda;
};

class TEncRateCtrl
{
public:
  TEncRateCtrl();
  ~TEncRateCtrl();

public:
  Void init( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int keepHierBits, Bool useLCUSeparateModel, GOPEntry GOPList[MAX_GOP] );
  Void destroy();
  Void initRCPic( Int frameLevel );
  Void initRCGOP( Int numberOfPictures );
  Void destroyRCGOP();

public:
  Void       setRCQP ( Int QP ) { m_RCQP = QP;   }
  Int        getRCQP ()         { return m_RCQP; }
  TEncRCSeq* getRCSeq()          { assert ( m_encRCSeq != NULL ); return m_encRCSeq; }
  TEncRCGOP* getRCGOP()          { assert ( m_encRCGOP != NULL ); return m_encRCGOP; }
  TEncRCPic* getRCPic()          { assert ( m_encRCPic != NULL ); return m_encRCPic; }
  list<TEncRCPic*>& getPicList() { return m_listRCPictures; }


private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;  //每压缩一个GOP前，建立TEncRCGOP对象，并做初始化，然后对该GOP编码，销毁该TEncRCGOP对象
  TEncRCPic* m_encRCPic;
  list<TEncRCPic*> m_listRCPictures;//保存最近编码的最多32帧图像的RC信息
  Int        m_RCQP;


};

#endif


