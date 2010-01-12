/*
  This file is part of MADNESS.
  
  Copyright (C) 2007,2010 Oak Ridge National Laboratory
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
  
  For more information please contact:
  
  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367
  
  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680
  
  $Id$
*/

  
/*!
	\file navstokes_cosines.cc
	\brief Example Solving the Navier-Stokes equations 
	\defgroup examplense Solves a Navier-Stokes equation 
	\ingroup examples

  The source is <a href=http://code.google.com/p/m-a-d-n-e-s-s/source/browse/local/trunk/src/apps/examples/navstokes_cosines.cc >here</a>.

  \par Points of interest
  - convolution with the periodic Green's function (Possion kernel and Modified Helmheltz/Bound State Helmheltz/Yukawa kernel)
  - output data to vtk for ParaView
  
  \par Background
  This illustrates solution of a Navier-Stokes equation for incompressible flows,
  \f[
   u_t - u \cdot \grad u + \grad p = \mu \Delta u + f
   \grad \cdot u = 0
  \f]
  where the force and the viscocity  \f$ f and \mu \f$ are given in the code.

  \par Implementation
    
    Step 1.  Calculate the pressure at time n+1 explicitly.
	            \Delta p = \grad \cdot (f - u_{n} \cdot \grad u_{n} ) 
	Step 2.  Calculate the velocity at time n+1.
	            \frac{1}{ \delta t \mu } - \Delta) u_{n+1} = \frac {f - \grad p +u_{n}}{ \mu } 
	
	The resulting method is a first order in time scheme and can be used with Spectral/Krylov deferred correction to construct higher order methods.
	Particularly, the construction under this frame is easy and semilar to the Crank-Nicolson technique.
*/


#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include <mra/vmra.h>
//#include <mra/poperator.h>
#include "poperator.h"

using namespace madness;

typedef Vector<double, 3> coordT3d;
typedef Vector<double, 1> coordT1d;
typedef Function<double, 3> functionT;
typedef std::vector<functionT> functT;

const double L = 2*WST_PI;
const double N = 8.0;

const double mu = 1; // Effective Viscosity
const double deltaT = 0.005; // Size of time step
const int Nts = L/deltaT+10; // Number of time steps
const int k = 10; // Wavelet order (usually precision + 2)
const double pthresh = 1.e-6; // Precision
const double pthresh1 = 1e-7;// * pthresh;
const double uthresh = pthresh; // Precision
const double uthresh1 = pthresh1;


double mytime = 0.0; // Global variable for the current time
// This should be passed in thru the class or app context
const double cc = 1;// L/(deltaT*Nts)/2;

template<typename T, int NDIM>
void plotvtk_begin(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary);

template<typename T, int NDIM>
void plotvtk_data(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary);

template<typename T, int NDIM>
void plotvtk_end(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary);

//*****************************************************************************
static double init_zero(const coordT3d& r) {
	return 0.0;
}
//*****************************************************************************
//*****************************************************************************
                   
static double uxexact(const coordT3d& r) {
	const double x=r[0]+cc*mytime, y = r[1], z = r[2];
	double t = mytime;

	return cos(.5*t) * sin(x) * sin(x) * (sin(2. * y) * sin(z) * sin(z) - sin(y)
			* sin(y) * sin(2. * z));
	
}
//*****************************************************************************
//*****************************************************************************
static double uyexact(const coordT3d& r) {
	const double x=r[0]+cc*mytime, y = r[1], z = r[2];
	double t = mytime;

	return cos(.5*t) * sin(y) * sin(y) * (sin(2. * z) * sin(x) * sin(x) - sin(z)
			* sin(z) * sin(2. * x));
}
//*****************************************************************************
//*****************************************************************************
static double uzexact(const coordT3d& r) {
	const double x=r[0]+cc*mytime, y = r[1], z = r[2];
	double t = mytime;

	return cos(.5*t) * sin(z) * sin(z) * (sin(2. * x) * sin(y) * sin(y) - sin(x)
			* sin(x) * sin(2. * y));
}
//*****************************************************************************
//*****************************************************************************
static double fxexact(const coordT3d& r)
{
  const double x=r[0], y=r[1], z=r[2];
        double t = mytime ;

 return -sin(t / 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) / 0.2e1 + 0.2e1 * cos(t / 0.2e1) * sin(x + cc * t) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) * cos(x + cc * t) * cc - cos(t / 0.2e1) * sin(x + cc * t) * sin(y) * cos(z) + 0.2e1 * pow(cos(t / 0.2e1), 0.2e1) * pow(sin(x + cc * t), 0.3e1) * pow(sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z), 0.2e1) * cos(x + cc * t) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(y), 0.2e1) * (sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) * pow(sin(x + cc * t), 0.2e1) * (0.2e1 * cos(0.2e1 * y) * pow(sin(z), 0.2e1) - 0.2e1 * sin(y) * sin(0.2e1 * z) * cos(y)) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(z), 0.2e1) * (sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) * pow(sin(x + cc * t), 0.2e1) * (0.2e1 * sin(0.2e1 * y) * sin(z) * cos(z) - 0.2e1 * pow(sin(y), 0.2e1) * cos(0.2e1 * z)) - mu * (0.2e1 * cos(t / 0.2e1) * pow(cos(x + cc * t), 0.2e1) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) - 0.2e1 * cos(t / 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) + cos(t / 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (-0.4e1 * sin(0.2e1 * y) * pow(sin(z), 0.2e1) - 0.2e1 * pow(cos(y), 0.2e1) * sin(0.2e1 * z) + 0.2e1 * pow(sin(y), 0.2e1) * sin(0.2e1 * z)) + cos(t / 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (0.2e1 * sin(0.2e1 * y) * pow(cos(z), 0.2e1) - 0.2e1 * sin(0.2e1 * y) * pow(sin(z), 0.2e1) + 0.4e1 * pow(sin(y), 0.2e1) * sin(0.2e1 * z)));
}
//*****************************************************************************
//*****************************************************************************
static double fyexact(const coordT3d& r)
{
  const double x=r[0], y=r[1], z=r[2];
        double t = mytime ;

  return -sin(t / 0.2e1) * pow(sin(y), 0.2e1) * (sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) / 0.2e1 + cos(t / 0.2e1) * pow(sin(y), 0.2e1) * (0.2e1 * sin(0.2e1 * z) * sin(x + cc * t) * cos(x + cc * t) * cc - 0.2e1 * pow(sin(z), 0.2e1) * cos(0.2e1 * x + 0.2e1 * cc * t) * cc) + cos(t / 0.2e1) * cos(x + cc * t) * cos(y) * cos(z) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) * pow(sin(y), 0.2e1) * (0.2e1 * sin(0.2e1 * z) * sin(x + cc * t) * cos(x + cc * t) - 0.2e1 * pow(sin(z), 0.2e1) * cos(0.2e1 * x + 0.2e1 * cc * t)) + 0.2e1 * pow(cos(t / 0.2e1), 0.2e1) * pow(sin(y), 0.3e1) * pow(sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t), 0.2e1) * cos(y) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(z), 0.2e1) * (sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) * pow(sin(y), 0.2e1) * (0.2e1 * cos(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - 0.2e1 * sin(z) * sin(0.2e1 * x + 0.2e1 * cc * t) * cos(z)) - mu * (cos(t / 0.2e1) * pow(sin(y), 0.2e1) * (0.2e1 * sin(0.2e1 * z) * pow(cos(x + cc * t), 0.2e1) - 0.2e1 * sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) + 0.4e1 * pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) + 0.2e1 * cos(t / 0.2e1) * pow(cos(y), 0.2e1) * (sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) - 0.2e1 * cos(t / 0.2e1) * pow(sin(y), 0.2e1) * (sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) + cos(t / 0.2e1) * pow(sin(y), 0.2e1) * (-0.4e1 * sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - 0.2e1 * pow(cos(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t) + 0.2e1 * pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)));

}
//*****************************************************************************
//*****************************************************************************
static double fzexact(const coordT3d& r)
{
  const double x=r[0], y=r[1], z=r[2];
        double t = mytime ;

  return -sin(t / 0.2e1) * pow(sin(z), 0.2e1) * (sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) / 0.2e1 + cos(t / 0.2e1) * pow(sin(z), 0.2e1) * (0.2e1 * cos(0.2e1 * x + 0.2e1 * cc * t) * cc * pow(sin(y), 0.2e1) - 0.2e1 * sin(x + cc * t) * sin(0.2e1 * y) * cos(x + cc * t) * cc) - cos(t / 0.2e1) * cos(x + cc * t) * sin(y) * sin(z) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(x + cc * t), 0.2e1) * (sin(0.2e1 * y) * pow(sin(z), 0.2e1) - pow(sin(y), 0.2e1) * sin(0.2e1 * z)) * pow(sin(z), 0.2e1) * (0.2e1 * cos(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - 0.2e1 * sin(x + cc * t) * sin(0.2e1 * y) * cos(x + cc * t)) + pow(cos(t / 0.2e1), 0.2e1) * pow(sin(y), 0.2e1) * (sin(0.2e1 * z) * pow(sin(x + cc * t), 0.2e1) - pow(sin(z), 0.2e1) * sin(0.2e1 * x + 0.2e1 * cc * t)) * pow(sin(z), 0.2e1) * (0.2e1 * sin(0.2e1 * x + 0.2e1 * cc * t) * sin(y) * cos(y) - 0.2e1 * pow(sin(x + cc * t), 0.2e1) * cos(0.2e1 * y)) + 0.2e1 * pow(cos(t / 0.2e1), 0.2e1) * pow(sin(z), 0.3e1) * pow(sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y), 0.2e1) * cos(z) - mu * (cos(t / 0.2e1) * pow(sin(z), 0.2e1) * (-0.4e1 * sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - 0.2e1 * pow(cos(x + cc * t), 0.2e1) * sin(0.2e1 * y) + 0.2e1 * pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) + cos(t / 0.2e1) * pow(sin(z), 0.2e1) * (0.2e1 * sin(0.2e1 * x + 0.2e1 * cc * t) * pow(cos(y), 0.2e1) - 0.2e1 * sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) + 0.4e1 * pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) + 0.2e1 * cos(t / 0.2e1) * pow(cos(z), 0.2e1) * (sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)) - 0.2e1 * cos(t / 0.2e1) * pow(sin(z), 0.2e1) * (sin(0.2e1 * x + 0.2e1 * cc * t) * pow(sin(y), 0.2e1) - pow(sin(x + cc * t), 0.2e1) * sin(0.2e1 * y)));
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
static double pexact(const coordT3d& r) {
	const double x=r[0]+cc*mytime, y = r[1], z = r[2];
	double t = mytime;

	return cos(.5*t) * cos(x) * sin(y) * cos(z);
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

inline functionT div(const functT& uint) {
	return diff(uint[0], 0) + diff(uint[1], 1) + diff(uint[2], 2);
}

inline functionT lap(const functionT& uint) {
	return diff(diff(uint, 0),0) + diff(diff(uint,1), 1) + diff(diff(uint,2), 2);
}

Tensor<int> bc(3, 2), bc0(3, 2);

World *pworld;
#define myfun std::vector< Function<T,NDIM> >

template<typename T, int NDIM> void adv(const myfun& uu, myfun& advu) {
	for (int i=0; i < 3; ++i)  advu[i] = diff(uu[0]*uu[i],0) + diff(uu[1]*uu[i],1) + diff(uu[2]*uu[i],2);
}

template<typename T, int NDIM> inline myfun operator-(const myfun& l, const myfun& r) { return sub(*pworld, l, r); }

void testNavierStokes(int argc, char**argv) {
	initialize(argc, argv);
	try {
	World world(MPI::COMM_WORLD);
	
	pworld = &world;
	startup(world, argc, argv);

	// Function defaults
	FunctionDefaults<3>::set_k(k);
	FunctionDefaults<3>::set_cubic_cell(0.0, L);
	FunctionDefaults<3>::set_thresh(pthresh);
	//FunctionDefaults<3>::set_initial_level(4);

	bc = 1;
	bc0 = 0;

	FunctionDefaults<3>::set_bc(bc0);
	
	Tensor<double> cellsize = FunctionDefaults<3>::get_cell_width();
	SeparatedConvolution<double, 3> op = PeriodicCoulombOp<double, 3> (world,
			k, pthresh1, pthresh1, cellsize);

	double const dum = 1 / deltaT / mu;
	SeparatedConvolution<double, 3> op1 = PeriodicBSHOp<double, 3> (world,
			sqrt(dum), k, uthresh1, uthresh1, cellsize);

	FunctionDefaults<3>::set_bc(bc);

	// Initialize the old solution and print out to vts files
	mytime = 0.0;

	functT u(3);
        functT rhs(3);
	functT f(3);
	u[0] = FunctionFactory<double, 3> (world).f(uxexact  ) .truncate_on_project();
	u[1] = FunctionFactory<double, 3> (world).f(uyexact  ) .truncate_on_project();
	u[2] = FunctionFactory<double, 3> (world).f(uzexact  ) .truncate_on_project();

	Function<double, 3> divu = div(u);
   	//char filename[100];
	//sprintf(filename, "data-init-vel.vts");
        //plotvtk_begin(u[0], "u", world, filename, 0.0, L, 21, false);
        //plotvtk_data(u[0], "u", world, filename, 0.0, L, 21, false);
        //plotvtk_data(u[1], "v", world, filename, 0.0, L, 21, false);
        //plotvtk_data(u[2], "w", world, filename, 0.0, L, 21, false);
        //plotvtk_end(u[0], "u", world, filename, 0.0, L, 21, false);
	double divun=divu.norm2();
	int dd=divu.max_depth();
	if (world.rank()==0) print("initial div, depth:", divun, dd);

	for (int t =  0; t < Nts; t++) {
		mytime = deltaT*(t+1);
		//if (world.rank()==0) print("current time: ", mytime);
		// Step 1.  Calculate the pressure at time t+1.
		//            Laplace p = div (f)
		f[0] = FunctionFactory<double, 3> (world).f(fxexact).truncate_on_project();
		f[1] = FunctionFactory<double, 3> (world).f(fyexact).truncate_on_project();
		f[2] = FunctionFactory<double, 3> (world).f(fzexact).truncate_on_project();
		
		
		adv(u, rhs);
		//for (int i=0; i < 3; ++i)  rhs[i] = u[0]*diff(u[i],0) + u[1]*diff(u[i],1) + u[2]*diff(u[i],2);
		
		functionT divf = div(f-rhs);

		FunctionDefaults<3>::set_bc(bc0);
		divf.set_bc(bc0);
		Function<double,3> p = apply(op, divf);
		p.scale(-1. / (4. * WST_PI)).set_bc(bc);
		divf.set_bc(bc);
		FunctionDefaults<3>::set_bc(bc);

		//if (world.rank()==0) print("1 step done");
		//if (world.rank()==0) print("poission solving error:", (lap(p)-divf).norm2());

		// Step 2.  Calculate the velocity at time t+1.
		//            (1/(deltaT mu) - Laplace) u_t+1 = (f - grad p)/mu + u_t/(deltaT mu)

		//~ rhs[0] = (f[0] - diff(p, 0) -rhs[0])*(1. / mu) + u[0]*dum;
		//~ rhs[1] = (f[1] - diff(p, 1) -rhs[1])*(1. / mu) + u[1]*dum;
		//~ rhs[2] = (f[2] - diff(p, 2) -rhs[2])*(1. / mu) + u[2]*dum;
		f[0] -= diff(p,0);
		f[1] -= diff(p,1);
		f[2] -= diff(p,2);
		gaxpy(world, 1, rhs, -1, f);
		gaxpy(world, -1./mu, rhs, dum, u);

		FunctionDefaults<3>::set_bc(bc0);
		for (int i = 0; i < 3; ++i)  rhs[i].set_bc(bc0);
		functT ue = apply(world, op1, rhs);
		for (int i = 0; i < 3; ++i)  {
			ue[i].set_bc(bc);
			rhs[i].set_bc(bc);
		}
		FunctionDefaults<3>::set_bc(bc);
		
		//u = ue;  // use this line for first order/mixed Euler's method
		
		gaxpy(world,-1,u,2,ue); ++t; mytime += deltaT; //for (int i=0; i < 3; ++i) u[i] = 2.0*ue[i] - u[i];// += (mu*lap(ue[i])-(ue[0]*diff(ue[i],0) + ue[1]*diff(ue[i],1) + ue[2]*diff(ue[i],2)) - diff(p,i) + f[i])*(2*deltaT);
		//use the above line for second order/Crank-Nicolson like scheme
		//note that in this case, the time-step size is instead 2*deltaT
		

		if ( (t%10)==0) {
				char filename[256];
				sprintf(filename, "data-%02d.vts", t);
				plotvtk_begin(u[0], "u", world, filename, 0.0, L, 21, false);
				plotvtk_data(u[0], "u", world, filename, 0.0, L, 21, false);
				plotvtk_data(u[1], "v", world, filename, 0.0, L, 21, false);
				plotvtk_data(u[2], "w", world, filename, 0.0, L, 21, false);
				plotvtk_data(p, "p", world, filename, 0.0, L, 21, false);
				plotvtk_end(u[0], "u", world, filename, 0.0, L, 21, false);
		}


		Function<double, 3> du = FunctionFactory<double, 3> (world).f(uxexact).truncate_on_project();
		du -= u[0];
		Function<double, 3> dv = FunctionFactory<double, 3> (world).f(uyexact).truncate_on_project();
		dv -= u[1];
		Function<double, 3> dw = FunctionFactory<double, 3> (world).f(uzexact).truncate_on_project(); 
		dw -= u[2];
		
		{double  a=div(u).norm2(), b=du.norm2(), c=dv.norm2(),d=dw.norm2();
		if (world.rank()==0)  print(t+1, mytime, a,b,c,d);
		}
			
		// Print out the solution
		//sprintf(filename, "data-%02d.vts", t);
		//plotvtk_begin(u[0], "u", world, filename, 0.0, L, 21, false);
		//plotvtk_data(u[0], "u", world, filename, 0.0, L, 21, false);
		//plotvtk_data(u[1], "v", world, filename, 0.0, L, 21, false);
		//plotvtk_data(u[2], "w", world, filename, 0.0, L, 21, false);
		//plotvtk_data(ue[0], "ue", world, filename, 0.0, L, 21, false);
		//plotvtk_data(ue[1], "ve", world, filename, 0.0, L, 21, false);
		//plotvtk_data(ue[2], "we", world, filename, 0.0, L, 21, false);
		//plotvtk_data(p, "p", world, filename, 0.0, L, 21, false);
		//plotvtk_end(u[0], "u", world, filename, 0.0, L, 21, false);
	}    
	
//	RMI::end();
	} catch (const MPI::Exception& e) {
        //        print(e);
        error("caught an MPI exception");
    } catch (const madness::MadnessException& e) {
        print(e);
        error("caught a MADNESS exception");
    } catch (const madness::TensorException& e) {
        print(e);
        error("caught a Tensor exception");
    } catch (const char* s) {
        print(s);
        error("caught a c-string exception");
    } catch (char* s) {
        print(s);
        error("caught a c-string exception");
    } catch (const std::string& s) {
        print(s);
        error("caught a string (class) exception");
    } catch (const std::exception& e) {
        print(e.what());
        error("caught an STL exception");
    } catch (...) {
        error("caught unhandled exception");
    };
    finalize();
}
//*****************************************************************************

//*****************************************************************************
int main(int argc, char**argv) {
	testNavierStokes(argc, argv);
	return 0;
}
//*****************************************************************************

template<typename T, int NDIM>
void plotvtk_begin(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary) {
	PROFILE_FUNC;
	MADNESS_ASSERT(NDIM <= 6);

	Tensor<double> cell(3, 2);
	cell(_, 0) = Lplotl;
	cell(_, 1) = Lploth;

	FILE *f = 0;
	if (world.rank() == 0) {
		f = fopen(filename, "w");
		if (!f)
			MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

		fprintf(
				f,
				"<VTKFile type=\"StructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">\n");
		fprintf(f, "  <StructuredGrid WholeExtent=\"0 %d 0 %d 0 %d\">\n", npt
				- 1, npt - 1, npt - 1);
		fprintf(f, "  <Piece Extent=\"0 %d 0 %d 0 %d\">\n", npt - 1, npt - 1,
				npt - 1);

		fprintf(f, "      <Points>\n");
		fprintf(
				f,
				"        <DataArray NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\">\n");

		double coordx = cell(0, 0);
		double coordy = cell(1, 0);
		double coordz = cell(2, 0);

		double spacex = (cell(0, 1) - cell(0, 0)) / (npt - 1);
		double spacey = (cell(1, 1) - cell(1, 0)) / (npt - 1);
		double spacez = (cell(2, 1) - cell(2, 0)) / (npt - 1);

		for (int i = 0; i < npt; i++) {
			coordy = cell(1, 0);
			for (int j = 0; j < npt; j++) {
				coordz = cell(2, 0);
				for (int k = 0; k < npt; k++) {
					fprintf(f, "%f %f %f\n", coordx, coordy, coordz);
					coordz = coordz + spacez;
				}
				coordy = coordy + spacey;
			}
			coordx = coordx + spacex;
		}
		fprintf(f, "        </DataArray>\n");
		fprintf(f, "      </Points>\n");
		fprintf(f, "      <PointData>\n");
		fclose(f);
	}
	world.gop.fence();
}

template<typename T, int NDIM>
void plotvtk_end(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary) {
	PROFILE_FUNC;
	MADNESS_ASSERT(NDIM <= 6);

	FILE *f = 0;
	if (world.rank() == 0) {
		f = fopen(filename, "a");
		if (!f)
			MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

		fprintf(f, "      </PointData>\n");
		fprintf(f, "      <CellData>\n");
		fprintf(f, "      </CellData>\n");
		fprintf(f, "    </Piece>\n");
		fprintf(f, "  </StructuredGrid>\n");
		fprintf(f, "</VTKFile>\n");
		fclose(f);
	}
	world.gop.fence();
}

template<typename T, int NDIM>
void plotvtk_data(const Function<T, NDIM>& function, const char* fieldname,
		World& world, const char* filename, double Lplotl, double Lploth,
		int npt, bool binary) {
	PROFILE_FUNC;
	MADNESS_ASSERT(NDIM <= 6);

	Tensor<double> cell(3, 2);
	cell(_, 0) = Lplotl;
	cell(_, 1) = Lploth;
	std::vector<long> numpt(3, npt);

	world.gop.barrier();

	function.verify();
	FILE *f = 0;
	if (world.rank() == 0) {
		f = fopen(filename, "a");
		if (!f)
			MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

		fprintf(
				f,
				"        <DataArray Name=\"%s\" format=\"ascii\" type=\"Float32\" NumberOfComponents=\"1\">\n",
				fieldname);
	}

	world.gop.fence();
	Tensor<T> tmpr = function.eval_cube(cell, numpt);
	world.gop.fence();

	if (world.rank() == 0) {
		for (IndexIterator it(numpt); it; ++it) {
			fprintf(f, "%.6e\n", tmpr(*it));
		}
		fprintf(f, "        </DataArray>\n");

		fclose(f);
	}
	world.gop.fence();
}
