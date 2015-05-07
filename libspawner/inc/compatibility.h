#ifndef _SPAWNER_COMPATIBILITY_H_
#define _SPAWNER_COMPATIBILITY_H_

#include <inc/platform.h>
#include <inc/options.h>
#include <inc/restrictions.h>
#include <inc/report.h>
#include <string>

std::string ExitCodeToString(const unsigned int &code);

std::string GenerateSpawnerReport(const report_class &rep, const options_class &options, const restrictions_class &restrictions);

void ReadEnvironmentVariables(options_class &options, restrictions_class &restrictions);

void SetRestriction(restrictions_class &restrictions, const restriction_kind_t &restriction_kind, const std::string &value);

#endif//_SPAWNER_COMPATIBILITY_H_
