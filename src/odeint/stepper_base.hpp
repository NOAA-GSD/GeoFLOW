/*
 * stepper_base.hpp
 *
 *  Created on: Nov 16, 2018
 *      Author: bflynt
 */

#ifndef SRC_ODEINT_STEPPER_BASE_HPP_
#define SRC_ODEINT_STEPPER_BASE_HPP_

#include <cstddef>
#include <memory>

namespace geoflow {
namespace odeint {

/**
 * Base class to provided an interface for all time stepping schemes.
 *
 * The base class provides the interface using the strategy pattern
 * for all time stepping schemes.  Time steppers have the ability to
 * take one time step of size dt using the equations provided.  They
 * can not modify the time step size or refuse to take the step.
 */
template<typename EquationType>
class StepperBase {

public:
	using Equation    = EquationType;
	using State       = typename Equation::State;
	using Value       = typename Equation::Value;
	using Derivative  = typename Equation::Derivative;
	using Time        = typename Equation::Time;
	using Jacobian    = typename Equation::Jacobian;
	using Size        = typename Equation::Size;
	using EquationPtr = std::shared_ptr<Equation>;

	StepperBase() = default;
	StepperBase(const StepperBase& si) = default;
	virtual ~StepperBase() = default;
	StepperBase& operator=(const StepperBase& si) = default;

	/**
	 * Constructor taking a pointer to the set of equations.
	 *
	 * @param[in] peqn Pointer to the set of equation to time step
	 */
	StepperBase(const EquationPtr& peqn) :
		eqn_ptr_(peqn){
	}

	/**
	 * Get a pointer to the system of equations the stepper operates on.
	 *
	 * @return Pointer to the system of equations
	 */
	const EquationPtr& getEquationPtr() const{
		return this->eqn_ptr_;
	}

	/**
	 * Return the order of temporal accuracy for the method
	 *
	 * @return Order of temparal accuracy
	 */
	Size order() const{
		return this->order_impl();
	}

	/** Take one step from time t to time t+dt.
	 *
	 * Take exactly one time step from t with current solution u to
	 * time t + dt and return new solution within u.
	 *
	 * \param[in]     t  Current time of state u before taking step
	 * \param[in]     dt Size of time step to take
	 * \param[in,out] u  State of the system of equations
	 */
	void step(const Time& t, const Time& dt, State& u){
		this->step_impl(t,dt,u);
	}

	/** Take one step from time t to time t + dt.
	 *
	 * Take exactly one time step from t with current solution uin to
	 * time t + dt and return new solution within uout.
	 *
	 * \param[in]  t     Current time of state u before taking step
	 * \param[in]  uin   State of the system of equations at time t
	 * \param[in]  dt    Size of time step to take
	 * \param[out] uout  New state of the system of equations at t + dt
	 */
	void step(const Time& t, const State& uin, const Time& dt, State& uout){
		this->step_impl(t,uin,dt,uout);
	}

	/** Take one step from time t to time t + dt using derivative dudt.
	 *
	 * Take exactly one time step from t with current solution u and
	 * derivative dudt to time t + dt and return new solution within u.
	 *
	 * \param[in]     t     Current time of state u before taking step
	 * \param[in]     dt    Size of time step to take
	 * \param[in]     dudt  Derivative dudt at time t
	 * \param[in,out] u     State of the system of equations
	 */
	void step(const Time& t,  const Time& dt, const Derivative& dudt, State& u){
		this->step_impl(t,dt,dudt,u);
	}

	/** Take one step from time t to time t + dt using derivative dudt.
	 *
	 * Take exactly one time step from t with current solution uin and
	 * derivative dudt to time t + dt and return new solution in uout.
	 *
	 * \param[in]  t     Current time of state u before taking step
	 * \param[in]  uin   State of the system of equations at time t
	 * \param[in]  dudt  Derivative dudt at time t
	 * \param[in]  dt    Size of time step to take
	 * \param[out] uout  New state of the system of equations at t + dt
	 */
	void step(const Time& t, const State& uin, const Derivative& dudt, const Time& dt, State& uout){
		this->step_impl(t,uin,dudt,dt,uout);
	}


protected:
	EquationPtr eqn_ptr_;


	/**
	 * Must be provided by implementation
	 */
	virtual Size order_impl() const = 0;

	/**
	 * Must be provided by implementation
	 */
	virtual void step_impl(const Time& t, const Time& dt, State& u) = 0;

	/**
	 * Must be provided by implementation
	 */
	virtual void step_impl(const Time& t, const State& uin, const Time& dt, State& uout) = 0;

	/**
	 * Must be provided by implementation
	 */
	virtual void step_impl(const Time& t, const Time& dt, const Derivative& dudt, State& u) = 0;

	/**
	 * Must be provided by implementation
	 */
	virtual void step_impl(const Time& t, const State& uin, const Derivative& dudt, const Time& dt, State& uout) = 0;

};

} // namespace odeint
} // namespace geoflow

#endif /* SRC_ODEINT_STEPPER_BASE_HPP_ */
