#include <iostream>
#include <algorithm>

#include "program_options.h"

using namespace std;

ProgramOptions::ProgramOptions(int argc, char** argv, bool windowsStyle)
	: _argc(argc), _argv(argv), _optionPrefix(windowsStyle ? "/" : "--"), _optionSuffix(windowsStyle ? ":" : "=") {
}

ProgramOptions::~ProgramOptions() {
}

ProgramOptions& ProgramOptions::append(const OptionDefinition& optionDefinition) {
	_definition.insert(make_pair(optionDefinition._key, optionDefinition));
	if (!optionDefinition._optional) {
		_requiredOptions.insert(optionDefinition._key);
	}
	return *this;
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, long defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_LONG, optional, str.str()));
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, unsigned long defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_ULONG, optional, str.str()));
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, unsigned long long defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_ULONGLONG, optional, str.str()));
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, bool defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_BOOLEAN, optional, str.str()));
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, double defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_DOUBLE, optional, str.str()));
}

ProgramOptions& ProgramOptions::append(const string& key, bool optional, const string& defaultValue) {
	stringstream str;
	str << defaultValue;
	return append(OptionDefinition(key, ODVT_STRING, optional, str.str()));
}

bool ProgramOptions::parse(bool exitOnFail) {
	cout << "Parsing helper" << endl;
	
	bool status = true;
	set<string> parsedKeys;
	for (int i = _argc; i < _argc && status; ++i) {
	}

	// Allocate enought memory for the set_difference to insert into this vector
	vector<string> argDiff;
	argDiff.resize(max(_requiredOptions.size(), parsedKeys.size()));
	set_difference(_requiredOptions.begin(), _requiredOptions.end(), parsedKeys.begin(), parsedKeys.end(), argDiff.begin());
	if (argDiff.size() > 0) {
		status = false;
	}

	if (!status) {
		_printHelp("");
	}

	cout << "Completed parsing [parsedArgs: " << _parsedOptions.size() << ", residualArgs: " << _residualArgs.size()
		<< ", requiredArgs: " << _requiredOptions.size() << ", parseStatus: " << status << "]" << endl;
	return status;
}

void ProgramOptions::_printHelp(const string& error) {
	cout << "Print Help" << endl;
}