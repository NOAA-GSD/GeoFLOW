//==================================================================================
// Module       : gtest_burgers.cpp
// Date         : 12/24/18 (DLR)
// Description  : GeoFLOW test of Burgers PDE
// Copyright    : Copyright 2018-19. Colorado State University. All rights reserved.
// Derived From : 
//==================================================================================

#include "gexec.h"
#include "gtypes.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <gptl.h>
#include <memory>
#include <cstdlib>
#include <cassert>
#include <random>
#include "gcomm.hpp"
#include "ggfx.hpp"
#include "gllbasis.hpp"
#include "gmorton_keygen.hpp"
#include "gburgers.hpp"
#include "ggrid_factory.hpp"
#include "pdeint/equation_base.hpp"
#include "pdeint/integrator.hpp"
#include "pdeint/null_observer.hpp"
#include "tbox/property_tree.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"

using namespace geoflow::pdeint;
using namespace geoflow::tbox;
using namespace std;


template< // default template arg types
typename StateType = GTVector<GTVector<GFTYPE>*>,
typename ValueType = GFTYPE,
typename DerivType = StateType,
typename TimeType  = ValueType,
typename JacoType  = StateType,
typename SizeType  = GSIZET
>
struct EquationTypes {
        using State      = StateType;
        using Value      = ValueType;
        using Derivative = DerivType;
        using Time       = TimeType;
        using Jacobian   = JacoType;
        using Size       = SizeType;
};

GGrid *grid_;
GTVector<GTVector<GFTYPE>*> u_;
GTVector<GTVector<GFTYPE>*> ua_;
GTVector<GTVector<GFTYPE>*> ub_;
GTVector<GTVector<GFTYPE>*> utmp_;
GTVector<GFTYPE> nu_(3);

void compute_analytic(GGrid &grid, GFTYPE &t, const PropertyTree& ptree, GTVector<GTVector<GFTYPE>*> &u);
void update_dirichlet(const GFTYPE &t, GTVector<GTVector<GFTYPE>*> &u, GTVector<GTVector<GFTYPE>*> &ub);
void init_ggfx(GGrid &grid, GGFX &ggfx);

//#include "init_pde.h"

int main(int argc, char **argv)
{

    // Get types used for equations and solver
//  using MyTypes = EquationTypes<GTVector<GTVector<GFTYPE>*>>; // Define types used
    using MyTypes = EquationTypes<>; // Define types used
    using EqnBase = EquationBase<MyTypes>;                     // Equation Base Type
    using EqnImpl = GBurgers<MyTypes>;                         // Equation Implementa
    using ObsBase = ObserverBase<EqnBase>;                     // Observer Base Type
    using ObsImpl = NullObserver<EqnImpl>;                     // Observer Implementation Type
//  using IntImpl = Integrator<EqnImpl>;                       // Integrator Implementation Type

    GString serr ="main: ";
    GBOOL  bret;
    GINT   iopt;
    GINT   ilevel=0;// 2d ICOS refinement level
    GINT   np=1;    // elem 'order'
    GINT   nstate=GDIM;  // number 'state' arrays
    GSIZET maxSteps;
    GFTYPE radiusi=1, radiuso=2;
    std::vector<GINT> ne(3); // # elements in each direction in 3d
    GString sgrid;// name of JSON grid object to use
    GC_COMM comm = GC_COMM_WORLD;

    typename MyTypes::Time  t  = 0;
    typename MyTypes::Time  dt = 0.1;

    // Initialize comm & global environment:
    mpixx::environment env(argc,argv); // init GeoFLOW comm
    mpixx::communicator world;
    GlobalManager::initialize(argc,argv); 
    GlobalManager::startup();
    comm = world; // need this for solver(s) & grid


    // Read main prop tree; may ovewrite with
    // certain command line args:
    PropertyTree ptree;       // main prop tree
    PropertyTree eqptree;     // equation props
    PropertyTree gridptree;   // grid props
    PropertyTree stepptree;   // stepper props
    PropertyTree dissptree;   // dissipation props
    PropertyTree tintptree;   // time integration props

cout << "main: call load_file..." << endl;
    ptree    = InputManager::getInputPropertyTree();       // main param file structure
cout << "main: load_file done." << endl;
    // Create other prop trees for various objects:
    sgrid       = ptree.getValue<GString>("grid_type");
    np          = ptree.getValue<GINT>("exp_order");
    eqptree     = ptree.getPropertyTree("adv_equation_traits");
    gridptree   = ptree.getPropertyTree(sgrid);
    stepptree   = ptree.getPropertyTree("stepper_props");
    dissptree   = ptree.getPropertyTree("dissipation_traits");
    tintptree   = ptree.getPropertyTree("time_integration");

    ne          = gridptree.getArray<GINT>("num_elems");  // may be modified by command line

#if 1

    // Parse command line. ':' after char
    // option indicates that it takes an argument.
    // Note: -i reserved for InputManager:
    while ((iopt = getopt(argc, argv, "i:j:k:l:m:p:h")) != -1) {
      switch (iopt) {
      case 'i': // handled by InputManager
          break;
      case 'j': // get # elements in r/x
          ne[0] = atoi(optarg);
          gridptree.setArray<GINT>("num_elems",ne);
          break;
      case 'k': // get # elements in lat/y
          ne[1] = atoi(optarg);
          gridptree.setArray<GINT>("num_elems",ne);
          break;
      case 'm': // get # elements in long/z
          ne[2] = atoi(optarg);
          gridptree.setArray<GINT>("num_elems",ne);
          break;
      case 'l': // # 2d refinement level
          ilevel = atoi(optarg);
          gridptree.setValue<GINT>("ilevel",ilevel);
          break;
      case 'p': // get nodal exp order
          np = atoi(optarg);
          ptree.setValue<GINT>("exp_order",np);
          break;
      case 'h': // help
          std::cout << "usage: " << std::endl <<
          argv[0] << " [-h] [-j #Elems in x/r] [-k #Elems in y/lat]  [-m #Elems in z/long] [-l refine level] -p expansion order] " << std::endl;
          std::cout << "Note: Icos grid only requires refine level to specify number of elements. " << std::endl;
          exit(1); 
          break;
      case ':': // missing option argument
          std::cout << argv[0] << ": option " << optopt << " requires an argument" << std::endl;
          exit(1); 
          break;
      case '?':
      default: // invalid option
          std::cout << argv[0] << ": option " << optopt << " invalid" << std::endl;
          exit(1);
          break;
      }
    }

#endif

    // Set solver traits from prop tree:
    GFTYPE nu_scalar;
    GBurgers<MyTypes>::Traits solver_traits;
    solver_traits.doheat     = eqptree  .getValue<GBOOL>("doheat");
    solver_traits.bpureadv   = eqptree  .getValue<GBOOL>("bpureadv");
    solver_traits.bconserved = eqptree  .getValue<GBOOL>("bconserved");
    solver_traits.itorder    = stepptree.getValue <GINT>("time_deriv_order");
    solver_traits.inorder    = stepptree.getValue <GINT>("extrap_order");
    nu_scalar                = dissptree.getValue<GFTYPE>("nu");
    nu_.resize(1); 
    nu_[0] = nu_scalar; 
    GTVector<GString> ssteppers;
    for ( GSIZET j=0; j<GSTEPPER_MAX; j++ ) ssteppers.push_back(sGStepperType[j]);
    GSIZET itype; 
    GString stepmthd = stepptree.getValue<GString>("stepping_method");
    GBOOL  bfound = ssteppers.contains(stepmthd,itype);
    assert( bfound && "Invalide stepping method in JSON file");
    
    solver_traits.steptype   = static_cast<GStepperType>(itype);
    

    // Set GTPL options:
    GPTLsetoption (GPTLcpu, 1);

    // Initialize GPTL:
    GPTLinitialize();


    // Create basis:
    GTVector<GNBasis<GCTYPE,GFTYPE>*> gbasis(GDIM);
    for ( GSIZET k=0; k<GDIM; k++ ) {
      gbasis [k] = new GLLBasis<GCTYPE,GFTYPE>(np);
std::cout << "main: gbasis [" << k << "]_order=" << gbasis [k]->getOrder() << std::endl;
    }
    
    GPTLstart("gen_grid");
    // Create grid:
    grid_ = GGridFactory::build(gridptree, gbasis, comm);
    GPTLstop("gen_grid");


    GPTLstart("do_gather_op");
    // Initialize gather/scatter operator:
    GGFX ggfx;
    init_ggfx(*grid_, ggfx);
    GPTLstop("do_gather_op");


    // Create state and tmp space:
    utmp_.resize(12);
    
    if      ( solver_traits.doheat   ) nstate = 1;
    else if ( solver_traits.bpureadv ) nstate = GDIM + 1; // 1-state + GDIM  v-components
    u_.resize(nstate);
    ua_.resize(nstate);
    ub_.resize(nstate);
    for ( GSIZET j=0; j<utmp_.size(); j++ ) utmp_[j] = new GTVector<GFTYPE>(grid_->size());
    for ( GSIZET j=0; j<u_   .size(); j++ ) u_   [j] = new GTVector<GFTYPE>(grid_->size());
    for ( GSIZET j=0; j<ua_  .size(); j++ ) ua_  [j] = new GTVector<GFTYPE>(grid_->size());

    // Create observer(s), equations, integrator:
    std::shared_ptr<EqnImpl> eqn_impl(new EqnImpl(ggfx, *grid_, u_, solver_traits, utmp_));
    std::shared_ptr<EqnBase> eqn_base = eqn_impl;

    // Set Dirichlet bdy state update function:
    std::function<void(const GFTYPE &t, GTVector<GTVector<GFTYPE>*> &u, 
                                        GTVector<GTVector<GFTYPE>*> &ub)>  
        fcallback = update_dirichlet; // set tmp function with proper signature for...
    eqn_impl->set_bdy_update_callback(fcallback);
    eqn_impl->set_nu(nu_);

    // Create the observers: 
    std::shared_ptr<ObsImpl> obs_impl(new ObsImpl());
    std::shared_ptr<ObsBase> obs_base =  NULLPTR; //obs_impl;

//  std::shared_ptr<IntImpl> int_impl(new IntImpl(eqn_base,obs_base,int_traits));
cout << "main: getting dt..." << endl; 
    dt       = tintptree.getValue<GFTYPE>("dt"); 
cout << "main: getting cycle_end..." << endl; 
    maxSteps = tintptree.getValue<GSIZET>("cycle_end");
cout << "main: getting integrator data done." << endl; 

    // Initialize state:
cout << "main: Initializing state..." << endl; 
    compute_analytic(*grid_, t, ptree, u_);
cout << "main: State initiallized." << endl; 

cout << "main: entering time loop..." << endl; 
    GPTLstart("time_loop");
    for( GSIZET i=0; i<maxSteps; i++ ){
      eqn_base->step(t,u_,ub_,dt);
      t += dt;
    }
    GPTLstop("time_loop");
cout << "main: time-stepping done." << endl; 


#if 1
    // Compute analytic solution, do comparisons:
    GTVector<GFTYPE> lerrnorm(3), gerrnorm(3), maxerror(3);
    maxerror = 0.0;
    compute_analytic(*grid_, t, ptree, ua_);
cout << "main: ua[0]=" << *ua_[0] << endl;
cout << "main: u [0]=" << *u_ [0] << endl;
    for ( GSIZET j=0; j<u_.size(); j++ ) { //local errors
      *utmp_[0] = *u_[j] - *ua_[j];
cout << "main: diff[" << j << "]=" << *utmp_[0] << endl;
       lerrnorm[0]  = utmp_[0]->L1norm (); // inf-norm
       lerrnorm[1]  = utmp_[0]->Eucnorm(); // Euclidean-norm
       lerrnorm[1] *= lerrnorm[1]; // square it, so it can be added 
       gerrnorm[2]  = grid_->integrate(*utmp_[0],*utmp_[1]) /
                      grid_->integrate(*ua_  [j],*utmp_[1]) ; // Global L2 error norm 
      // Accumulate to find global errors for this field:
      GComm::Allreduce(lerrnorm.data()  , gerrnorm.data()  , 1, T2GCDatatype<GFTYPE>() , GC_OP_MAX, comm);
      GComm::Allreduce(lerrnorm.data()+1, gerrnorm.data()+1, 1, T2GCDatatype<GFTYPE>() , GC_OP_SUM, comm);
      // now find max errors of each type for each field:
      for ( GSIZET i=0; i<3; i++ ) maxerror[i] = MAX(maxerror[i],gerrnorm[j]);
    }
   
    GSIZET lnelems=grid_->nelems();
    GSIZET gnelems;
    GComm::Allreduce(&lnelems, &gnelems, 1, T2GCDatatype<GSIZET>() , GC_OP_SUM, comm);

    // Print convergence data to file:
    std::ifstream itst;
    std::ofstream ios;
    itst.open("burgers_err.txt");
    ios.open("burgers_err.txt",std::ios_base::app);

    // Write header, if required:
    if ( itst.peek() == std::ofstream::traits_type::eof() ) {
    ios << "# p  num_elems   L1_err   Eucl_err   L2_err" << std::endl;
    }
    itst.close();

    ios << np  << "  "  << "  " << gnelems << "  "
        << "  " << maxerror[0] << "  " << maxerror[1] << "  " << maxerror[2]
        << std::endl;
    ios.close();

#endif
 
    GPTLpr_file("timing.txt");
    GPTLfinalize();

//  GComm::TermComm();
    GlobalManager::shutdown();
    GlobalManager::finalize();
    if ( grid_ != NULLPTR ) delete grid_;

    return(0);

} // end, main


//**********************************************************************************
//**********************************************************************************
// METHOD : update_dirichlet
// DESC   : update/set Dirichlet vectors, ub
// ARGS   : t    : time
//          u    : current state
//          ub   : bdy vectors (one for each state element)
// RETURNS: none.
//**********************************************************************************
void update_dirichlet(const GFTYPE &t, GTVector<GTVector<GFTYPE>*> &u, GTVector<GTVector<GFTYPE>*> &ub)
{
  GTVector<GTVector<GSIZET>> *igbdy = &grid_->igbdy();

  // ...GBDY_DIRICHLET:
  // Set from State vector, ua_:
  for ( GSIZET k=0; k<u.size(); k++ ) { 
    for ( GSIZET j=0; j<(*igbdy)[GBDY_DIRICHLET].size(); j++ ) {
      (*ub[k])[j] = (*ua_[k])[(*igbdy)[GBDY_DIRICHLET][j]];
    }
  }

} // end of method update_dirichlet


//**********************************************************************************
//**********************************************************************************
// METHOD: compute_percircnwave_burgers
// DESC  : Compute solution to (nonlinear) Burgers with N-wave
//         initial conditions for use with GBDY_PERIODIC bdys
//         Must use box grid.
// ARGS  : grid    : GGrid object
//         t       : time
//         ptree   : main property tree
//         ua      : return solution
//**********************************************************************************
void compute_percircnwave_burgers(GGrid &grid, GFTYPE &t, const PropertyTree& ptree,  GTVector<GTVector<GFTYPE>*> &ua)
{

  assert(FALSE && "N-wave not ready yet");

  GBOOL            bContin;
  GFTYPE           wsum, prod, argxp, argxm, da, eps=1e-18;
  GFTYPE           nxy, pint, sig0, ufact=1.0, u0;
  GFTYPE           K2;
  GTVector<GFTYPE> f(GDIM), K[GDIM], xx(GDIM), si(GDIM), sig(GDIM);
  GTPoint<GFTYPE>  r0(3), P0(3), gL(3);

  PropertyTree heatptree = ptree.getPropertyTree("init_lump");

  GTVector<GTVector<GFTYPE>> *xnodes = &grid_->xNodes();

  assert(grid.gtype() == GE_REGULAR && "Invalid element types");

  // Get periodicity length, gL:
  PropertyTree boxptree = ptree.getPropertyTree("grid_box");
  std::vector<GFTYPE> xyz0 = boxptree.getArray<GFTYPE>("xyz0");
  std::vector<GFTYPE> dxyz = boxptree.getArray<GFTYPE>("delxyz");
  P0 = xyz0; r0 = dxyz; gL = P0 + r0;

  nxy = (*xnodes)[0].size(); // same size for x, y, z
 
  assert(FALSE && "Periodized N-wave unavailable"); 
#if 0
  for ( GSIZET j=0; j<nxy; j++ ) {
    for ( GSIZET i=0; i<GDIM; i++ ) {
      f [i] = modf((*c[i])[j]*t/gL[i],&pint);
      xx[i] = (*xnodes)[i][j] - r0[i] - f[i]*gL[i];

    }

    prod = 1.0;
    for ( GSIZET k=0; k<GDIM; k++ ) {
      wsum     = exp(-xx[k]*xx[k]*si[k]);
      n       = 1;
      bContin = TRUE;
      while ( bContin ) {
        argxp = -pow((xx[k]+n*gL[k]),2.0)*si[k];
        argxm = -pow((xx[k]-n*gL[k]),2.0)*si[k];
        da    =  exp(argxp) + exp(argxm);
        wsum  += da;
        bContin = da/wsum > eps;
        n++;
      }
      prod *= wsum;
    }
    (*ua[0])[j] = (*ua[0])[j]*ufact + u0*pow(sig0,2)/pow(sig[0],2)*prod;
  }
#endif
  
} // end, compute_percircnwave_burgers


//**********************************************************************************
//**********************************************************************************
// METHOD: compute_pergauss_adv
// DESC  : Compute solution to pure advection equation with 
//         GBDY_PERIODIC bcs, a Gaussian 'lump'. Must use box grid.
// ARGS  : grid    : GGrid object
//         t       : time
//         ptree   : main property tree
//         ua      : return solution
//**********************************************************************************
void compute_pergauss_adv(GGrid &grid, GFTYPE &t, const PropertyTree& ptree,  GTVector<GTVector<GFTYPE>*>  &ua)
{
  GBOOL            bAdd, bContin;
  GINT             n;
  GFTYPE           wsum, prod, argxp, argxm, da, eps=1e-18;
  GFTYPE           nxy, pint, sig0, ufact=1.0, u0;
  GTVector<GFTYPE> f(GDIM), xx(GDIM), si(GDIM), sig(GDIM);
  GTPoint<GFTYPE>  r0(3), P0(3), gL(3);

  PropertyTree heatptree = ptree.getPropertyTree("init_lump");

  GTVector<GTVector<GFTYPE>> *xnodes = &grid_->xNodes();

  assert(grid.gtype() == GE_REGULAR && "Invalid element types");

  // Get periodicity length, gL:
  PropertyTree boxptree = ptree.getPropertyTree("grid_box");
  std::vector<GFTYPE> xyz0 = boxptree.getArray<GFTYPE>("xyz0");
  std::vector<GFTYPE> dxyz = boxptree.getArray<GFTYPE>("delxyz");
  P0 = xyz0; r0 = dxyz; gL = P0 + r0;

  bAdd = FALSE; // add solution to existing ua

  nxy = (*xnodes)[0].size(); // same size for x, y, z
  
  r0.x1 = heatptree.getValue<GFTYPE>("x0"); 
  r0.x2 = heatptree.getValue<GFTYPE>("y0"); 
  r0.x3 = heatptree.getValue<GFTYPE>("z0"); 
  sig0  = heatptree.getValue<GFTYPE>("sigma"); 
  u0    = heatptree.getValue<GFTYPE>("u0"); 

  // Set adv velocity components:
  GTVector<GTVector<GFTYPE>*> c(GDIM);

  for ( GSIZET k=0; k<GDIM; k++ ) {
    sig[k] = sqrt(sig0*sig0 + 2.0*t*nu_[k]);
    si [k] = 0.5/(sig[k]*sig[k]);
    c  [k] = ua[k+1];
  }
  ufact = bAdd ? 1.0 : 0.0;

  for ( GSIZET j=0; j<nxy; j++ ) {
    for ( GSIZET i=0; i<GDIM; i++ ) {
      f [i] = modf((*c[i])[j]*t/gL[i],&pint);
      xx[i] = (*xnodes)[i][j] - r0[i] - f[i]*gL[i];

    }

    prod = 1.0;
    for ( GSIZET k=0; k<GDIM; k++ ) {
      wsum     = exp(-xx[k]*xx[k]*si[k]);
      n       = 1;
      bContin = TRUE;
      while ( bContin ) {
        argxp = -pow((xx[k]+n*gL[k]),2.0)*si[k];
        argxm = -pow((xx[k]-n*gL[k]),2.0)*si[k];
        da    =  exp(argxp) + exp(argxm);
        wsum  += da;
        bContin = da/wsum > eps;
        n++;
      }
      prod *= wsum;
    }
    (*ua[0])[j] = (*ua[0])[j]*ufact + u0*pow(sig0,2)/pow(sig[0],2)*prod;
  }

  
} // end, compute_pergauss_adv


//**********************************************************************************
//**********************************************************************************
// METHOD: compute_pergauss_heat
// DESC  : Compute solution to heat equation with GBDY_PERIODIC bcs,
//         a Gaussian 'lump'. Must use box grid.
// ARGS  : grid    : GGrid object
//         t       : time
//         ptree   : main property tree
//         ua      : return solution
//**********************************************************************************
void compute_pergauss_heat(GGrid &grid, GFTYPE &t, const PropertyTree& ptree,  GTVector<GTVector<GFTYPE>*> &ua)
{
  GBOOL            bAdd, bContin;
  GINT             n;
  GFTYPE           wsum, prod, argxp, argxm, da, eps=1e-18;
  GFTYPE           nxy, sig0, ufact=1.0, u0;
  GTVector<GFTYPE> xx(GDIM), si(GDIM), sig(GDIM);
  GTPoint<GFTYPE>  r0(3), P0(3), gL(3);
  std::vector<GFTYPE> svec;

  PropertyTree heatptree = ptree.getPropertyTree("init_lump");

  GTVector<GTVector<GFTYPE>> *xnodes = &grid_->xNodes();

  assert(grid.gtype() == GE_REGULAR && "Invalid element types");
  
  // Get periodicity length, gL:
  PropertyTree boxptree = ptree.getPropertyTree("grid_box");
  std::vector<GFTYPE> xyz0 = boxptree.getArray<GFTYPE>("xyz0");
  std::vector<GFTYPE> dxyz = boxptree.getArray<GFTYPE>("delxyz");
  P0 = xyz0; r0 = dxyz; gL = P0 + r0;
  

  bAdd = FALSE; // add solution to existing ua

  nxy = (*xnodes)[0].size(); // same size for x, y, z
  
  r0.x1 = heatptree.getValue<GFTYPE>("x0"); 
  r0.x2 = heatptree.getValue<GFTYPE>("y0"); 
  r0.x3 = heatptree.getValue<GFTYPE>("z0"); 
  sig0  = heatptree.getValue<GFTYPE>("sigma"); 
  u0    = heatptree.getValue<GFTYPE>("u0"); 

  for ( GSIZET k=0; k<GDIM; k++ ) {
    sig[k] = sqrt(sig0*sig0 + 2.0*t*nu_[0]); // scalar viscosity only
    si [k] = 0.5/(sig[k]*sig[k]);
  }
  ufact = bAdd ? 1.0 : 0.0;

  for ( GSIZET j=0; j<nxy; j++ ) {
    for ( GSIZET i=0; i<GDIM; i++ ) xx[i] = (*xnodes)[i][j] - r0[i];

    prod = 1.0;
    for ( GSIZET k=0; k<GDIM; k++ ) {
      wsum    = exp(-xx[k]*xx[k]*si[k]);
      n       = 1;
      bContin = TRUE;
      while ( bContin ) {
        argxp = -pow((xx[k]+n*gL[k]),2.0)*si[k];
        argxm = -pow((xx[k]-n*gL[k]),2.0)*si[k];
        da    =  exp(argxp) + exp(argxm);
        wsum  += da;
        bContin = da/wsum > eps;
        n++;
      }
      prod *= wsum;
    }
    (*ua[0])[j] = (*ua[0])[j]*ufact + u0*pow(sig0,2)/pow(sig[0],2)*prod;
  }

  
} // end, compute_pergauss_heat


//**********************************************************************************
//**********************************************************************************
// METHOD: compute_analytic
// DESC  : Compute analytic solutions based on property tree
// ARGS  : grid    : GGrid object
//         t       : time
//         ptree   : main property tree
//         ua      : return solution
//**********************************************************************************
void compute_analytic(GGrid &grid, GFTYPE &t, const PropertyTree& ptree, GTVector<GTVector<GFTYPE>*>  &ua)
{
  PropertyTree advptree  = ptree.getPropertyTree("adv_equation_traits");
  GBOOL doheat   = advptree.getValue<GBOOL>("doheat");
  GBOOL bpureadv = advptree.getValue<GBOOL>("bpureadv");

  GString      sblock = ptree.getValue<GString>("init_block"); // name of initialization block
  PropertyTree blockptree = ptree.getPropertyTree(sblock); // sub-block of main ptree describing initialization type

  
  if ( doheat  ) {
    
    if ( sblock.compare("init_lump") == 0  ) {
      compute_pergauss_heat(grid, t, ptree, ua);
    }
    else {
      assert(FALSE && "Invalid heat equation initialization specified");
    }
    return;
  } // end, heat equation inititilization types



  // Inititialize for pure advection:
  if ( bpureadv ) {
    if ( sblock.compare("init_lump") == 0  ) {
      compute_pergauss_adv(grid, t, ptree, ua);
    }
    else {
      assert(FALSE && "Invalid pure adv equation initialization specified");
    }
    return;
  }

  // Inititialize for nonlinear advection:
  if ( sblock.compare("init_lump") == 0  ) {
    compute_percircnwave_burgers(grid, t, ptree, ua);
  }
  else {
    assert(FALSE && "Invalid Burgers equation initialization specified");
  }

} // end, compute_analytic


//**********************************************************************************
//**********************************************************************************
// METHOD: init_ggfx
// DESC  : Initialize gather scatter operator
// ARGS  : grid    : GGrid object
//         ggfx    : gather/scatter op, GGFX
//**********************************************************************************
void init_ggfx(GGrid &grid, GGFX &ggfx)
{
  GFTYPE                         delta;
  GMorton_KeyGen<GNODEID,GFTYPE> gmorton;
  GTPoint<GFTYPE>                dX, porigin, P0;
  GTVector<GNODEID>              glob_indices;
  GTVector<GTVector<GFTYPE>>    *xnodes;

  delta  = grid.minnodedist().min();
  dX     = 0.5*delta;
  xnodes = &grid.xNodes();
  glob_indices.resize(grid.ndof());

  // Integralize *all* internal nodes
  // using Morton indices:
  gmorton.setIntegralLen(P0,dX);
  gmorton.key(glob_indices, *xnodes);

  // Initialize gather/scatter operator:
  GBOOL bret = ggfx.init(glob_indices);
  assert(bret && "Initialization of GGFX operator failed");

} // end method init_ggfx
