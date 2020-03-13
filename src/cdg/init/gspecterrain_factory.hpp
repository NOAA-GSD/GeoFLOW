//==================================================================================
// Module       : gspecterrain_factory.hpp
// Date         : 3/11/20 (DLR)
// Description  : GeoFLOW terrain specification factory
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#if !defined(_GSPECTERRAIN_FACTORY_HPP)
#define _GSPECTERRAIN_FACTORY_HPP 

#include "tbox/property_tree.hpp"
#include "pdeint/equation_base.hpp"
#include "gcomm.hpp"
#include "gtvector.hpp"
#include "ggrid.hpp"
#include "ggrid_icos.hpp"
#include "ggrid_box.hpp"
#include "gterrain_specbox_user.hpp"
#include "gterrain_specsph_user.hpp"

using namespace geoflow::pdeint;
using namespace std;

template<typename TypePack>
class GSpecTerrainFactory
{
  public:
        using Types         = TypePack;
        using State         = typename Types::State;
        using StateComp     = typename Types::StateComp;
        using Grid          = typename Types::Grid;
        using Value         = typename Types::Value;


	static GBOOL spec(const geoflow::tbox::PropertyTree& ptree, Grid &grid, State &utmp, State &xb);

  private:
	       GBOOL spec_box   (const geoflow::tbox::PropertyTree& ptree, Grid &grid, State &utmp, GString stype, State &xb);
	       GBOOL spec_sphere(const geoflow::tbox::PropertyTree& ptree, Grid &grid, State &utmp, GString stype,  State &xb);

}; // end, class GSpecTerrainFactory

#include "gspecterrain_factory.ipp"

#endif 
