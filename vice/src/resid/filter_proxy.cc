#include "filter_proxy.h"

#include "filter.cc"
#undef RESID_FILTER_CC

namespace RESID_NEW_8580_FILTER {
using ::reSID::PointPlotter;
using ::reSID::build_dac_table;
using ::reSID::double_point;
using ::reSID::MOS6581;

#include "filter8580new.cc"

}

