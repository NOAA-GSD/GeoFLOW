//==================================================================================
// Module       : gpoisson.hpp
// Date         : 3/27/20 (DLR)
// Description  : Encapsulates the methods and data associated with
//                a Poisson solver.
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================

#if !defined(_GPOISSON_HPP)
#define _GPOISSON_HPP

#include <sstream>
#include "gtvector.hpp"
#include "gtmatrix.hpp"
#include "gcg.hpp"
#include "gcomm.hpp"
#include "ggfx.hpp"
#include "pdeint/lin_solver_base.hpp"

using namespace geoflow::pdeint;
using namespace std;

template<typename TypePack> 
class GPoisson 
{
public:
//                    enum  GPoissonERR         {GPoissonERR_NONE=0, GPoissonERR_NOCONVERGE, GPoissonERR_SOLVE, GPoissonERR_PRECOND,  GPoissonERR_BADITERNUM};
                      using Types          = TypePack;
                      using SolverBase     = LinSolverBase<Types>;
                      using LapOperator    = typename types::LapOperator
                      using Preconditioner = typename Types::Preconditioner;
                      using State          = typename Types::State;
                      using StateComp      = typename Types::StateComp;
                      using Grid           = typename Types::Grid;
                      using Value          = typename Types::Value;
                      using ConnectivityOp = typename Types::ConnectivityOp;
                      using Traits         = typename SolverBase::Traits;

                      static_assert(std::is_same<State,GTVector<GTVector<GFTYPE>*>>::value,
                                    "State is of incorrect type");
                      static_assert(std::is_same<StateComp,GTVector<GFTYPE>>::value,
                                    "StateComp is of incorrect type");
                      static_assert(std::is_same<Value,GFTYPE>::value,
                                    "Value is of incorrect type");
                      static_assert(std::is_same<ConnectivityOp,GGFX<Value>>::value,
                                    "ConnectivityOp is of incorrect type");


                      GPoisson() = delete;
                      GPoisson(Traits& traits, Grid& grid, LapOperator& L, Preconditioner* P, ConnectivityOp& ggfx, State& tmppack);
                     ~GPoisson();
                      GPoisson(const GPoisson &a);
                      GPoisson  &operator=(const GPoisson &);
                   

                      GINT     solve(const StateComp& b, StateComp& x);
                      GINT     solve(const StateComp& b, 
                                     const StateComp& xb, StateComp& x);

                      StateComp&   get_residuals() { return cg_->get_residuals(); }  
                      GFTYPE       get_resid_max() { return cg_->get_resid_max(); }
                      GFTYPE       get_resid_min() { return cg_->get_resod_min(); }
                      GINT         get_iteration_count() { return cg_->get_iteration_count(); }  


private:
// Private methods:
                      void     init();
// Private data:
     GC_COMM          comm_;        // communicator
     GBOOL            bInit_;       // object initialized?
     State           *tmppack_;     // tmp vector pool
     Grid            *grid_;        // grid object
     Preconditioner  *precond_;     // preconditioner
     LapOperator     *L_;           // Laplacian operator
     GCG<Types>      *cg_;          // Conjugate gradient operator


};

#include "gcg.ipp"

#endif

