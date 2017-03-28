#ifndef MADNESS_PARSEC_INCLUED
#define MADNESS_PARSEC_INCLUED

#include <madness/madness_config.h>

#ifdef HAVE_PARSEC

#include <parsec.h>
#include <parsec/parsec_internal.h>
#include <parsec/devices/device.h>
#include <parsec/execution_unit.h>
#include <parsec/scheduling.h>  
#include <iostream>

namespace madness{
  extern "C"{

    extern const parsec_function_t madness_function;
    extern parsec_handle_t madness_handle;
  }

}

#endif // HAVE_PARSEC

#endif // MADNESS_PARSEC_INCLUED
