//==================================================================================
// Module       : ginitbdy_factory.hpp
// Date         : 7/11/19 (DLR)
// Description  : GeoFLOW boundary initialization factory object. 
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#if !defined(_GINITBDY_FACTORY_HPP)
#define _GINITBDY_FACTORY_HPP 

#include "tbox/property_tree.hpp"
#include "gcomm.hpp"
#include "gstateinfo.hpp"
#include "ggrid.hpp"
#include "ginitbdy_user.hpp"

using namespace geoflow;
using namespace geoflow::tbox;
using namespace std;


template<typename EquationType>
class GInitBdyFactory
{
  public:
        using Equation      = EquationType;
        using EqnBase       = EquationBase<EquationType>;
        using EqnBasePtr    = std::shared_ptr<EqnBase>;
        using State         = typename Equation::State;
        using StateInfo     = typename Equation::StateInfo;
        using Grid          = typename Equation::Grid;
        using Value         = typename Equation::Value;
        using Time          = typename Equation::Time;


	static GBOOL init               (const geoflow::tbox::PropertyTree& ptree, GGrid &grid,  StateInfo &stinfo, Time &time, State &utmp, State &u, State &ub);

  private:
        static void  set_bdy_from_state(const geoflow::tbox::PropertyTree& ptree, GGrid &grid, StateInfo &stinfo, Time &time, State &utmp, State &u, State &ub);

}; // class GInitBdyFactory



#include "ginitbdy_factory.ipp"

#endif 