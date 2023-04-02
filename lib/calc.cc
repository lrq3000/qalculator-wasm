#include <emscripten/bind.h>
#include <libqalculate/Calculator.h>
#include <libqalculate/Variable.h>

using namespace std;
using namespace emscripten;

Calculator calc;
EvaluationOptions evalops;
PrintOptions printops;

#include "settings.h"

struct Calculation
{
	string input;
	string output;
	string messages;
};

Calculation calculate(string calculation, int timeout = 500, int optionFlags = 0)
{
	calculator->clearMessages();

	calculation = calc.unlocalizeExpression(calculation, evalops.parse_options);
	string parsed_str;
	bool resultIsComparison;
	auto result = calc.calculateAndPrint(calculation, timeout, evalops, printops, AUTOMATIC_FRACTION_AUTO, AUTOMATIC_APPROXIMATION_AUTO, &parsed_str, -1, &resultIsComparison, true, 2, TAG_TYPE_HTML);

	Calculation ret;
	ret.input = parsed_str;
	ret.output = result;
	CalculatorMessage *message;
	if ((message = calculator->message()))
	{
		auto msgType = message->type();
		string severity = msgType == MESSAGE_INFORMATION ? "Info" : msgType == MESSAGE_WARNING ? "Warning"
																									: "Error";
		ret.messages += severity + ": " + message->message() + "\n";
	}

	return ret;
}

struct VariableInfo
{
	string name;
	string description;
	string aliases;
};

vector<VariableInfo> getVariables()
{
	vector<VariableInfo> variables;
	for (auto &variable : calc.variables)
	{
		if (!variable->isKnown() || variable->isHidden())
			continue;

		VariableInfo info;
		info.name = variable->preferredDisplayName(true, true).name;
		info.description = variable->title(false, true);
		auto nameCount = variable->countNames();
		if (nameCount < 1)
		{
			info.aliases = variable->preferredDisplayName(true, true).name;
		}
		else
		{
			for (size_t i = 1; i <= nameCount; i++)
			{
				info.aliases += variable->getName(i).name;
				if (i < nameCount)
					info.aliases += "\t";
			}
		}
		variables.push_back(info);
	}
	return variables;
}

void setVariables(string variables)
{
	calc.resetVariables();
	// to add
}

int main()
{
	calc.loadGlobalDefinitions();
	printops.use_unicode_signs = true;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	printops.base_display = BASE_DISPLAY_NORMAL;
	printops.digit_grouping = DIGIT_GROUPING_STANDARD;
	printops.indicate_infinite_series = true;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.limit_implicit_multiplication = true;
	return 0;
}

string info()
{
	return "libqalculate by Hanna Knutsson, compiled by Stephan Troyer";
}

int version()
{
	return 3;
}

EMSCRIPTEN_BINDINGS(Calculator)
{
	function("calculate", &calculate);
	function("info", &info);
	function("version", &version);
	function("getVariables", &getVariables);
	function("set_option", &set_option);
}

EMSCRIPTEN_BINDINGS(calculation)
{
	class_<Calculation>("Calculation")
		.constructor<>()
		.property("input", &Calculation::input)
		.property("output", &Calculation::output)
		.property("messages", &Calculation::messages);
}

EMSCRIPTEN_BINDINGS(variableInfo)
{
	class_<VariableInfo>("VariableInfo")
		.constructor<>()
		.property("name", &VariableInfo::name)
		.property("description", &VariableInfo::description)
		.property("aliases", &VariableInfo::aliases);
	register_vector<VariableInfo>("vector<VariableInfo>");
}
