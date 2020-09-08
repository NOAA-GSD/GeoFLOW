//==================================================================================
// Module       : gdiv.ipp
// Date         : 09/05/20 (DLR)
// Description  : Represents the SEM discretization of the divergence
//                operator,
//                      Div(rho \vec{v})
//                where rho is a scalar field, and  \vec{v} is
//                a vector field. This isn't the strictly conservative
//                form that uses Gauss' theorem to add surfaces fluxes; 
//                it is volume-integrated.
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none
//==================================================================================


//**********************************************************************************
//**********************************************************************************
// METHOD : Constructor method (1)
// DESC   : Default constructor
// ARGS   : none
// RETURNS: none
//**********************************************************************************
template<typename TypePack>
GDivOp<TypePack>::GDivOp(Grid &grid)
:
grid_         (&grid),
massop_       (&grid.massop())
{
  assert(grid_->ntype().multiplicity(0) == GE_MAX-1 
        && "Only a single element type allowed on grid");
} // end of constructor method (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : Destructor method 
// DESC   : 
// ARGS   : none
// RETURNS: none
//**********************************************************************************
template<typename TypePack>
GDivOp<TypePack>::~GDivOp()
{
} // end, destructor


//**********************************************************************************
//**********************************************************************************
// METHOD : apply (1)
// DESC   : Compute application of this operator to scalar
//          field:
//            div = Div (d \vec{v})
//          Remember, normally, this discretization would be multiplied by
//          -1 to represent this operation. We do not apply this sign here.
// ARGS   : d   : scalar field
//          u   : input vector field
//          utmp: array of tmp arrays. 3 arrays required.
//          div : output (result) 
//             
// RETURNS:  none
//**********************************************************************************
template<typename TypePack>
void GDivOp<TypePack>::apply(StateComp &d, State &u, State &utmp, StateComp &div) 
{

  assert( utmp.size() >= 3
       && "Insufficient temp space specified");

  GINT       nxy = grid_->gtype() == GE_2DEMBEDDED ? GDIM+1 : GDIM;
  GElemList *gelems=&grid_->elems();


  // div = D^{T,j} ( d u_j ):
  d.pointProd(*u[0], *utmp[1]);
  grid_->deriv(*utmp[1], 1, FALSE, *utmp[0], div);
  for ( auto j=1; j<nxy; j++ ) { 
     d.pointProd(*u[j], *utmp[1]);
     grid_->deriv(*utmp[1], j+1, FALSE, *utmp[0], *utmp[2]);
     div += *utmp[2];
  }

  div *= *(massop_->data());

} // end of method apply (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : apply (2)
// DESC   : Compute application of this operator to pure vector 
//          field:
//            div = Div (\vec{v})
//          Remember, normally, this discretization would be multiplied by
//          -1 to represent this operation. We do not apply this sign here.
// ARGS   : u   : input vector field
//          utmp: array of tmp arrays. 2 arrays required.
//          div : output (result) 
//             
// RETURNS:  none
//**********************************************************************************
template<typename TypePack>
void GDivOp<TypePack>::apply(State &u, State &utmp, StateComp &div) 
{

  assert( utmp.size() >= 2
       && "Insufficient temp space specified");

  GINT       nxy = grid_->gtype() == GE_2DEMBEDDED ? GDIM+1 : GDIM;
  GElemList *gelems=&grid_->elems();


  // div = D^{T,j} ( u_j ):
  grid_->deriv(*u[0], 1, TRUE , *utmp[0], div);
  for ( auto j=1; j<nxy; j++ ) { 
     grid_->deriv(*u[j], j+1, TRUE , *utmp[0], *utmp[1]);
     div += *utmp[1];
  }

  div *= *(massop_->data());

} // end of method apply (1)

