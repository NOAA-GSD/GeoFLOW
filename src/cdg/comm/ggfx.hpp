//==================================================================================
// Module       : ggfx.hpp
// Date         : 5/9/18 (DLR)
// Description  : Encapsulates the methods and data associated with
//                a geometry--free global exchange (GeoFLOW Geometry-Free eXchange)
//                operator
// Copyright    : Copyright 2018. Colorado State University. All rights reserved
// Derived From : none.
//==================================================================================

#if !defined(GGFX_HPP)
#define GGFX_HPP

#include <sstream>
#include "gtvector.hpp"
#include "gtmatrix.hpp"
#include "gcomm.hpp"

#define GGFX_TRACE_OUTPUT

enum GGFX_Timer_t {GGFXTR_OP, GGFXTR_EXCH, GGFXTR_LCOMB, GGFXTR_GCOMB, GGFXTR_GCOMBA, GGFXTR_GCOMBB, GGFXTR_GCOMBC};

// GGFX reduction operation defs:
#if !defined(GGFX_OP_DEF)
#define GGFX_OP_DEF
enum GGFX_OP {GGFX_OP_SUM=0, GGFX_OP_PROD, GGFX_OP_MAX, GGFX_OP_MIN, GGFX_OP_SMOOTH};
#endif


class GGFX
{
public:
                      GGFX(GC_COMM icomm=GC_COMM_WORLD);
                     ~GGFX();
                      GGFX(const GGFX &a);
                      GGFX  &operator=(const GGFX &);
                   

                      GBOOL    init(GNIDBuffer &glob_index);
template<typename T>  GBOOL    doOp(T *&u, GSIZET nu,  GGFX_OP op);
template<typename T>  GBOOL    doOp(GTVector<T> &u,  GGFX_OP op);
template<typename T>  GBOOL    doOp(T *&u, GSIZET nu,  GSIZET *iind, GSIZET nind, GGFX_OP op);
template<typename T>  GBOOL    doOp(GTVector<T> &u,  GSZBuffer &ind, GGFX_OP op);
                      GC_COMM  getComm() { return comm_; }

                      GDOUBLE  getTimes(GGFX_Timer_t type);
                      void     resetTimes();

private:
// Private methods:
 
// Init-specific methods:
                      GBOOL initSort(GNIDBuffer  &glob_index, GIBuffer &iOpL2RTasks, 
                                    GSZMatrix &iOpL2RIndices, GSZBuffer &nOpL2RIndices,
                                    GSZMatrix &iOpR2LIndices, GSZBuffer &nOpR2LIndices,
                                    GSZMatrix &iOpL2LIndices, GSZBuffer &nOpL2LIndices);
                      GBOOL binSort(GNIDBuffer &glob_index   , GIMatrix    &gBinMat, 
                                    GINT       &nlocfilledbins,
                                    GINT       &maxfilledbins, GINT       &maxbinmem,
                                    GNIDMatrix &gBinBdy      , GNIDBuffer &locwork);
                      GBOOL doSendRecvWork(GNIDBuffer &, GNIDMatrix &, GIBuffer &, GNIDMatrix &, GIBuffer &, GNIDMatrix &);
                      GBOOL binWorkFill(GNIDBuffer &, GNIDMatrix &, GNIDMatrix &, GIBuffer &);
                      GBOOL createWorkBuffs(GIMatrix &, GINT, GIBuffer &, GNIDMatrix &, GIBuffer &, GNIDMatrix &);
                      GBOOL doCommonNodeSort(GNIDBuffer &, GNIDMatrix &, GIBuffer &, GIBuffer &, GNIDMatrix &);
                      GBOOL extractOpData(GNIDBuffer &glob_index, GNIDMatrix &mySharedNodes, GIBuffer &iOpL2RTasks, 
                                          GSZMatrix &iOpL2RIndices, GSZBuffer &nOpL2RIndices, 
                                          GSZMatrix &iOpR2LIndices, GSZBuffer &nOpR2LIndices, 
                                          GSZMatrix &iOpL2LIndices, GSZBuffer &nOpL2LIndices);
 

                     // doOp-specific methods:
template<typename T> GBOOL localGS(T *&u, GSIZET  nu, GSZMatrix  &ilocal, GSZBuffer &nlocal, GGFX_OP op, GDOUBLE *qop=NULLPTR);
template<typename T> GBOOL localGS(T *&u, GSIZET  nu, GSIZET *&iind, GSIZET nind, GSZMatrix  &ilocal, GSZBuffer &nlocal, GGFX_OP op, GDOUBLE *qop=NULLPTR);
template<typename T> GBOOL dataExchange(T *&u, GSIZET nu );
template<typename T> GBOOL dataExchange(T *&send_data, GSIZET nu, GSIZET *&iind, GSIZET nind );

                     // misc. methods:
                     void initMult(); // initialize multiplicity for smoothing

// Private data:
GBOOL              bBinSorted_   ;  // have local nodes been bin-sorted?      
GBOOL              bInit_        ;  // has operator initialization occurred?
GBOOL              bMultReq_     ;  // is (inverse) multiplicity required (e.g. for smoothing)?
GC_COMM            comm_         ;  // communicator
GINT               nprocs_       ;  // number of tasks/ranks
GINT               rank_         ;  // this rank
GSIZET             nglob_index_  ;  // number of indices in call to Init
GNODEID            maxNodeVal_   ;  // Node value dynamical range
GNODEID            maxNodes_     ;  // total number of nodes distributed among all procs
GNIDMatrix         gBinBdy_      ;  // global bin bdy ranges for each task [0,nporocs_-1]
GIBuffer           iOpL2RTasks_  ;  // task ids to send op data to, and recv from
GSZMatrix          iOpL2RIndices_;  // matrix with send/recv data for each off-task shared node
GSZMatrix          iOpR2LIndices_;  // matrix with send/recv data for each off-task shared node
GSZBuffer          nOpL2RIndices_;  // number shared nodes to snd/rcv for each task
GSZBuffer          nOpR2LIndices_;  // number shared nodes to snd/rcv for each task
GSZMatrix          iOpL2LIndices_;  // matrix with local indices pointing to shared nodes
GSZBuffer          nOpL2LIndices_;  // number valid columns in each row of iOpLoIndices_
GDMatrix           sendBuff_     ;  // send buffer
GDMatrix           recvBuff_     ;  // recv buffer
GFVector           imult_        ;  // inverse of multiplicity matrix (for H1-smoothing)

// Timer data:
GDVector           timer_data_   ;  // cumlative times for each timer

};

#include "ggfx.ipp"

#endif

