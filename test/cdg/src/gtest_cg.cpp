//==================================================================================
// Module       : gtest_cg.cpp
// Date         : 3/16/20 (DLR)
// Description  : GeoFLOW test of conjigate gradient operator, 
//                solving
//                    Nabla^2 u = -f, 
//                In a 2D box, where f =  6xy(1-y) - 2x^3
//                on 0 <= x,y <= 1, and u(x,y=0)=u(x,y=1)=0;
//                u(x=0,y)=0; u(x=1,y) = y(1-y). 
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================

#include "gexec.h"
#include "gtypes.h"
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "pdeint/observer_base.hpp"
#include "pdeint/io_base.hpp"
#include "pdeint/observer_factory.hpp"
#include "pdeint/null_observer.hpp"
#include "tbox/property_tree.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"
#include "tbox/error_handler.hpp"
#include "gcomm.hpp"
#include "gmass.hpp"
#include "ggfx.hpp"
#include "gllbasis.hpp"
#include "gmorton_keygen.hpp"
#include "ggrid_box.hpp"
#include "ggrid_factory.hpp"
#include "gio_observer.hpp"
#include "gio.hpp"
#include "gcg.hpp"
#include "ghelmholtz.hpp"
#if defined(_G_USE_GPTL)
  #include "gptl.h"
#endif


using namespace geoflow::pdeint;
using namespace geoflow::tbox;
using namespace std;

struct stStateInfo {
  GINT        sttype  = 1;       // state type index (grid=0 or state=1)
  GINT        gtype   = 0;       // check src/cdg/include/gtypes.h
  GSIZET      index   = 0;       // time index
  GSIZET      nelems  = 0;       // num elems
  GSIZET      cycle   = 0;       // continuous time cycle
  GFTYPE      time    = 0.0;     // state time
  std::vector<GString>
              svars;             // names of state members
  GTVector<GStateCompType>
              icomptype;         // encoding of state component types    
  GTMatrix<GINT>
              porder;            // if ivers=0, is 1 X GDIM; else nelems X GDIM;
  GString     idir;              // input directory
  GString     odir;              // output directory
};


struct MyCGTypes {
        using Types            = MyCGTypes;
        using Operator         = class GHelmholtz;
        using Preconditioner   = LinSolverBase<Types>;
        using State            = GTVector<GTVector<GFTYPE>*>;
        using StateComp        = GTVector<GFTYPE>;
        using Grid             = GGrid;
        using Value            = GFTYPE;
        using ConnectivityOp   = GGFX<GFTYPE>;
};



template< // default template arg types
typename StateType     = GTVector<GTVector<GFTYPE>*>,
typename StateInfoType = stStateInfo,
typename GridType      = GGrid,
typename ValueType     = GFTYPE,
typename DerivType     = StateType,
typename TimeType      = ValueType,
typename CompType      = GTVector<GStateCompType>,
typename JacoType      = StateType,
typename SizeType      = GSIZET 
>
struct EquationTypes {
        using State      = StateType;
        using StateInfo  = StateInfoType;
        using Grid       = GridType;
        using Value      = ValueType;
        using Derivative = DerivType;
        using Time       = TimeType;
        using CompDesc   = CompType;
        using Jacobian   = JacoType;
        using Size       = SizeType;
};
using MyTypes       = EquationTypes<>;           // Define types used
using EqnBase       = EquationBase<MyTypes>;     // Equation Base type
using EqnBasePtr    = std::shared_ptr<EqnBase>;  // Equation Base ptr
using IOBaseType    = IOBase<MyTypes>;           // IO Base type
using IOBasePtr     = std::shared_ptr<IOBaseType>;// IO Base ptr
using Grid          = GGrid;


GGrid        *grid_=NULLPTR;
GC_COMM      comm_=GC_COMM_WORLD ;      // communicator

void init_ggfx(PropertyTree &ptree, Grid &grid, GGFX<GFTYPE> &ggfx);


int main(int argc, char **argv)
{
    GString   serr="main: ";
    GINT      errcode, gerrcode, iret;
    IOBasePtr pIO;         // ptr to IOBase operator
    GTVector<GTVector<GFTYPE>>    *xnodes;
    LinSolverBase<MyCGTypes>::Traits cgtraits;
    GString   snorm;
    std::ifstream itst;
    std::ofstream ios;

    typename ObserverBase<MyTypes>::Traits
                   binobstraits;


    if ( argc > 1 ) {
      cout << "No arguments accepted" << endl;
      exit(1);
    }
    errcode = 0;

    // Initialize comm:
    GComm::InitComm(&argc, &argv);
    mpixx::environment  env(argc,argv); // init GeoFLOW comm
    mpixx::communicator world;
    GlobalManager::initialize(argc,argv);
    GlobalManager::startup();


    GINT myrank  = GComm::WorldRank();
    GINT nprocs  = GComm::WorldSize();

#if defined(_G_USE_GPTL)
    // Set GTPL options:
    GPTLsetoption (GPTLcpu, 1);

    // Initialize GPTL:
    GPTLinitialize();
#endif

    // Get minimal property tree:
    PropertyTree gtree, ptree; 
    GString sgrid;
    GTPoint<GFTYPE> P0, P1, dP;
    std::vector<GINT> pstd(GDIM);

    ptree.load_file("input_cg.jsn");
    sgrid       = ptree.getValue<GString>("grid_type");
    pstd        = ptree.getArray<GINT>("exp_order");
    gtree       = ptree.getPropertyTree(sgrid);

    std::vector<GFTYPE> xyz0 = gtree.getArray<GFTYPE>("xyz0");
    std::vector<GFTYPE> dxyz = gtree.getArray<GFTYPE>("delxyz");
    P0  = xyz0;
    P1  = dxyz;
    P1 += P0;

    assert(P0.x1 == 0.0 && P1.x1 == 1.0
        && P0.x2 == 0.0 && P1.x2 == 1.0 );

    // Create basis:
    GTVector<GNBasis<GCTYPE,GFTYPE>*> gbasis(GDIM);
    for ( GSIZET k=0; k<GDIM; k++ ) {
      gbasis [k] = new GLLBasis<GCTYPE,GFTYPE>(pstd[k]);
    }

    EH_MESSAGE("main: Create grid...");

    // Generate grid:
#if defined(_G_USE_GPTL)
    GPTLstart("gen_grid");
#endif

    ObserverFactory<MyTypes>::get_traits(ptree, "gio_observer", binobstraits);
    grid_ = GGridFactory<MyTypes>::build(ptree, gbasis, pIO, binobstraits, comm_);

#if defined(_G_USE_GPTL)
    GPTLstop("gen_grid");
#endif

    EH_MESSAGE("main: Initialize GGFX operator...");

    // Initialize gather/scatter operator:
    GGFX<GFTYPE>  ggfx;
    init_ggfx(ptree, *grid_, ggfx);

    xnodes = &(grid_->xNodes());

    GTVector<GFTYPE>            f (grid_->ndof());
    GTVector<GFTYPE>            u (grid_->ndof());
    GTVector<GFTYPE>            ua(grid_->ndof());
    GTVector<GFTYPE>            ub(grid_->ndof());
    GTVector<GFTYPE>           *mass_local = grid_->massop().data();
    GTVector<GTVector<GFTYPE>*> utmp(10);

    for ( auto j=0; j<utmp.size(); j++ ) utmp[j] = new GTVector<GFTYPE>(grid_->ndof());

    cgtraits.maxit    = gtree.getValue<GDOUBLE>("maxit");
    cgtraits.tol      = gtree.getValue<GDOUBLE>("tol");
    snorm             = gtree.getValue<GString>("norm_type");
    cgtraits.normtype = LinSolverBase<MyCGTypes>::str2normtype(snorm);

    EH_MESSAGE("main: Initialize CG operator...");

    // Initialize GCG operator:
    GSIZET            niter;
    GFTYPE            eps = 100.0*std::numeric_limits<GFTYPE>::epsilon();
    GFTYPE            err, resmin, resmax, x, y, z;
    GTVector<GFTYPE> *resvec;

    EH_MESSAGE("main: Create Lap op...");
    GHelmholtz       L(*grid_); // Laplacian operator

    EH_MESSAGE("main: Create CG op...");

    GCG<MyCGTypes>   cg(cgtraits, *grid_, ggfx, utmp);

    EH_MESSAGE("main: Check for SPD...");

//L.use_metric(FALSE);

    // Check if operator is SPD:
    GTMatrix<GFTYPE> Hmat (f.size(),f.size());
    f = 0.0;
    for ( auto j=0; j<f.size(); j++ ) {
      f[j] = 1.0;
//    ggfx.doOp(f, GGFX_OP_SUM);
      L.opVec_prod(f, utmp, u);
//    ggfx.doOp(u, GGFX_OP_SUM);
      for ( auto i=0; i<f.size(); i++ ) Hmat(i,j) = u[i];
      f[j] = 0.0;
    }

//cout << "Hmat=" << Hmat << endl;
#if 1
    GSIZET nbad = 0, ngbad;
    for ( auto j=0; j<f.size(); j++ ) {
      for ( auto i=j; i<f.size(); i++ ) {
        if ( !FUZZYEQ(Hmat(i,j), Hmat(j,i), eps) ) {
          cout << "main: (" << i << "," << j << "): H=" << Hmat(i,j) << " H^T=" << Hmat(j,i) << endl;
          nbad++;
        }
      }
    }

    // Accumulate 'bad' entry number:
    GComm::Allreduce(&nbad, &ngbad, 1, T2GCDatatype<GSIZET>() , GC_OP_MAX, comm_);
    if ( ngbad == 0 ) {
      cout << "main: .................................. operator is SPD!" << endl;
    } else {
      cout << "main: .................................. operator NOT SPD!" << endl;
      cout << "main: .................................. ngbad=" << nbad << "; size=" << f.size()*(f.size()+1)/2 << endl;
      errcode = 1;
      goto prerror;
    }
#endif
      

    EH_MESSAGE("main: Set RHS...");

    // Generate smooth RHS, bdyy vectors:
    //    f =  6xy(1-y) - 2x^3

    ub = 0.0;
    for ( auto j=0; j<grid_->ndof(); j++ ) {

      x = (*xnodes)[0][j]; y = (*xnodes)[1][j];
      if ( GDIM > 2 ) z = (*xnodes)[2][j];
      f [j] = 6.0*x*y*(1.0-y) - 2.0*pow(x,3.0);           // RHS
      ua[j] = y*(1.0-y)*pow(x,3.0);                       // analytic solution
      if ( FUZZYEQ(P0.x2,y,eps) || FUZZYEQ(P1.x2,y,eps) ) // N & S bdy
        ub[j] = 0.0; 
      if ( FUZZYEQ(P0.x1,x,eps) ) // W bdy
        ub[j] = 0.0; 
      if ( FUZZYEQ(P1.x1,x,eps) ) // E bdy
        ub[j] = y*(1.0-y);

    }

    // Multiply f by local mass matrix:
    f.pointProd(*mass_local);     // M_L f_L
    f *= -1.0;                    // -f required by discretization

    EH_MESSAGE("main: Solve linear system...");

    // Invert mass operator, solve L u = f for u, 
    // where L is Laplacian operator:
    u = 0.0; // initial guess
    iret = cg.solve(L, f, ub, u);

    niter  =  cg.get_iteration_count();
    resmin =  cg.get_resid_min();
    resmax =  cg.get_resid_max();
    resvec = &cg.get_residuals();
    EH_MESSAGE("main: Compute errors...");

    *utmp[0] = u - ua;
    utmp[0]->rpow(2);
    err = grid_->integrate(*utmp[0], *utmp[1]) * grid_->ivolume();

    EH_MESSAGE("main: Write to file...");

    itst.open("cg_err.txt");
    ios.open("cg_err.txt",std::ios_base::app);

    if ( itst.peek() == std::ofstream::traits_type::eof() ) {
      ios << "#elems" << "  " << "#dof" << "  ";
      for ( auto j=0; j<GDIM; j++ ) ios << "p" << j+1 << "  ";
      ios << "L2_err" << "  " << "niter" << "  " 
          << "resid_min" << "  " << "resid_max" <<  std::endl;
    }
    itst.close();

    ios << grid_->ngelems() << "  "
        << grid_->ndof()    << "  " ;
        for ( auto j=0; j<GDIM; j++ ) 
    ios << pstd[j]          << "  " ;
    ios << err              << "  " ;
    ios << niter            << "  " ;
    ios << resmin           << "  " ;
    ios << resmax           << std::endl;

    ios.close();

    errcode = err < 1e-12 ? 0 : 2;
    if ( errcode != 0 || iret != GCG<MyCGTypes>::GCGERR_NONE ) {
      cout << serr << " Error: err=" << err << " code=" << errcode << endl;
      cout << serr << " residuals=" << *resvec << endl;
    }
    assert(iret == GCG<MyCGTypes>::GCGERR_NONE  && "Solve failure");


prerror:
    // Accumulate error codes:
    GComm::Allreduce(&errcode, &gerrcode, 1, T2GCDatatype<GINT>() , GC_OP_MAX, comm_);
 
    if ( gerrcode != 0 ) {
      cout << serr << " Error: errcode=" << gerrcode << endl;
    }
    else {
      cout << serr << " Success!" << endl;
    }

#if defined(_G_USE_GPTL)
    GPTLpr_file("timing.txt");
    GPTLfinalize();
#endif

    GComm::TermComm();

    return(gerrcode);

} // end, main


//==================================================================================
// Module       : gtools.cpp
// Date         : 5/5/19 (DLR)
// Description  : GeoFLOW initialization tools
// Copyright    : Copyright 2019. Colorado State University. All rights reserved.
// Derived From : 
//==================================================================================
#include "gtools.h"

//**********************************************************************************
//**********************************************************************************
// METHOD: init_ggfx
// DESC  : Initialize gather/scatter operator
// ARGS  : ptree   : main property tree
//         grid    : Grid object
//         ggfx    : gather/scatter op, GGFX
//**********************************************************************************
void init_ggfx(PropertyTree &ptree, Grid &grid, GGFX<GFTYPE> &ggfx)
{
  GString                        serr = "init_ggfx: ";
  GFTYPE                         delta;
  GFTYPE                         rad;
  GMorton_KeyGen<GNODEID,GFTYPE> gmorton;
  GTPoint<GFTYPE>                dX, porigin, P0;
  GTVector<GNODEID>              glob_indices;
  GTVector<GTVector<GFTYPE>>    *xnodes;
  GString                        sgrid;
  std::vector<GFTYPE>            pstd;
  PropertyTree                   gtree;

  sgrid = ptree.getValue<GString>("grid_type");
  gtree = ptree.getPropertyTree(sgrid);

  if ( sgrid == "grid_box" ) {
    pstd = gtree.getArray<GFTYPE>("xyz0");
    P0   = pstd;
  }
  if ( sgrid == "grid_icos" ) {
    rad   = gtree.getValue<GFTYPE>("radius");
    P0.x1 = -rad ;
    P0.x2 = -rad ;
    P0.x3 = -rad ;
  }
  if ( sgrid == "grid_sphere" ) {
    rad   = gtree.getValue<GFTYPE>("radiuso");
    P0.x1 = -rad ;
    P0.x2 = -rad ;
    P0.x3 = -rad ;
  }

  // First, periodize coords if required to, 
  // before labeling nodes:
  if ( typeid(grid) == typeid(GGridBox) ) { 
    static_cast<GGridBox*>(&grid)->periodize();
  }

  delta  = grid.minnodedist();
  dX     = 0.1*delta;
  xnodes = &grid.xNodes();
  glob_indices.resize(grid.ndof());

  // Integralize *all* internal nodes
  // using Morton indices:
  gmorton.setIntegralLen(P0,dX);

  gmorton.key(glob_indices, *xnodes);

  // Initialize gather/scatter operator:
  GBOOL bret;
  bret = ggfx.init(glob_indices);
  assert(bret && "Initialization of GGFX operator failed");

  // Unperiodize nodes now that connectivity map is
  // generated, so that coordinates mean what they should:
  if ( typeid(grid) == typeid(GGridBox) ) { // periodize coords
    static_cast<GGridBox*>(&grid)->unperiodize();
  }

} // end method init_ggfx

