// 
// File:          Ex1_System_Impl.hxx
// Symbol:        Ex1.System-v0.0.0
// Symbol Type:   class
// Babel Version: 1.0.4
// Description:   Server-side implementation for Ex1.System
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// 

#ifndef included_Ex1_System_Impl_hxx
#define included_Ex1_System_Impl_hxx

#ifndef included_sidl_cxx_hxx
#include "sidl_cxx.hxx"
#endif
#ifndef included_Ex1_System_IOR_h
#include "Ex1_System_IOR.h"
#endif
#ifndef included_Ex1_System_hxx
#include "Ex1_System.hxx"
#endif
#ifndef included_TOPS_System_Compute_Residual_hxx
#include "TOPS_System_Compute_Residual.hxx"
#endif
#ifndef included_gov_cca_CCAException_hxx
#include "gov_cca_CCAException.hxx"
#endif
#ifndef included_gov_cca_Component_hxx
#include "gov_cca_Component.hxx"
#endif
#ifndef included_gov_cca_Services_hxx
#include "gov_cca_Services.hxx"
#endif
#ifndef included_gov_cca_ports_GoPort_hxx
#include "gov_cca_ports_GoPort.hxx"
#endif
#ifndef included_sidl_BaseClass_hxx
#include "sidl_BaseClass.hxx"
#endif
#ifndef included_sidl_BaseInterface_hxx
#include "sidl_BaseInterface.hxx"
#endif
#ifndef included_sidl_ClassInfo_hxx
#include "sidl_ClassInfo.hxx"
#endif
#ifndef included_sidl_RuntimeException_hxx
#include "sidl_RuntimeException.hxx"
#endif


// DO-NOT-DELETE splicer.begin(Ex1.System._includes)
#include "TOPS.hxx"
// DO-NOT-DELETE splicer.end(Ex1.System._includes)

namespace Ex1 { 

  /**
   * Symbol "Ex1.System" (version 0.0.0)
   */
  class System_impl : public virtual ::Ex1::System 
  // DO-NOT-DELETE splicer.begin(Ex1.System._inherits)
  // Insert-Code-Here {Ex1.System._inherits} (optional inheritance here)
  // DO-NOT-DELETE splicer.end(Ex1.System._inherits)
  {

  // All data marked protected will be accessable by 
  // descendant Impl classes
  protected:

    bool _wrapped;

    // DO-NOT-DELETE splicer.begin(Ex1.System._implementation)
    TOPS::Structured::Solver solver;
    gov::cca::Services myServices;
    // DO-NOT-DELETE splicer.end(Ex1.System._implementation)

  public:
    // default constructor, used for data wrapping(required)
    System_impl();
    // sidl constructor (required)
    // Note: alternate Skel constructor doesn't call addref()
    // (fixes bug #275)
    System_impl( struct Ex1_System__object * s ) : StubBase(s,true), _wrapped(
      false) { _ctor(); }

    // user defined construction
    void _ctor();

    // virtual destructor (required)
    virtual ~System_impl() { _dtor(); }

    // user defined destruction
    void _dtor();

    // true if this object was created by a user newing the impl
    inline bool _isWrapped() {return _wrapped;}

    // static class initializer
    static void _load();

  public:

    /**
     * user defined non-static method.
     */
    void
    computeResidual_impl (
      /* in array<double> */::sidl::array<double> x,
      /* in array<double> */::sidl::array<double> f
    )
    ;


    /**
     *  Starts up a component presence in the calling framework.
     * @param services the component instance's handle on the framework world.
     * Contracts concerning Svc and setServices:
     * 
     * The component interaction with the CCA framework
     * and Ports begins on the call to setServices by the framework.
     * 
     * This function is called exactly once for each instance created
     * by the framework.
     * 
     * The argument Svc will never be nil/null.
     * 
     * Those uses ports which are automatically connected by the framework
     * (so-called service-ports) may be obtained via getPort during
     * setServices.
     */
    void
    setServices_impl (
      /* in */::gov::cca::Services services
    )
    // throws:
    //     ::gov::cca::CCAException
    //     ::sidl::RuntimeException
    ;


    /**
     *  
     * Execute some encapsulated functionality on the component. 
     * Return 0 if ok, -1 if internal error but component may be 
     * used further, and -2 if error so severe that component cannot
     * be further used safely.
     */
    int32_t
    go_impl() ;
  };  // end class System_impl

} // end namespace Ex1

// DO-NOT-DELETE splicer.begin(Ex1.System._misc)
// Insert-Code-Here {Ex1.System._misc} (miscellaneous things)
// DO-NOT-DELETE splicer.end(Ex1.System._misc)

#endif
