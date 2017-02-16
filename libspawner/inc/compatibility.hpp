#ifndef _SP_COMPATIBILITY_HPP_
#define _SP_COMPATIBILITY_HPP_

#include "inc/options.hpp"
#include "inc/restrictions.hpp"
#include "inc/multibyte.hpp"
#include "platform_report.hpp"

std::string GenerateSpawnerReport(const report_class &rep, const options_class &options, const restrictions_class &restrictions);

void SetRestriction(restrictions_class &restrictions, const restriction_kind_t &restriction_kind, const std::string &value);

#endif // _SP_COMPATIBILITY_HPP_
