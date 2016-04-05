#ifndef _SPAWNER_COMPATIBILITY_H_
#define _SPAWNER_COMPATIBILITY_H_

#include <inc/options.h>
#include <inc/restrictions.h>
#include <inc/report.h>
#include <string>
#include "inc/multibyte.h"
#include "platform.h"

std::string GenerateSpawnerReport(const report_class &rep, const options_class &options, const restrictions_class &restrictions);

void SetRestriction(restrictions_class &restrictions, const restriction_kind_t &restriction_kind, const std::string &value);

#endif//_SPAWNER_COMPATIBILITY_H_
