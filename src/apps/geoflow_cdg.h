//==================================================================================
// Module       : geoflow_cdg.h
// Date         : 7/7/19 (DLR)
// Description  : GeoFLOW main driver for CG and DG initial value,
//                boundary value problems
// Copyright    : Copyright 2019. Colorado State University. All rights reserved.
// Derived From : 
//==================================================================================
#if !defined(GEOFLOW_CDG_MAIN_H)
#define GEOFLOW_CDG_MAIN_H

#if defined(_OPENMP)
   #include <omp.h>
#endif

#include "gexec.h"
#include "gtypes.h"
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <cassert>
#include <random>
#include <typeinfo>
#include "gcomm.hpp"
#include "ggfx.hpp"
#include "gllbasis.hpp"
#include "gmorton_keygen.hpp"
#include "gburgers.hpp"
#include "ggrid_box.hpp"
#include "ggrid_factory.hpp"
#include "pdeint/observer_base.hpp"
#include "pdeint/io_base.hpp"
#include "gio_observer.hpp"
#include "gio.hpp"
#include "pdeint/equation_base.hpp"
#include "pdeint/equation_factory.hpp"
#include "pdeint/integrator.hpp"
#include "pdeint/integrator_factory.hpp"
#include "pdeint/mixer_factory.hpp"
#include "pdeint/observer_factory.hpp"
#include "pdeint/null_observer.hpp"
#include "ginitstate_factory.hpp"
#include "ginitforce_factory.hpp"
#include "ginitbdy_factory.hpp"
#include "gupdatebdy_factory.hpp"
#include "gspecterrain_factory.hpp"
#include "tbox/command_line.hpp"
#include "tbox/property_tree.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"
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


template< // Complete typepack
typename StateType     = GTVector<GTVector<GFTYPE>*>,
typename StateCompType = GTVector<GFTYPE>,
typename StateInfoType = stStateInfo,
typename GridType      = GGrid,
typename ValueType     = GFTYPE,
typename DerivType     = StateType,
typename TimeType      = ValueType,
typename CompType      = GTVector<GStateCompType>,
typename JacoType      = StateType,
typename SizeType      = GSIZET
>
struct TypePack {
        using State      = StateType;
        using Statecomp  = StateCompType;
        using StateInfo  = StateInfoType;
        using Grid       = GridType;
        using Value      = ValueType;
        using Derivative = DerivType;
        using Time       = TimeType;
        using CompDesc   = CompType;
        using Jacobian   = JacoType;
        using Size       = SizeType;
};
using StateInfo     = stStateInfo;
using MyTypes       = TypePack<>;           // Define grid types used
using Grid          = GGrid;           
using EqnBase       = EquationBase<MyTypes>;    // Equation Base type
using EqnBasePtr    = std::shared_ptr<EqnBase>;   // Equation Base ptr
using IOBaseType    = IOBase<MyTypes>;          // IO Base type
using IOBasePtr     = std::shared_ptr<IOBaseType>;// IO Base ptr
using MixBase       = MixerBase<MyTypes>;       // Stirring/mixing Base type
using MixBasePtr    = std::shared_ptr<MixBase>;   // Stirring/mixing Base ptr
using IntegratorBase= Integrator<MyTypes>;        // Integrator
using IntegratorPtr = std::shared_ptr<IntegratorBase>; // Integrator ptr
                                                  // Integrator ptr
using ObsBase       = ObserverBase<EqnBase>;      // Observer Base type
using ObsTraitsType = ObserverBase<MyTypes>::Traits;
using BasisBase     = GTVector<GNBasis<GCTYPE,GFTYPE>*>; 
                                                  // Basis pool type

// 'Member' data:
GBOOL            bench_=FALSE;
GINT             nsolve_;      // number *solved* 'state' arrays
GINT             nstate_;      // number 'state' arrays
GINT             ntmp_  ;      // number tmp arrays
GINT             irestobs_;    // index in pObservers_ that writes restarts
Grid            *grid_=NULLPTR;// grid object
State            u_;           // state array
State            c_;           // advection velocity, if used
State            ub_;          // global bdy vector
State            uf_;          // forcing tendency
State            utmp_;        // temp array
GTVector<GFTYPE> nu_(3);       // viscosity
BasisBase        gbasis_(GDIM);// basis vector
EqnBasePtr       pEqn_;        // equation pointer
IntegratorPtr    pIntegrator_; // integrator pointer
MixBasePtr       pMixer_;      // mixer object
PropertyTree     ptree_;       // main prop tree

GGFX<GFTYPE>    *ggfx_=NULLPTR;// DSS operator
IOBasePtr        pIO_;         // ptr to IOBase operator
std::shared_ptr<std::vector<std::shared_ptr<ObserverBase<MyTypes>>>>
                 pObservers_(new std::vector<std::shared_ptr<ObserverBase<MyTypes>>>()); // observer array
GC_COMM          comm_ ;       // communicator


// Callback functions:
void update_boundary  (const Time &t, State &u, State &ub);     // bdy vector update
void update_forcing   (const Time &t, State &u, State &uf);     // forcing vec update
void steptop_callback (const Time &t, State &u, const Time &dt);// backdoor function

// Public methods:
void init_state       (const PropertyTree &ptree, GGrid &, EqnBasePtr &pEqn, Time &t, State &utmp, State &u, State &ub);
void init_force       (const PropertyTree &ptree, GGrid &, EqnBasePtr &pEqn, Time &t, State &utmp, State &u, State &uf);
void init_bdy         (const PropertyTree &ptree, GGrid &, EqnBasePtr &pEqn, Time &t, State &utmp, State &u, State &ub);
void allocate         (const PropertyTree &ptree);
void deallocate       ();
void create_observers (EqnBasePtr &eqn_ptr, PropertyTree &ptree, GSIZET icycle, Time time, std::shared_ptr<std::vector<std::shared_ptr<ObserverBase<MyTypes>>>> &pObservers);
void create_equation  (const PropertyTree &ptree, EqnBasePtr &pEqn);
void create_mixer     (PropertyTree &ptree, MixBasePtr &pMixer);
void create_basis_pool(PropertyTree &ptree, BasisBase &gbasis);
void do_terrain       (const PropertyTree &ptree, GGrid &grid);
void init_ggfx        (PropertyTree &ptree, GGrid &grid, GGFX<GFTYPE> *&ggfx);
void gresetart        (PropertyTree &ptree);
void compare          (const PropertyTree &ptree, GGrid &, EqnBasePtr &pEqn, Time &t, State &utmp, State &ub, State &u);
void do_restart       (const PropertyTree &ptree, GGrid &, State &u, GTMatrix<GINT>&p,  GSIZET &cycle, Time &t);
void do_bench         (GString sbench, GSIZET ncyc);

//#include "init_pde.h"

#endif
