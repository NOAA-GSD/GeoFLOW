//==================================================================================
// Module       : gtest_derivs.cpp 
// Date         : 10/21/20 (DLR)
// Description  : Test of derivative operations
//                improvement
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : 
//==================================================================================


#include "gtypes.h"
#include "gcomm.hpp"
#include "ggfx.hpp"
#include "gllbasis.hpp"
#include "gmass.hpp"
#include "gmorton_keygen.hpp"
#include "ggrid_factory.hpp"
#include "gspecterrain_factory.hpp"
#include "pdeint/observer_base.hpp"
#include "pdeint/observer_factory.hpp"
#include "tbox/pio.hpp"
#include "tbox/property_tree.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"
#include "tbox/tracer.hpp"

#include <cassert>
//#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <unistd.h>


using namespace geoflow::tbox;
using namespace std;


struct TypePack {
        using State      = GTVector<GTVector<GFTYPE>*>;
        using StateComp  = GTVector<GFTYPE>;
        using StateInfo  = GStateInfo;
        using Grid       = GGrid<TypePack>;
        using GridBox    = GGridBox<TypePack>;
        using GridIcos   = GGridIcos<TypePack>;
        using Mass       = GMass<TypePack>;
        using Ftype      = GFTYPE;
        using Derivative = State;
        using Time       = Ftype;
        using CompDesc   = GTVector<GStateCompType>;
        using Jacobian   = State;
        using Size       = GSIZET;
        using EqnBase    = EquationBase<TypePack>;     // Equation Base type
        using EqnBasePtr = std::shared_ptr<EqnBase>;   // Equation Base ptr
        using IBdyVol    = GTVector<GSIZET>;
        using TBdyVol    = GTVector<GBdyType>;
        using Operator   = GHelmholtz<TypePack>;
        using GElemList  = GTVector<GElem_base*>;
        using Preconditioner = GHelmholtz<TypePack>;
        using ConnectivityOp = GGFX<Ftype>;
        using FilterBasePtr  = std::shared_ptr<FilterBase<TypePack>>;
        using FilterList     = std::vector<FilterBasePtr>;

};
using Types         = TypePack;                   // Define grid types used
using Ftype         = typename Types::Ftype;
using Grid          = typename Types::Grid;
using GridBox       = typename Types::GridBox;
using State         = typename Types::State;
using StateComp     = typename Types::StateComp;
using IOBaseType    = IOBase<Types>;              // IO Base type
using IOBasePtr     = std::shared_ptr<IOBaseType>;// IO Base ptr
using ObsTraitsType = ObserverBase<Types>::Traits;


void init_ggfx(PropertyTree &ptree, Grid &grid, GGFX<Ftype> &ggfx);
void do_terrain(const PropertyTree &ptree, Grid &grid);

Grid      *grid_ = NULLPTR;
IOBasePtr  pIO_  = NULLPTR; // ptr to IOBase operator

GINT szMatCache_ = _G_MAT_CACHE_SIZE;
GINT szVecCache_ = _G_VEC_CACHE_SIZE;


enum    TErr   {ERR_INF=0, ERR_L2};
#define NMETH 2


int main(int argc, char **argv)
{
	GEOFLOW_TRACE_INITIALIZE();

    GINT    errcode=0 ;
    TErr    ierr=ERR_INF;  // which error to use for success/fail
    GINT    nc=GDIM; // no. coords
    GSIZET  ibeg, iend;
    GFTYPE  eps=100.0*std::numeric_limits<GFTYPE>::epsilon();
    GFTYPE  dnorm[2], gn, xn;
    GString sgrid;// name of JSON grid object to use
    GString sterrain;
    GTVector<GINT>
            pvec;
    std::vector<GINT> 
            pstd(GDIM);  
    GC_COMM comm = GC_COMM_WORLD;

    // Initialize comm:
    GComm::InitComm(&argc, &argv);
    GINT myrank  = GComm::WorldRank();
    GINT nprocs  = GComm::WorldSize();
    pio::initialize(myrank);
    GEOFLOW_TRACE();

    // Read main prop tree; may ovewrite with
    // certain command line args:
    PropertyTree ptree;     // main prop tree
    PropertyTree gridptree; // grid prop tree
    PropertyTree polyptree; // poly_test prop tree

    /////////////////////////////////////////////////////////////////
    /////////////////////// Initialize system ///////////////////////
    /////////////////////////////////////////////////////////////////
    ptree.load_file("deriv_test.jsn");       // main param file structure

    // Create other prop trees for various objects:
    sgrid       = ptree.getValue<GString>("grid_type");
    sterrain    = ptree.getValue<GString>("terrain_type","");
    pstd        = ptree.getArray<GINT>("exp_order");
    gridptree   = ptree.getPropertyTree(sgrid);
    polyptree   = ptree.getPropertyTree("poly_test");
    nc          = GDIM;
 
    assert(pstd.size() >= GDIM); 

    pvec.resize(pstd.size()); pvec = pstd; pvec.range(0,GDIM-1);

    // Create basis:
    GTVector<GNBasis<GCTYPE,GFTYPE>*> gbasis(GDIM);
    for ( auto k=0; k<GDIM; k++ ) {
      gbasis [k] = new GLLBasis<GCTYPE,GFTYPE>(pstd[k]);
    }
    
    // Create grid:
    ObsTraitsType binobstraits;
    grid_ = GGridFactory<Types>::build(ptree, gbasis, pIO_, binobstraits, comm);

    // Initialize gather/scatter operator:
    GGFX<Ftype> ggfx;
    init_ggfx(ptree, *grid_, ggfx);
    grid_->set_ggfx(ggfx);

    GridBox  *box   = dynamic_cast <GridBox*>(grid_);
    assert(box && "Must use a box grid");

    GTPoint<GFTYPE> dP(GDIM), P0(GDIM), P1(GDIM);
    P0 = box->getP0();
    P1 = box->getP1();
    dP = P1 - P0; dP.x3 = dP.x3 == 0.0 ? 1.0 : dP.x3;

    // Do terrain, if necessary:
    if ( sterrain != "" ) do_terrain(ptree, *grid_);

    /////////////////////////////////////////////////////////////////
    ////////////////////// Allocate arrays //////////////////////////
    /////////////////////////////////////////////////////////////////

    // Create state and tmp space:
    State     utmp (3);
    State     da   (nc);
    State     du   (nc);
    StateComp diff, dunew, duold, u;
    
    for ( auto j=0; j<utmp .size(); j++ ) utmp [j] = new StateComp(grid_->size());
    for ( auto j=0; j<da   .size(); j++ ) da   [j] = new StateComp(grid_->size());
    u    .resize(grid_->size());
    diff .resize(grid_->size());
    dunew.resize(grid_->size());
    duold.resize(grid_->size());
    du = NULLPTR;

    /////////////////////////////////////////////////////////////////
    ////////////////////// Compute solutions/////////////////////////
    /////////////////////////////////////////////////////////////////

    // Initialize u: set p, q, r exponents;
    //   u = x^p y^q z^r:
    std::vector<GFTYPE> 
            vpoly = polyptree.getArray<GFTYPE>("poly");
    
    GINT    ncyc  = polyptree.getValue<GINT>("ncycles",100);
    GINT    idir  = polyptree.getValue<GINT>("idir",2);
            ierr  = (TErr)polyptree.getValue<GINT>("ierr",ERR_INF);
    GFTYPE  p, q, r, x, y, z;
    GFTYPE  ix, iy, iz;
    GTVector<GTVector<GFTYPE>> 
           *xnodes = &grid_->xNodes();   

    assert(vpoly.size() >= GDIM); 
    assert(idir >= 1 && idir <= GDIM);

    p = vpoly[0]; q = vpoly[1]; r = GDIM == 3 ? vpoly[2] : 0.0;
    ix = 1.0/dP.x1; iy = 1.0/dP.x2; iz = 1.0 / dP.x3;
//  ix = 1.0      ; iy = 1.0      ; iz = 1.0        ;
    // Set scalar field, and analytic derivative:
    for ( auto j=0; j<(*xnodes)[0].size(); j++ ) {
      x = (*xnodes)[0][j]; 
      y = (*xnodes)[1][j]; 
      z = GDIM == 3 ? (*xnodes)[2][j] : 1.0; 
        u     [j] = pow(x*ix,p)*pow(y*ix,q)*pow(z*iz,r);
      (*da[0])[j] = p==0 ? 0.0 : ix*p*pow(x*ix,p-1)*pow(y*iy,q)*pow(z*iz,r);
      (*da[1])[j] = q==0 ? 0.0 : iy*q*pow(x*ix,p)*pow(y*iy,q-1)*pow(z*iz,r);
      if ( GDIM == 3 ) (*da[2])[j] = 
                    r==0 ? 0.0 : iz*r*pow(x*ix,p)*pow(y*iy,q)*pow(z*iz,r-1);
    } // end, loop over grid points

    // dnorm[0] = (u.gmax() - u.gmin()) / ( (*xnodes)[idir-1].gmax() - (*xnodes)[idir-1].gmin() );
 //    dnorm[0] = (u.gamax() - u.gamin()) / grid_->minnodedist();
       dnorm[0] = 1.0 / grid_->minnodedist();
if ( myrank == 0 ) 
cout << "main: dnorm[0]=" << dnorm[0] << endl;

    // Compute numerical derivs of u in each direction, using
    // different methods:
    grid_->set_derivtype(GGrid<Types>::GDV_VARP); // variable order
    GEOFLOW_TRACE_START("old_deriv");
    for ( auto n=0; n<ncyc; n++ ) {
       grid_->deriv(u, idir, *utmp[0], duold);
    }
    GEOFLOW_TRACE_STOP();

    grid_->set_derivtype(GGrid<Types>::GDV_CONSTP); // const order
    GEOFLOW_TRACE_START("new_deriv");
    for ( auto n=0; n<ncyc; n++ ) {
       grid_->deriv(u, idir, *utmp[0], dunew);
    }
    GEOFLOW_TRACE_STOP();

    //cout << "da_x  =" << *da   [idir-1] << endl;
    //cout << "dnew_x=" <<  dunew << endl;

    // Find inf-norm and L2-norm errors for each method::
    GTMatrix<GFTYPE> errs(NMETH,2); // for each method, Linf and L2 errs
    StateComp        lnorm(2), gnorm(2);
    std::string      smethod[NMETH] = {"old", "new"};
    GTVector<GString> serr(2);

    /////////////////////////////////////////////////////////////////
    //////////////////////// Compute Errors /////////////////////////
    /////////////////////////////////////////////////////////////////
    serr[0] = "Inf"; serr[1] = "L2";
    du[0] = &duold; du[1] = &dunew;
    for ( auto n=0; n<NMETH; n++ ) { // over old and new methods
      diff     = (*da[idir-1]) - (*du[n]);
     *utmp [0] = diff;                   // for inf-norm
     *utmp [1] = diff; utmp[1]->rpow(2); // for L2 norm

#if 0
      xn    = da[idir-1]->gamax();
      dnorm[0] = gn < eps ? 1.0 : gn;
#endif

      lnorm[0] = utmp[0]->amax();
      GComm::Allreduce(lnorm.data(), gnorm.data(), 1, T2GCDatatype<GFTYPE>(), GC_OP_MAX, comm);
      xn       = grid_->integrate(*utmp[1],*utmp[2]); // int (u-ua)^2 dV
      dnorm[1] = xn < eps ? 1.0 : dnorm[0]*dnorm[0]*grid_->volume();
      gnorm[1] = xn;
if ( myrank == 0 && n == 0 ) {
cout << " ............rel Inf diff : " << gnorm[0] << endl; 
cout << " ............rel Inf norm : " << dnorm[0] << endl; 
cout << " ............rel L2 diff  : " << sqrt(gnorm[1]) << endl; 
cout << " ............rel L2 norm  : " << sqrt(dnorm[1]) << endl; 
}

      for ( auto j=0; j<2; j++ ) errs(n,j) = gnorm[j]/dnorm[j]; // errors
      if ( myrank == 0 ) {
        if ( errs(n,ierr) > eps ) {
          std::cout << "main: ---------------------------derivative FAILED: " << serr[ierr] << " norm=" << errs(n,ierr) << " : direction=" << idir << " method: " << smethod[n]  << std::endl;
          errcode += 1;
        } else {
          std::cout << "main: ---------------------------derivative OK: " << serr[ierr] << " norm=" << errs(n,ierr) << " : direction=" << idir << " method: " << smethod[n] << std::endl;
          errcode += 0;
        }
      }

    } // end, new-old method loop


    // Print convergence data to file:
    std::ifstream itst;
    std::ofstream ios;
    itst.open("deriv_err.txt");
    ios.open("deriv_err.txt",std::ios_base::app);

    if ( myrank == 0 ) {
      // Write header, if required:
      if ( itst.peek() == std::ofstream::traits_type::eof() ) {
      ios << "#  idir   p      num_elems  ncyc  inf_err_old  L2_err_old   inf_err_new  L2_err_new  " << std::endl;
      }
      itst.close();

      ios << idir << "  " << pvec[0] ;
          for ( auto j=1; j<GDIM; j++ ) ios << " " <<  pvec[j]; 
      ios << "  " << grid_->ngelems() 
          << "  " << ncyc
          << "  " << errs(0,0) << "  " << errs(0,1) 
          << "  " << errs(1,0) << "  " << errs(1,1) 
          << std::endl;
      ios.close();
    }
 
    pio::finalize();
    GEOFLOW_TRACE_STOP();
    GComm::TermComm();
    GEOFLOW_TRACE_FINALIZE();

    return( errcode );

} // end, main


//**********************************************************************************
//**********************************************************************************
void do_terrain(const PropertyTree &ptree, Grid &grid) {
    GEOFLOW_TRACE();
    GBOOL bret, bterr;
    GINT iret, nc;
    State xb, tmp, utmp;

    nc = grid.xNodes().size();
    xb.resize(nc);

    // Cannot use utmp_ here because it
    // may not have been allocated yet, so
    // use local variable:
    utmp.resize(13 + nc);
    tmp.resize(utmp.size() - nc);
    for (auto j = 0; j < utmp.size(); j++) utmp[j] = new GTVector<Ftype>(grid_->ndof());

    // Set terrain & tmp arrays from tmp array pool:
    for (auto j = 0; j < nc; j++) xb[j] = utmp[j];
    for (auto j = 0; j < tmp.size(); j++) tmp[j] = utmp[j + nc];

    bret = GSpecTerrainFactory<Types>::spec(ptree, grid, tmp, xb, bterr);
    assert(bret);

    if (bterr) grid.add_terrain(xb, tmp);

    for (auto j = 0; j < utmp.size(); j++) delete utmp[j];

}  // end of method do_terrain


//**********************************************************************************
//**********************************************************************************
void init_ggfx(PropertyTree& ptree, Grid& grid, GGFX<Ftype>& ggfx)
{
  const auto ndof = grid_->ndof();
  std::vector<std::array<Ftype,GDIM>> xyz(ndof);
  for(std::size_t i = 0; i < ndof; i++){
          for(std::size_t d = 0; d < GDIM; d++){
                  xyz[i][d] = grid.xNodes()[d][i];
          }
  }

  // Create GGFX
  ggfx.init(0.25*grid.minnodedist(), xyz);

} // end method init_ggfx

