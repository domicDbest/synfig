/* === S Y N F I G ========================================================= */
/*!	\file valuenode_dynamic.h
**	\brief Header file for implementation of the "Dynamic" valuenode conversion.
**
**	$Id$
**
**	\legal
**	Copyright (c) 2014 Carlos López
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_VALUENODE_DYNAMIC_H
#define __SYNFIG_VALUENODE_DYNAMIC_H

/* === H E A D E R S ======================================================= */

#include "valuenode.h"
#include "valuenode_derivative.h"
#include "valuenode_const.h"

/* === M A C R O S ========================================================= */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {

class Oscillator;

class ValueNode_Dynamic : public LinkableValueNode
{
	friend class Oscillator;
private:

	ValueNode::RHandle tip_static_;    // Equilibrium position without external forces
	ValueNode::RHandle origin_;        // Basement of the dynamic system
	ValueNode::RHandle force_;         // External force applied on the mass center of gravity
	ValueNode::RHandle damping_coef_;  // Radial Damping coeficient 
	ValueNode::RHandle friction_coef_; // Rotational friction coeficient
	ValueNode::RHandle spring_coef_;   // Spring coeficient 
	ValueNode::RHandle torsion_coef_;  // Torsion coeficient
	ValueNode::RHandle mass_;          // Mass 
	ValueNode::RHandle inertia_;       // Moment of Inertia


	ValueNode_Derivative::RHandle origin_d_;      // Derivative of the origin along the time
	mutable Time last_time;
	ValueNode_Dynamic(const ValueBase &value);
		/*
		State types (4) for:
		q=radius
		p=d/dt(radius)
		b=angle
		g=d/dt(angle)

		where

		p=dxdt[0]
		p'=dxdt[1]
		g=dxdt[2]
		g'=dxdt[3]
		q=x[0]
		q'=x[1]
		b=x[2]
		b'=x[3]
		*/
	mutable std::vector<double> state;
	void reset_state(Time t)const;
public:

	typedef etl::handle<ValueNode_Dynamic> Handle;
	typedef etl::handle<const ValueNode_Dynamic> ConstHandle;

	virtual ValueBase operator()(Time t)const;

	virtual ~ValueNode_Dynamic();

	virtual String get_name()const;
	virtual String get_local_name()const;

	virtual ValueNode::LooseHandle get_link_vfunc(int i)const;

protected:
	LinkableValueNode* create_new()const;
	virtual bool set_link_vfunc(int i,ValueNode::Handle x);

public:
	using synfig::LinkableValueNode::get_link_vfunc;

	using synfig::LinkableValueNode::set_link_vfunc;
	static bool check_type(ValueBase::Type type);
	static ValueNode_Dynamic* create(const ValueBase &x);
	virtual Vocab get_children_vocab_vfunc()const;
}; // END of class ValueNode_Dynamic


class Oscillator
{
	etl::handle<const ValueNode_Dynamic> d;
public:
    Oscillator(const ValueNode_Dynamic* x) : d(x) { }
    void operator() ( const std::vector<double> &x , std::vector<double> &dxdt , const double t )
	{
		Vector u(cos(x[2]), sin(x[2]));
		Vector v(-u[1], u[0]);
		Vector s=(*(d->origin_))(t).get(Vector());
		Vector sd=(*(d->origin_d_))(t).get(Vector());
		Vector f=(*(d->force_))(t).get(Vector());
		double c=(*(d->damping_coef_))(t).get(double());
		double mu=(*(d->friction_coef_))(t).get(double());
		double k=(*(d->spring_coef_))(t).get(double());
		double tau=(*(d->torsion_coef_))(t).get(double());
		double m=(*(d->mass_))(t).get(double());
		double i=(*(d->inertia_))(t).get(double());
		Vector tip=(*(d->tip_static_))(t).get(Vector());
	
		double fr=f*u;
		double fa=f*v;
		// Those are the second derivatives (speed of origin)
		double srd=sd*u;
		double sad=sd*v;
		// Calculate the steady position in terms of state
		double r0=tip.mag();
		double a0=(double)(Angle::rad(tip.angle()).get());
		// Check if the spring is constant and zero. It means no spring (riggid)
		bool spring_is_riggid=(
			(ValueNode_Const::Handle::cast_dynamic(d->spring_coef_)
				&&
			k ==0.0)
				||
			m ==0,0);
		// Check if the torsion is constant and zero. It means no torsion (riggid)
		bool torsion_is_riggid=(
			(ValueNode_Const::Handle::cast_dynamic(d->torsion_coef_)
				&&
			mu ==0.0)
				||
			i ==0.0);
		// Integration operations
		dxdt[0]=x[1];
		if(spring_is_riggid)
			dxdt[1]=0.0;
		else
			dxdt[1]=(fr-c*x[1]-k*(x[0]-r0))/m-srd;
		dxdt[2]=x[3];
		if(torsion_is_riggid)
			dxdt[3]=0.0;
		else
			dxdt[3]=(fa*x[0]-mu*x[3]-tau*(x[2]-a0))/i-sad;
	}

};
}; // END of namespace synfig

/* === E N D =============================================================== */

#endif
