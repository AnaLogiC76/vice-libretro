#include "dac.h"
#include "spline.h"
#include <math.h>

#include "resid-config.h"

namespace RESID_NEW_8580_FILTER {
using reSID::PointPlotter;
using reSID::build_dac_table;
using reSID::chip_model;
using reSID::reg4;
using reSID::reg8;
using reSID::reg12;
using reSID::cycle_count;
using reSID::double_point;
using reSID::MOS6581;

#include "filter8580new.cc"

}

