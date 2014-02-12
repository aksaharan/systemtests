#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>

using namespace std;

class ProgramOptions {
public:
	enum ODV_TYPE {
		ODVT_INVALID,
		ODVT_LONG,
		ODVT_ULONG,
		ODVT_ULONGLONG,
		ODVT_DOUBLE,
		ODVT_BOOLEAN,
		ODVT_STRING
	};

	struct OptionDefinition {
		OptionDefinition(const string& key, ODV_TYPE type, bool optional = true, const string& defaultValue = "")
			: _key(key), _type(type), _optional(optional), _defaultValue(defaultValue) {
		}

		string _key;
		ODV_TYPE _type;
		bool _optional;
		string _defaultValue;
	};

	ProgramOptions(int argc, char** argv, bool windowsStyle = false);
	virtual ~ProgramOptions();

	ProgramOptions& append(const OptionDefinition& optionDefinition);
	ProgramOptions& append(const string& key, bool optional = true, long defaultValue = 0);
	ProgramOptions& append(const string& key, bool optional = true, unsigned long defaultValue = 0);
	ProgramOptions& append(const string& key, bool optional = true, unsigned long long defaultValue = 0);
	ProgramOptions& append(const string& key, bool optional = true, bool defaultValue = false);
	ProgramOptions& append(const string& key, bool optional = true, double defaultValue = 0.0);
	ProgramOptions& append(const string& key, bool optional = true, const string& defaultValue = "");

	const vector<string> residualArgs() {
		return _residualArgs;
	}

	bool parse(bool exitOnFail = true);
	

private:
	int _argc;
	char** _argv;
	std::map<string, OptionDefinition> _definition;
	set<string> _requiredOptions;
	map<string, string> _parsedOptions;
	vector<string> _residualArgs;

	const string _optionPrefix;
	const string _optionSuffix;

	void _printHelp(const string& error);
};