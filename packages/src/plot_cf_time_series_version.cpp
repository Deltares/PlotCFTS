//---- LGPL --------------------------------------------------------------------
//
// Copyright (C)  Stichting Deltares, 2011-2015.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation version 2.1.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, see <http://www.gnu.org/licenses/>.
//
// contact: delft3d.support@deltares.nl
// Stichting Deltares
// P.O. Box 177
// 2600 MH Delft, The Netherlands
//
// All indications and logos of, and references to, "Delft3D" and "Deltares"
// are registered trademarks of Stichting Deltares, and remain the property of
// Stichting Deltares. All rights reserved.
//
//------------------------------------------------------------------------------
// $Id: plot_cf_time_series_version.cpp 1771 2019-01-28 07:29:32Z mooiman $
// $HeadURL: file:///D:/SVNrepository/examples/plot_time_series_ugrid/trunk/src/plot_cf_time_series_version.cpp $
//#include <stdio.h>
#include "string.h"
#include "plot_cf_time_series_version.h"

#if defined(WIN64)
#   define strdup _strdup
#endif

#if defined(LINUX64)
static char plot_cf_time_series_version[] = {plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build" (Linux64)"};
static char plot_cf_time_series_version_id[] = {"@(#)Deltares, "plot_cf_time_series_program" Version "plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build" (Linux64), "__DATE__", "__TIME__""};
#elif defined(UCRT64)
static char plot_cf_time_series_version[] = { plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build " (Win64)" };
static char plot_cf_time_series_version_id[] = {"@(#)Deltares, " plot_cf_time_series_program " Version " plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build " (Win64), " __DATE__ ", " __TIME__ "" };
#elif defined(WIN32) || defined(WIN64)
static char plot_cf_time_series_version[] = { plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build " (Win64)" };
static char plot_cf_time_series_version_id[] = {"@(#)Deltares, " plot_cf_time_series_program " Version " plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build " (Win64), " __DATE__ ", " __TIME__ "" };
#else
static char plot_cf_time_series_version[] = {plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build" (Unknown)"};
static char plot_cf_time_series_version_id[] = {"@(#)Deltares, "plot_cf_time_series_program" Version "plot_cf_time_series_major "." plot_cf_time_series_minor "." plot_cf_time_series_revision "." plot_cf_time_series_build" (Unknown), "__DATE__", "__TIME__""};
#endif
static char plot_cf_time_series_company_name[] = {"Deltares"};
static char plot_cf_time_series_program_name[] = { plot_cf_time_series_program };

char * getfullversionstring_plot_cf_time_series(void)
{
    return strdup(plot_cf_time_series_version_id);
};
char * getversionstring_plot_cf_time_series(void)
{
    return strdup(plot_cf_time_series_version);
};
char * getcompanystring_plot_cf_time_series(void)
{
    return strdup(plot_cf_time_series_company_name);
};
char * getprogramstring_plot_cf_time_series(void)
{
    return strdup(plot_cf_time_series_program_name);
};
char * getsourceurlstring_plot_cf_time_series(void)
{
    return strdup(plot_cf_time_series_source_url);
};
