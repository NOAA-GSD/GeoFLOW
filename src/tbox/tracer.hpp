/*
 * tracer.hpp
 *
 *  Created on: Dec 23, 2017
 *      Author: bflynt
 */

#ifndef GEOFLOW_SRC_TBOX_TRACER_HPP_
#define GEOFLOW_SRC_TBOX_TRACER_HPP_

#include "configure.hpp"

/** \file tracer.hpp
 * Implementation of Tracer tools to trace program execution.
 *
 * The Tracer tools can be used to trace program execution by
 * printing to screen a message indented by the nested creation
 * of Tracer classes.  To enable zero runtime cost for release
 * builds a macro is used to only insert Tracer construction
 * when TRACER_ON is defined.
 */
#if defined( GEOFLOW_USE_TRACER )

#include <string>


namespace geoflow {
namespace tbox {

/**
 * \brief Tracer to track program execution to stdout
 *
 * Tracer will trace a programs execution to the screen
 * when a Tracer is constructed and allowed to be destructed
 * when it goes out of scope.
 *
 * Example:
 * \code{.cpp}
 * int MyFunction(){
 *    Tracer my_local_tracer("My Message");
 *
 *    // Program Contents
 * };
 * \endcode
 *
 */
class Tracer {

public:

	Tracer() = delete;
	Tracer(const Tracer& T) = delete;
	Tracer& operator=(const Tracer& T) = delete;

	/**
	 * \brief
	 * Construct with a message and no prefix.
	 */
	Tracer(const std::string message);

	/**
	 * \brief
	 * Construct with a prefix and message.
	 */
	Tracer(const std::string prefix, const std::string message);

	/**
	 * \brief
	 * Destructor will decrease indention
	 */
	~Tracer();

	/**
	 * \brief
	 * Returns a reference to the current line indention
	 * count.
	 *
	 * \details
	 * Returns a reference to the size of the current
	 * line indention.  This is determined by the current
	 * level of nesting.  This is implemented as a static
	 * function since static data members cannot be
	 * initialized within header files.
	 */
	static std::size_t& indent();

private:
	static constexpr std::size_t m_nest_indent = 3;
	static std::size_t m_current_indent;
};

} // namespace tbox
} // namespace geoflow


#ifndef GEOFLOW_TRACE

// This is some crazy magic that helps produce __BASE__247
// Vanilla interpolation of __BASE__##__LINE__ would produce __BASE____LINE__
// Instead this fixes things with macro resolution ordering
// __COUNTER__ may have portability issues so you could use __LINE__ instead
#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res
#define UNIQUE_NAME(base) PP_CAT(base, __COUNTER__)

/**
 * \def TRACER(msg)
 * \brief A macro to insert a tracer with message
 * \param	msg	Message to insert after prefix.
 */

/**
 * \def TRACER_W_PREFIX(pre,msg)
 * \brief A macro to insert a tracer with prefix and message
 * \param	pre	Prefix to insert before message.
 * \param	msg	Message to insert after prefix.
 */
#define GEOFLOW_TRACE() ::geoflow::tbox::Tracer UNIQUE_NAME(trace)(__FUNCTION__);
#define GEOFLOW_TRACE_MSG(msg) ::geoflow::tbox::Tracer UNIQUE_NAME(trace)(msg);
#endif  // #ifndef GEOFLOW_TRACE

#else // #ifdef GEOFLOW_USE_TRACER

/**
 * Macro to ignore Tracer construction
 */
#ifndef GEOFLOW_TRACE
	#define GEOFLOW_TRACE()
	#define GEOFLOW_TRACE_MSG(msg)
#endif

#endif //  GEOFLOW_USE_TRACER


#endif /* GEOFLOW_SRC_TBOX_TRACER_HPP_ */
