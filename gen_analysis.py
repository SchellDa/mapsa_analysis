#!/usr/bin/env python2

import os
import sys
import re

header_template = """
#ifndef {guard}
#define {guard}

#include "analysis.h"

class {class} : public core::Analysis
{{
public:
        {class}();
	virtual ~{class}();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        bool analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::MPAStreamReader::event_t& mpa_event);
\tvoid finish();
}};

#endif//{guard}
"""

source_template = """
#include "{header}"

REGISTER_ANALYSIS_TYPE({class}, "Textual analysis description here.")

{class}::{class}() :
 Analysis()
{{
\taddProcess("analyze", CS_ALWAYS /* CS_TRACK */,
\t           core::Analysis::init_callback_t {{}},
\t           core::Analysis::run_init_callback_t {{}},
\t           std::bind(&{class}::analyze, this, std::placeholders::_1, std::placeholders::_2),
\t           core::Analysis::run_post_callback_t {{}}
\t           std::bind(&{class}::finish, this)
\t           );
}}

{class}::~{class}()
{{
}}

void {class}::init(const po::variables_map& vm)
{{
}}

std::string {class}::getUsage(const std::string& argv0) const
{{
	return Analysis::getUsage(argv0);
}}

std::string {class}::getHelp(const std::string& argv0) const
{{
        return Analysis::getHelp(argv0);
}}

bool {class}::analyze(const core::TrackStreamReader::event_t& track_event,
                      const core::MPAStreamReader::event_t& mpa_event)
{{
\treturn true;
}}

void {class}::finish()
{{
}}
"""

def unCamelCase(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def genClass(name):
    header_file = unCamelCase(name)+".h"
    source_file = unCamelCase(name)+".cpp"
    guard_define = unCamelCase(name).upper() + "_H"
    if os.path.exists(header_file):
        sys.stderr.write("Header file {} already exists! "
                "Aborting...\n".format(header_file))
        sys.exit(1)
    if os.path.exists(source_file):
        sys.stderr.write("Source file {} already exists! "
                "Aborting...\n".format(header_file))
        sys.exit(1)
    variables = {
        "class": name,
        "guard": guard_define,
        "header": header_file,
        "source": source_file
    }
    with open(header_file, "w") as f:
        f.write(header_template.format(**variables))
    with open(source_file, "w") as f:
        f.write(source_template.format(**variables))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: {} CppClassName\n\nGenerates an analysis class with name"
        " CppClass in the current directory with header and source"
        " files.\n".format(sys.argv[0])
    genClass(sys.argv[1])
