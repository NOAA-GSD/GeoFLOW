//==================================================================================
// Module       : ginitb.cpp
// Date         : 7/16/19 (DLR)
// Description  : Mag. field inititial condition implementations 
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#include "ginitb.hpp"


namespace ginitb {


//**********************************************************************************
//**********************************************************************************
// METHOD : impl_rand
// DESC   : Inititialize velocity with Gaussian-randomized values
// ARGS   : ptree  : main
//          sconfig: ptree block name containing variable config
//          grid   : grid object
//          stinfo : StateInfo
//          time   : initialization time
//          utmp   : tmp arrays
//          ub     : boundary state (also initialized here)
//          u      : state to be initialized.
// RETURNS: TRUE on success; else FALSE 
//**********************************************************************************
GBOOL impl_rand(const PropertyTree &ptree, GString &sconfig, GGrid &grid, StateInfo &stinfo, Time &time, State &utmp, State &ub, State &u)
{

  return FALSE;

} // end, method impl_rand


} // end, ginitb  namespace
