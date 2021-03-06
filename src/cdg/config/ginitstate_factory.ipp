//==================================================================================
// Module       : ginitstate_factory
// Date         : 7/11/19 (DLR)
// Description  : GeoFLOW state initialization factory
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================


//**********************************************************************************
//**********************************************************************************
// METHOD : init
// DESC   : Do init of state components
// ARGS   : ptree  : main property tree
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::init(const PropertyTree& ptree, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL         bret    = FALSE;
  GString       stype;  

  // Get type of initialization: by-name or by-block:
  stype = ptree.getValue<GString>("initstate_type","");
  if ( "direct"   == stype 
    || ""         == stype ) {
    bret = set_by_direct(ptree, eqn, grid, time, utmp, u);
  }
  else if ( "component" == stype ) {
    bret = set_by_comp  (ptree, eqn, grid, time, utmp, u);
  }
  else {
    assert(FALSE && "Invalid state initialization type");
  }

  return bret;

} // end, init method


//**********************************************************************************
//**********************************************************************************
// METHOD : set_by_direct
// DESC   : Do init of state components by calling init method to set
//          entire state at once. E.g., one might classify initialization
//          schemes by PDE-type, and user is responsible to ensuring 
//          all state members are initialized.
// ARGS   : ptree  : main property tree
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::set_by_direct(const PropertyTree& ptree, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL         bret    = FALSE;
  GString       sinit   = ptree.getValue<GString>("initstate_block");

  if      ( "zero"                        == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) {
      if ( u[i] != NULLPTR ) *u[i] = 0.0;
    } 
  }
  else if ( "initstate_icosgauss"  == sinit ) {
    bret = ginitstate<Types>::impl_icosgauss       (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_boxdirgauss"       == sinit ) {
    bret = ginitstate<Types>::impl_boxdirgauss     (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_boxpergauss"       == sinit ) {
    bret = ginitstate<Types>::impl_boxpergauss     (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_boxnwave"          == sinit ) {
    bret = ginitstate<Types>::impl_boxnwaveburgers (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_icosnwave"          == sinit ) {
    bret = ginitstate<Types>::impl_icosnwaveburgers (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_boxdrybubble"       == sinit ) {
    bret = ginitstate<Types>::impl_boxdrybubble     (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_icosabcconv"        == sinit ) {
    bret = ginitstate<Types>::impl_icosabcconv      (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else if ( "initstate_boxsod"             == sinit ) {
    bret = ginitstate<Types>::impl_boxsod           (ptree, sinit, eqn, grid, time, utmp, u);
  }
  else                                        {
    assert(FALSE && "Specified state initialization method unknown");
  }

  return bret;

} // end, set_by_direct method


//**********************************************************************************
//**********************************************************************************
// METHOD : set_by_comp
// DESC   : Do init of state components by specifying individual
//          variable group types within JSON block
//          in the state separately. This method uses the CompDesc data
//          in the EqnBase pointer to locate variable groups.
// ARGS   : ptree  : main property tree
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::set_by_comp(const PropertyTree& ptree, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
#if 0
  GBOOL           bret    = TRUE;
  GSIZET          ndist, *ivar, mvar, nvar;
  GSIZET         *idist, *pisz;
  GStateCompType  itype;
  GString         sblk      = ptree.getValue<GString>("initstate_block");
  GString         sinit;
  PropertyTree    vtree     = ptree.getPropertyTree(sblk);
  State           comp;
  GStateCompType *pct;
  CompDesc       *icomptype = &stinfo.icomptype;
  CompDesc        cdesc;

  GTVector<GSIZET>   itmp;

  cdesc.resize(icomptype->size());
  itmp .resize(icomptype->size());
  pct = cdesc.data();
  pisz = itmp.data();

  ndist = 0; // # distinct comp types
  idist = NULLPTR; // list of ids for  distinct comp types

  mvar      = 0; // # max of specific comp types
  ivar      = NULLPTR;  // list of ids for specific comp types

  // Get distinct component types:
  icomptype->distinct(idist, ndist, pct, pisz);

  // Cycle over all types required, get components of that
  // type, and initialize all of them. There should be a member
  // function for each GStateCompType:
  for ( GSIZET j=0; j<ndist && bret; j++ ) {
    
    itype = (*icomptype)[idist[j]]; 
    switch ( itype ) {

      case GSC_KINETIC:
        sinit = vtree.getValue<GString>("initv");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinitv(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_DENSITYT:
        sinit = vtree.getValue<GString>("initdt");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinitdt(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_MASSFRAC:
        sinit = vtree.getValue<GString>("initmfrac");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinitmfrac(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_ENERGY:
        sinit = vtree.getValue<GString>("initen");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinitenergy(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_TEMPERATURE:
        sinit = vtree.getValue<GString>("inittemp");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinittemp(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_PRESCRIBED:
        sinit = vtree.getValue<GString>("initc");
        nvar  = icomptype->contains(itype, ivar, mvar);
        comp.resize(nvar);
        for ( GINT i=0; i<nvar; i++ ) comp[i] = u[ivar[i]];
        bret = doinitc(ptree, sinit, eqn, grid, time, utmp, comp);
        break;
      case GSC_NONE:
        break;
      default:
        assert(FALSE && "Invalid component type");
    } // end, switch

  } // end, j loop

  if ( idist  != NULLPTR ) delete [] idist;
  if ( ivar   != NULLPTR ) delete [] ivar ;

  return bret;
#endif

  return FALSE;

} // end, set_by_comp method


//**********************************************************************************
//**********************************************************************************
// METHOD : doinitv
// DESC   : Do init of kinetic components. Full list of available
//          kinetic initializations are contained here. Only
//          kinetic components are passed in.
// ARGS   : ptree  : initial condition property tree
//          sconfig: ptree block name containing kinetic variable config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinitv(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL             bret  = TRUE;
  GString           sinit;
  GridIcos         *icos;
  GridBox          *box;
  PropertyTree      vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  icos = dynamic_cast<GridIcos*>(&grid);
  box  = dynamic_cast<GridBox*> (&grid);
  if      ( "null"   == sinit
       ||   ""       == sinit ) {     // set to 0
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {       // set to 0
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
  }
  else if ( "abc" == sinit ) {        // ABC init
    if      ( icos != NULLPTR ) {
      bret = ginitv<Types>::impl_abc_icos(ptree, sconfig, eqn, grid, time, utmp, u);
    }
    else  {
      bret = ginitv<Types>::impl_abc_box (ptree, sconfig, eqn, grid, time, utmp, u);
    }
  } 
  else if ( "simplesum1d" == sinit ) { // simple-wave sum init for 1d approx
      assert( icos == NULLPTR && "simplesum1d allowed only for box grid");
      bret = ginitv<Types>::impl_simpsum1d_box(ptree, sconfig, eqn, grid, time, utmp, u);
  }
  else if ( "simplesum" == sinit ) { // simple-wave sum init
    if      ( icos != NULLPTR ) {
      bret = ginitv<Types>::impl_simpsum_icos(ptree, sconfig, eqn, grid, time, utmp, u);
    }
    else  {
      bret = ginitv<Types>::impl_simpsum_box (ptree, sconfig, eqn, grid, time, utmp, u);
    }
  } 
  else if ( "random" == sinit ) {
    bret = ginitv<Types>::impl_rand   (ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown velocity initialization method");
  }

  return bret;

} // end, doinitv method


//**********************************************************************************
//**********************************************************************************
// METHOD : doinitdt
// DESC   : Do init of total density component. Full list of available
//          initializations is contained here.
//          Only total density components are passed in.
// ARGS   : ptree  : main property tree
//          sconfig: ptree block name containing scalar config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinitdt(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid,  Time &time, State &utmp, State &u)
{
  GBOOL           bret    = TRUE;
  GString         sinit;
  PropertyTree    vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  if      ( "null"   == sinit
       ||   ""             == sinit ) {
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "random" == sinit ) {
    bret = ginits<Types>::impl_rand(ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown b-field initialization method");
  }

  return bret;
} // end, doinits method


//**********************************************************************************
//**********************************************************************************
// METHOD : doinitmfrac
// DESC   : Do init of massfrac component. Full list of available
//          initializations are contained here.
//          Only d1 components are passed in.
// ARGS   : ptree  : initial condition property tree
//          sconfig: ptree block name containing passive scalar config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinitmfrac(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid,  Time &time, State &utmp, State &u)
{
  GBOOL           bret = FALSE;
  GString         sinit;
  PropertyTree    vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  if      ( "null"   == sinit
       ||   ""             == sinit ) {
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "random" == sinit ) {
    bret = ginits<Types>::impl_rand(ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown b-field initialization method");
  }

  return bret;
} // end, doinitmfrac method


//**********************************************************************************
//**********************************************************************************
// METHOD : doinitenergy
// DESC   : Do init of energy component. Full list of available
//          initializations are contained here.
//          Only d2 components are passed in.
// ARGS   : ptree  : initial condition property tree
//          sconfig: ptree block name containing passive scalar config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinitenergy(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL           bret = FALSE;
  GString         sinit;
  PropertyTree    vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  if      ( "null"   == sinit
       ||   ""             == sinit ) {
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "random" == sinit ) {
    bret = ginits<Types>::impl_rand(ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown b-field initialization method");
  }

  return bret;
} // end, doinitenergy method


//**********************************************************************************
//**********************************************************************************
// METHOD : doinittemp
// DESC   : Do init of temparature component. Full list of available
//          initializations are contained here.
//          Only temperature components are passed in.
// ARGS   : ptree  : initial condition property tree
//          sconfig: ptree block name containing passive scalar config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinittemp(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL           bret = FALSE;
  GString         sinit;
  PropertyTree    vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  if      ( "null"   == sinit
       ||   ""             == sinit ) {
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "random" == sinit ) {
    bret = ginits<Types>::impl_rand(ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown b-field initialization method");
  }

  return bret;
} // end, doinittemp method



//**********************************************************************************
//**********************************************************************************
// METHOD : doinitc
// DESC   : Do init of prescribed components. Full list of available
//          prescribed initializations are contained here. Only
//          prescribed components are passed in.
// ARGS   : ptree  : initial condition property tree
//          sconfig: ptree block name containing prescribed variable config
//          eqn    : equation implementation
//          grid   : grid object
//          time   : initialization time
//          utmp   : tmp arrays
//          u      : state to be initialized. 
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GInitStateFactory<Types>::doinitc(const PropertyTree &ptree, GString &sconfig, EqnBasePtr &eqn, Grid &grid, Time &time, State &utmp, State &u)
{
  GBOOL           bret  = FALSE;
  GString         sinit;
  PropertyTree    vtree = ptree.getPropertyTree(sconfig); 

  sinit = vtree.getValue<GString>("name");

  if      ( "null"   == sinit
       ||   ""             == sinit ) {
    bret = TRUE;
  }
  else if ( "zero" == sinit ) {
    for ( GINT i=0; i<u.size(); i++ ) *u[i] = 0.0;
    bret = TRUE;
  }
  else if ( "random" == sinit ) {
    bret = ginitc<Types>::impl_rand(ptree, sconfig, eqn, grid, time, utmp, u);
  } 
  else {
    assert(FALSE && "Unknown b-field initialization method");
  }

  return bret;
} // end, doinitc method

