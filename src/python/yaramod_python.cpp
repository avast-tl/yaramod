/**
 * @file src/python/yaramod_python.cpp
 * @brief Implementation of yaramod python bindings.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <algorithm>
#include <iterator>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <yaramod/builder/yara_expression_builder.h>
#include <yaramod/builder/yara_hex_string_builder.h>
#include <yaramod/builder/yara_rule_builder.h>
#include <yaramod/types/plain_string.h>
#include <yaramod/types/token_type.h>
#include <yaramod/yaramod.h>

#include "yaramod_python.h"
#include "py_visitor.h"

namespace py = pybind11;
using namespace py::literals;

using namespace yaramod;

template <typename ExprType, typename ParentType = Expression>
decltype(auto) exprClass(py::module& module, const std::string& name)
{
	return py::class_<ExprType, ParentType, std::shared_ptr<ExprType>>(module, name.c_str());
}

template <typename ExprType>
decltype(auto) unaryOpClass(py::module& module, const std::string& name)
{
	return exprClass<ExprType, UnaryOpExpression>(module, name);
}

template <typename ExprType>
decltype(auto) keywordClass(py::module& module, const std::string& name)
{
	return exprClass<ExprType, KeywordExpression>(module, name);
}

template <typename ExprType>
decltype(auto) binaryOpClass(py::module& module, const std::string& name)
{
	return exprClass<ExprType, BinaryOpExpression>(module, name);
}

void addVersionVariables(py::module& module)
{
	module.attr("YARAMOD_VERSION_MAJOR") = YARAMOD_VERSION_MAJOR;
	module.attr("YARAMOD_VERSION_MINOR") = YARAMOD_VERSION_MINOR;
	module.attr("YARAMOD_VERSION_PATCH") = YARAMOD_VERSION_PATCH;
	module.attr("YARAMOD_VERSION") = YARAMOD_VERSION;
	module.attr("YARA_SYNTAX_VERSION") = YARA_SYNTAX_VERSION;
}

void addEnums(py::module& module)
{
	py::enum_<ParserMode>(module, "ParserMode")
		.value("Regular", ParserMode::Regular)
		.value("IncludeGuarded", ParserMode::IncludeGuarded);

	py::enum_<IntMultiplier>(module, "IntMultiplier")
		.value("Empty", IntMultiplier::None)
		.value("Kilobytes", IntMultiplier::Kilobytes)
		.value("Megabytes", IntMultiplier::Megabytes);

	py::enum_<Rule::Modifier>(module, "RuleModifier")
		.value("Empty", Rule::Modifier::None)
		.value("Global", Rule::Modifier::Global)
		.value("Private", Rule::Modifier::Private)
		.value("PrivateGlobal", Rule::Modifier::PrivateGlobal);

	py::enum_<String::Type>(module, "StringType")
		.value("Plain", String::Type::Plain)
		.value("Hex", String::Type::Hex)
		.value("Regexp", String::Type::Regexp);

	py::enum_<StringModifier::Type>(module, "StringModifierType")
		.value("Ascii", StringModifier::Type::Ascii)
		.value("Wide", StringModifier::Type::Wide)
		.value("Fullword", StringModifier::Type::Fullword)
		.value("Nocase", StringModifier::Type::Nocase)
		.value("Private", StringModifier::Type::Private)
		.value("Xor", StringModifier::Type::Xor);

	py::enum_<Expression::Type>(module, "ExpressionType")
		.value("Undefined", Expression::Type::Undefined)
		.value("Bool", Expression::Type::Bool)
		.value("Int", Expression::Type::Int)
		.value("String", Expression::Type::String)
		.value("Regexp", Expression::Type::Regexp)
		.value("Object", Expression::Type::Object)
		.value("Float", Expression::Type::Float);

	py::enum_<VisitAction>(module, "VisitAction")
		.value("Delete", VisitAction::Delete);

	py::enum_<yaramod::TokenType>(module, "TokenType")
		.value("RuleName", TokenType::RULE_NAME)
		.value("Tag", TokenType::TAG)
		.value("HexAlt", TokenType::HEX_ALT)
		.value("HexNibble", TokenType::HEX_NIBBLE)
		.value("HexWildcard", TokenType::HEX_WILDCARD)
		.value("HexWildcardLow", TokenType::HEX_WILDCARD_LOW)
		.value("HexWildcardHigh", TokenType::HEX_WILDCARD_HIGH)
		.value("HexJumpLeftBracket", TokenType::HEX_JUMP_LEFT_BRACKET)
		.value("HexJumpRightBracket", TokenType::HEX_JUMP_RIGHT_BRACKET)
		.value("HexAltLeftBracket", TokenType::HEX_ALT_LEFT_BRACKET)
		.value("HexAltRightBracket", TokenType::HEX_ALT_RIGHT_BRACKET)
		.value("HexJumpFixed", TokenType::HEX_JUMP_FIXED)
		.value("HexStartBracket", TokenType::HEX_START_BRACKET)
		.value("HexEndBracket", TokenType::HEX_END_BRACKET)
		.value("NewLine", TokenType::NEW_LINE)
		.value("Meta", TokenType::META)
		.value("Lquote", TokenType::LQUOTE)
		.value("Rquote", TokenType::RQUOTE)
		.value("RuleEnd", TokenType::RULE_END)
		.value("RuleBegin", TokenType::RULE_BEGIN)
		.value("Range", TokenType::RANGE)
		.value("Dot", TokenType::DOT)
		.value("DoubleDot", TokenType::DOUBLE_DOT)
		.value("Lt", TokenType::LT)
		.value("Gt", TokenType::GT)
		.value("Le", TokenType::LE)
		.value("Ge", TokenType::GE)
		.value("Eq", TokenType::EQ)
		.value("Neq", TokenType::NEQ)
		.value("ShiftLeft", TokenType::SHIFT_LEFT)
		.value("ShiftRight", TokenType::SHIFT_RIGHT)
		.value("Minus", TokenType::MINUS)
		.value("Plus", TokenType::PLUS)
		.value("Multiply", TokenType::MULTIPLY)
		.value("Divide", TokenType::DIVIDE)
		.value("Modulo", TokenType::MODULO)
		.value("BitwiseXor", TokenType::BITWISE_XOR)
		.value("BitwiseAnd", TokenType::BITWISE_AND)
		.value("BitwiseOr", TokenType::BITWISE_OR)
		.value("BitwiseNot", TokenType::BITWISE_NOT)
		.value("Lp", TokenType::LP)
		.value("Rp", TokenType::RP)
		.value("Lcb", TokenType::LCB)
		.value("Rcb", TokenType::RCB)
		.value("Assign", TokenType::ASSIGN)
		.value("Colon", TokenType::COLON)
		.value("ColonBeforeNewline", TokenType::COLON_BEFORE_NEWLINE)
		.value("Comma", TokenType::COMMA)
		.value("Private", TokenType::PRIVATE)
		.value("Global", TokenType::GLOBAL)
		.value("None", TokenType::NONE)
		.value("Rule", TokenType::RULE)
		.value("Strings", TokenType::STRINGS)
		.value("Condition", TokenType::CONDITION)
		.value("Ascii", TokenType::ASCII)
		.value("Nocase", TokenType::NOCASE)
		.value("Wide", TokenType::WIDE)
		.value("Fullword", TokenType::FULLWORD)
		.value("PrivateStringModifier", TokenType::PRIVATE_STRING_MODIFIER)
		.value("Xor", TokenType::XOR)
		.value("ImportModule", TokenType::IMPORT_MODULE)
		.value("ImportKeyword", TokenType::IMPORT_KEYWORD)
		.value("Not", TokenType::NOT)
		.value("And", TokenType::AND)
		.value("Or", TokenType::OR)
		.value("All", TokenType::ALL)
		.value("Any", TokenType::ANY)
		.value("Of", TokenType::OF)
		.value("Them", TokenType::THEM)
		.value("For", TokenType::FOR)
		.value("Entrypoint", TokenType::ENTRYPOINT)
		.value("OpAt", TokenType::OP_AT)
		.value("OpIn", TokenType::OP_IN)
		.value("Filesize", TokenType::FILESIZE)
		.value("Contains", TokenType::CONTAINS)
		.value("Matches", TokenType::MATCHES)
		.value("Slash", TokenType::SLASH)
		.value("StringLiteral", TokenType::STRING_LITERAL)
		.value("Integer", TokenType::INTEGER)
		.value("Double", TokenType::DOUBLE)
		.value("StringId", TokenType::STRING_ID)
		.value("StringIdAfterNewline", TokenType::STRING_ID_AFTER_NEWLINE)
		.value("StringIdWildcard", TokenType::STRING_ID_WILDCARD)
		.value("StringLength", TokenType::STRING_LENGTH)
		.value("StringOffset", TokenType::STRING_OFFSET)
		.value("StringCount", TokenType::STRING_COUNT)
		.value("Id", TokenType::ID)
		.value("IntegerFunction", TokenType::INTEGER_FUNCTION)
		.value("Lsqb", TokenType::LSQB)
		.value("Rsqb", TokenType::RSQB)
		.value("Dash", TokenType::DASH)
		.value("RegexpOr", TokenType::REGEXP_OR)
		.value("RegexpIter", TokenType::REGEXP_ITER)
		.value("RegexpPiter", TokenType::REGEXP_PITER)
		.value("RegexpOptional", TokenType::REGEXP_OPTIONAL)
		.value("RegexpStartSlash", TokenType::REGEXP_START_SLASH)
		.value("RegexpEndSlash", TokenType::REGEXP_END_SLASH)
		.value("RegexpChar", TokenType::REGEXP_CHAR)
		.value("RegexpRange", TokenType::REGEXP_RANGE)
		.value("RegexpText", TokenType::REGEXP_TEXT)
		.value("RegexpClassNegative", TokenType::REGEXP_CLASS_NEGATIVE)
		.value("RegexpModifiers", TokenType::REGEXP_MODIFIERS)
		.value("RegexpGreedy", TokenType::REGEXP_GREEDY)
		.value("UnaryMinus", TokenType::UNARY_MINUS)
		.value("MetaKey", TokenType::META_KEY)
		.value("MetaValue", TokenType::META_VALUE)
		.value("StringKey", TokenType::STRING_KEY)
		.value("ValueSymbol", TokenType::VALUE_SYMBOL)
		.value("FunctionSymbol", TokenType::FUNCTION_SYMBOL)
		.value("ArraySymbol", TokenType::ARRAY_SYMBOL)
		.value("DictionarySymbol", TokenType::DICTIONARY_SYMBOL)
		.value("StructureSymbol", TokenType::STRUCTURE_SYMBOL)
		.value("LpEnumeration", TokenType::LP_ENUMERATION)
		.value("RpEnumeration", TokenType::RP_ENUMERATION)
		.value("LpWithSpaceAfter", TokenType::LP_WITH_SPACE_AFTER)
		.value("RpWithSpaceBefore", TokenType::RP_WITH_SPACE_BEFORE)
		.value("LpWithSpaces", TokenType::LP_WITH_SPACES)
		.value("RpWithSpaces", TokenType::RP_WITH_SPACES)
		.value("BoolTrue", TokenType::BOOL_TRUE)
		.value("BoolFalse", TokenType::BOOL_FALSE)
		.value("OnelineComment", TokenType::ONELINE_COMMENT)
		.value("Comment", TokenType::COMMENT)
		.value("IncludeDirective", TokenType::INCLUDE_DIRECTIVE)
		.value("IncludePath", TokenType::INCLUDE_PATH)
		.value("FunctionCallLp", TokenType::FUNCTION_CALL_LP)
		.value("FunctionCallRp", TokenType::FUNCTION_CALL_RP)
		.value("Invalid", TokenType::INVALID);
}

void addBasicClasses(py::module& module)
{
	py::class_<YaraFile>(module, "YaraFile")
		.def_property_readonly("text", &YaraFile::getText)
		.def_property_readonly("rules", &YaraFile::getRules)
		.def_property_readonly("imports", &YaraFile::getImports)
		.def_property_readonly("text_formatted", [](const YaraFile& self) { return self.getTextFormatted(); })
		.def_property_readonly("tokenstream", [](const YaraFile& self) { return self.getTokenStream();} )
		.def("find_symbol", &YaraFile::findSymbol)
		.def("add_rule", [](YaraFile& self, const std::shared_ptr<Rule>& rule) {
				self.addRule(rule, true);
			})
		.def("insert_rule", py::overload_cast<std::size_t, const std::shared_ptr<Rule>&>(&YaraFile::insertRule))
		.def("remove_rules", [](YaraFile& self, const std::function<bool(const std::shared_ptr<Rule>&)>& pred) {
				self.removeRules(pred);
			})
		.def("remove_imports", [](YaraFile& self, const std::function<bool(const std::shared_ptr<Module>&)>& pred) {
				self.removeImports(pred);
			});

	py::class_<Location>(module, "Location")
		.def_property_readonly("file_path", &Location::getFilePath)
		// `line_number` property is deprecated, preferred way is to use `begin.line`
		.def_property_readonly("line_number", [](const Location& self) { return self.begin().getLine(); })
		.def_property_readonly("begin", &Location::begin)
		.def_property_readonly("end", &Location::end)
		.def_property_readonly("text", &Location::getText);

	py::class_<Location::Position>(module, "Position")
		.def_property_readonly("line", &Location::Position::getLine)
		.def_property_readonly("column", &Location::Position::getColumn);

	py::class_<Rule, std::shared_ptr<Rule>>(module, "Rule")
		.def_property_readonly("text", &Rule::getText)
		.def_property("name", &Rule::getName, &Rule::setName)
		.def_property("metas", py::overload_cast<>(&Rule::getMetas), &Rule::setMetas, py::return_value_policy::reference)
		.def_property("tags", &Rule::getTags, &Rule::setTags)
		.def_property("modifier", &Rule::getModifier, &Rule::setModifier)
		.def_property_readonly("strings", &Rule::getStrings, py::return_value_policy::reference)
		.def_property_readonly("is_private", &Rule::isPrivate)
		.def_property_readonly("is_global", &Rule::isGlobal)
		.def_property_readonly("location", &Rule::getLocation)
		.def_property_readonly("symbol", &Rule::getSymbol)
		.def_property_readonly("token_first", [](Rule& self) {
				return *self.getFirstTokenIt();
			})
		.def_property_readonly("token_last", [](Rule& self) {
				return *self.getLastTokenIt();
			})
		.def_property("condition", &Rule::getCondition, &Rule::setCondition)
		.def("add_meta", &Rule::addMeta)
		.def("remove_metas", &Rule::removeMetas)
		.def("remove_string", &Rule::removeString)
		.def("get_meta_with_name", py::overload_cast<const std::string&>(&Rule::getMetaWithName), py::return_value_policy::reference)
		.def("add_tag", &Rule::addTag)
		.def("remove_tags", py::overload_cast<const std::string&>(&Rule::removeTags));

	py::class_<Meta>(module, "Meta")
		.def_property("key", &Meta::getKey, &Meta::setKey)
		.def_property("value", &Meta::getValue, &Meta::setValue);

	py::class_<Literal>(module, "Literal")
		.def(py::init<const std::string&>())
		.def(py::init<bool>())
		.def(py::init<std::int64_t>())
		.def(py::init<std::int64_t, const std::string&>())
		.def(py::init<std::uint64_t>())
		.def(py::init<std::uint64_t, const std::string&>())
		.def(py::init<double>())
		.def(py::init<double, const std::string&>())
		.def(py::init<const std::shared_ptr<Symbol>&>())
		.def_property_readonly("text", [](Literal& self) { return self.getText(); })
		.def_property_readonly("pure_text", &Literal::getPureText)
		.def_property_readonly("is_string", &Literal::isString)
		.def_property_readonly("is_bool", &Literal::isBool)
		.def_property_readonly("is_int", &Literal::isInt)
		.def_property_readonly("is_float", &Literal::isFloat)
		.def_property_readonly("is_symbol", &Literal::isSymbol)
		.def_property_readonly("string", &Literal::getString)
		.def_property_readonly("bool", &Literal::getBool)
		.def_property_readonly("int", &Literal::getInt)
		.def_property_readonly("uint", &Literal::getUInt)
		.def_property_readonly("float", &Literal::getFloat)
		.def_property_readonly("symbol", &Literal::getSymbol);

	py::class_<Module, std::shared_ptr<Module>>(module, "Module")
		.def_property_readonly("name", &Module::getName);

	py::class_<String, std::shared_ptr<String>>(module, "String")
		.def_property_readonly("text", &String::getText)
		.def_property_readonly("pure_text", [](String& self) {
			return py::bytes(self.getPureText());
		 })
		.def_property_readonly("type", &String::getType)
		.def_property_readonly("identifier", &String::getIdentifier)
		.def_property_readonly("is_plain", &String::isPlain)
		.def_property_readonly("is_hex", &String::isHex)
		.def_property_readonly("is_regexp", &String::isRegexp)
		.def_property_readonly("is_ascii", &String::isAscii)
		.def_property_readonly("is_wide", &String::isWide)
		.def_property_readonly("is_fullword", &String::isFullword)
		.def_property_readonly("is_nocase", &String::isNocase)
		.def_property_readonly("is_private", &String::isPrivate)
		.def_property_readonly("is_xor", &String::isXor)
		.def_property_readonly("location", &String::getLocation)
		.def_property_readonly("modifiers_text", &String::getModifiersText)
		.def_property_readonly("token_id", &String::getIdentifierToken)
		.def_property_readonly("token_assign", &String::getAssignToken)
		.def_property_readonly("token_first", [](String& self) {
				return *self.getFirstTokenIt();
			})
		.def_property_readonly("token_last", [](String& self) {
				return *self.getLastTokenIt();
			});

	py::class_<PlainString, String, std::shared_ptr<PlainString>>(module, "PlainString");
	py::class_<HexString, String, std::shared_ptr<HexString>>(module, "HexString");
	py::class_<Regexp, String, std::shared_ptr<Regexp>>(module, "Regexp")
		.def_property("unit",
				&Regexp::getUnit,
				py::overload_cast<const std::shared_ptr<RegexpUnit>&>(&Regexp::setUnit))
		.def_property_readonly("suffix_modifiers", &Regexp::getSuffixModifiers);

	py::class_<StringModifier, std::shared_ptr<StringModifier>>(module, "StringModifier")
		.def_property_readonly("type", &StringModifier::getType)
		.def_property_readonly("name", &StringModifier::getName)
		.def_property_readonly("is_ascii", &StringModifier::isAscii)
		.def_property_readonly("is_wide", &StringModifier::isWide)
		.def_property_readonly("is_fullword", &StringModifier::isFullword)
		.def_property_readonly("is_nocase", &StringModifier::isNocase)
		.def_property_readonly("is_private", &StringModifier::isPrivate)
		.def_property_readonly("is_xor", &StringModifier::isXor)
		.def_property_readonly("text", &StringModifier::getText);

	py::class_<AsciiStringModifier, StringModifier, std::shared_ptr<AsciiStringModifier>>(module, "AsciiStringModifier");
	py::class_<WideStringModifier, StringModifier, std::shared_ptr<WideStringModifier>>(module, "WideStringModifier");
	py::class_<FullwordStringModifier, StringModifier, std::shared_ptr<FullwordStringModifier>>(module, "FullwordStringModifier");
	py::class_<NocaseStringModifier, StringModifier, std::shared_ptr<NocaseStringModifier>>(module, "NocaseStringModifier");
	py::class_<PrivateStringModifier, StringModifier, std::shared_ptr<PrivateStringModifier>>(module, "PrivateStringModifier");
	py::class_<XorStringModifier, StringModifier, std::shared_ptr<XorStringModifier>>(module, "XorStringModifier")
		.def_property_readonly("is_range", &XorStringModifier::isRange)
		.def_property_readonly("is_single_key", &XorStringModifier::isSingleKey);

	py::class_<RegexpUnit, std::shared_ptr<RegexpUnit>>(module, "RegexpUnit")
		.def("accept", &RegexpUnit::accept)
		.def_property_readonly("text", &RegexpUnit::getText);

	py::class_<RegexpClass, RegexpUnit, std::shared_ptr<RegexpClass>>(module, "RegexpClass")
		.def_property("characters", &RegexpClass::getCharacters, &RegexpClass::setCharacters)
		.def_property_readonly("is_negative", &RegexpClass::isNegative);

	py::class_<RegexpText, RegexpUnit, std::shared_ptr<RegexpText>>(module, "RegexpText")
		.def_property_readonly("text", &RegexpText::getText);

	py::class_<RegexpAnyChar, RegexpText, std::shared_ptr<RegexpAnyChar>>(module, "RegexpAnyChar");
	py::class_<RegexpWordChar, RegexpText, std::shared_ptr<RegexpWordChar>>(module, "RegexpWordChar");
	py::class_<RegexpNonWordChar, RegexpText, std::shared_ptr<RegexpNonWordChar>>(module, "RegexpNonWordChar");
	py::class_<RegexpSpace, RegexpText, std::shared_ptr<RegexpSpace>>(module, "RegexpSpace");
	py::class_<RegexpNonSpace, RegexpText, std::shared_ptr<RegexpNonSpace>>(module, "RegexpNonSpace");
	py::class_<RegexpDigit, RegexpText, std::shared_ptr<RegexpDigit>>(module, "RegexpDigit");
	py::class_<RegexpNonDigit, RegexpText, std::shared_ptr<RegexpNonDigit>>(module, "RegexpNonDigit");
	py::class_<RegexpWordBoundary, RegexpText, std::shared_ptr<RegexpWordBoundary>>(module, "RegexpWordBoundary");
	py::class_<RegexpNonWordBoundary, RegexpText, std::shared_ptr<RegexpNonWordBoundary>>(module, "RegexpNonWordBoundary");
	py::class_<RegexpStartOfLine, RegexpText, std::shared_ptr<RegexpStartOfLine>>(module, "RegexpStartOfLine");
	py::class_<RegexpEndOfLine, RegexpText, std::shared_ptr<RegexpEndOfLine>>(module, "RegexpEndOfLine");

	py::class_<RegexpOperation, RegexpUnit, std::shared_ptr<RegexpOperation>>(module, "RegexpOperation")
		.def_property_readonly("operand", &RegexpOperation::getOperand)
		.def_property_readonly("operation", &RegexpOperation::getOperation)
		.def_property_readonly("is_greedy", &RegexpOperation::isGreedy);

	py::class_<RegexpIteration, RegexpOperation, std::shared_ptr<RegexpIteration>>(module, "RegexpIteration");
	py::class_<RegexpPositiveIteration, RegexpOperation, std::shared_ptr<RegexpPositiveIteration>>(module, "RegexpPositiveIteration");
	py::class_<RegexpOptional, RegexpOperation, std::shared_ptr<RegexpOptional>>(module, "RegexpOptional");
	py::class_<RegexpRange, RegexpOperation, std::shared_ptr<RegexpRange>>(module, "RegexpRange")
		.def_property_readonly("range", &RegexpRange::getRange);

	py::class_<RegexpConcat, RegexpUnit, std::shared_ptr<RegexpConcat>>(module, "RegexpConcat")
		.def_property("units",
				&RegexpConcat::getUnits,
				py::overload_cast<const std::vector<std::shared_ptr<RegexpUnit>>&>(&RegexpConcat::setUnits));

	py::class_<RegexpGroup, RegexpUnit, std::shared_ptr<RegexpGroup>>(module, "RegexpGroup")
		.def_property_readonly("unit", &RegexpGroup::getUnit);

	py::class_<RegexpOr, RegexpUnit, std::shared_ptr<RegexpOr>>(module, "RegexpOr")
		.def_property_readonly("left", &RegexpOr::getLeft)
		.def_property_readonly("right", &RegexpOr::getRight);

	py::class_<Symbol, std::shared_ptr<Symbol>>(module, "Symbol")
		.def_property_readonly("name", &Symbol::getName)
		.def_property_readonly("data_type", &Symbol::getDataType)
		.def_property_readonly("is_value", &Symbol::isValue)
		.def_property_readonly("is_array", &Symbol::isArray)
		.def_property_readonly("is_dictionary", &Symbol::isDictionary)
		.def_property_readonly("is_function", &Symbol::isFunction)
		.def_property_readonly("is_structure", &Symbol::isStructure);

	py::class_<ValueSymbol, Symbol, std::shared_ptr<ValueSymbol>>(module, "ValueSymbol");
	py::class_<ArraySymbol, Symbol, std::shared_ptr<ArraySymbol>>(module, "ArraySymbol");
	py::class_<DictionarySymbol, Symbol, std::shared_ptr<DictionarySymbol>>(module, "DictionarySymbol");
	py::class_<FunctionSymbol, Symbol, std::shared_ptr<FunctionSymbol>>(module, "FunctionSymbol");
	py::class_<StructureSymbol, Symbol, std::shared_ptr<StructureSymbol>>(module, "StructureSymbol")
		.def("get_attribute", [](const StructureSymbol& self, const std::string& name) {
				return self.getAttribute(name).value_or(nullptr);
			});
}

void addTokenStreamClass(py::module& module)
{
	py::class_<Token>(module, "Token")
		.def(py::init<yaramod::TokenType, const Literal&>())
		.def_property_readonly("text", [](Token& self) { return self.getText(); })
		.def_property_readonly("pure_text", &Token::getPureText)
		.def_property_readonly("is_string", &Token::isString)
		.def_property_readonly("is_bool", &Token::isBool)
		.def_property_readonly("is_int", &Token::isInt)
		.def_property_readonly("is_float", &Token::isFloat)
		.def_property_readonly("is_symbol", &Token::isSymbol)
		.def_property_readonly("type", &Token::getType)
		.def_property_readonly("literal", &Token::getLiteral)
		.def_property_readonly("location", &Token::getLocation)
		.def_property_readonly("string", &Token::getString)
		.def_property_readonly("bool", &Token::getBool)
		.def_property_readonly("int", &Token::getInt)
		.def_property_readonly("uint", &Token::getUInt)
		.def_property_readonly("float", &Token::getFloat)
		.def_property_readonly("symbol", &Token::getSymbol)
		.def_property_readonly("literal_reference", &Token::getLiteralReference);

	py::class_<TokenStream, std::shared_ptr<TokenStream>>(module, "TokenStream")
		.def(py::init<>())
		.def_property_readonly("empty", &TokenStream::empty)
		.def_property_readonly("size", &TokenStream::size)
		.def_property_readonly("front", &TokenStream::front)
		.def_property_readonly("back", &TokenStream::back)
		.def_property_readonly("tokens", &TokenStream::getTokens)
		.def_property_readonly("tokens_as_text", &TokenStream::getTokensAsText);
}

void addExpressionClasses(py::module& module)
{
	py::class_<Expression, std::shared_ptr<Expression>>(module, "Expression")
		.def("accept", &Expression::accept)
		.def("get_text", &Expression::getText, py::arg("indent") = std::string{})
		.def("exchange_tokens", py::overload_cast<Expression*>(&Expression::exchangeTokens))
		.def_property_readonly("text",
				// getText() has default parameter and Python can't deal with it
				[](const Expression* self) {
					return self->getText();
				})
		.def_property_readonly("tokenstream", [](const Expression& self) { return self.getTokenStream();} );

	exprClass<StringExpression>(module, "StringExpression")
		.def_property("id",
				&StringExpression::getId,
				py::overload_cast<const std::string&>(&StringExpression::setId));
	exprClass<StringWildcardExpression>(module, "StringWildcardExpression")
		.def_property("id",
				&StringWildcardExpression::getId,
				py::overload_cast<const std::string&>(&StringWildcardExpression::setId));
	exprClass<StringAtExpression>(module, "StringAtExpression")
		.def_property("id",
				&StringAtExpression::getId,
				py::overload_cast<const std::string&>(&StringAtExpression::setId))
		.def_property("at_expr",
				&StringAtExpression::getAtExpression,
				py::overload_cast<const Expression::Ptr&>(&StringAtExpression::setAtExpression));
	exprClass<StringInRangeExpression>(module, "StringInRangeExpression")
		.def_property("id",
				&StringInRangeExpression::getId,
				py::overload_cast<const std::string&>(&StringInRangeExpression::setId))
		.def_property("range_expr",
				&StringInRangeExpression::getRangeExpression,
				py::overload_cast<const Expression::Ptr&>(&StringInRangeExpression::setRangeExpression));
	exprClass<StringCountExpression>(module, "StringCountExpression")
		.def_property("id",
				&StringCountExpression::getId,
				py::overload_cast<const std::string&>(&StringCountExpression::setId));
	exprClass<StringOffsetExpression>(module, "StringOffsetExpression")
		.def_property("id",
				&StringOffsetExpression::getId,
				py::overload_cast<const std::string&>(&StringOffsetExpression::setId))
		.def_property("index_expr",
				&StringOffsetExpression::getIndexExpression,
				py::overload_cast<const Expression::Ptr&>(&StringOffsetExpression::setIndexExpression));
	exprClass<StringLengthExpression>(module, "StringLengthExpression")
		.def_property("id",
				&StringLengthExpression::getId,
				py::overload_cast<const std::string&>(&StringLengthExpression::setId))
		.def_property("index_expr",
				&StringLengthExpression::getIndexExpression,
				py::overload_cast<const Expression::Ptr&>(&StringLengthExpression::setIndexExpression));

	exprClass<UnaryOpExpression>(module, "UnaryOpExpression")
		.def_property("operand",
				&UnaryOpExpression::getOperand,
				py::overload_cast<const Expression::Ptr&>(&UnaryOpExpression::setOperand));
	unaryOpClass<NotExpression>(module, "NotExpression");
	unaryOpClass<UnaryMinusExpression>(module, "UnaryMinusExpression");
	unaryOpClass<BitwiseNotExpression>(module, "BitwiseNotExpression");

	exprClass<BinaryOpExpression>(module, "BinaryOpExpression")
		.def_property("left_operand",
				&BinaryOpExpression::getLeftOperand,
				py::overload_cast<const Expression::Ptr&>(&BinaryOpExpression::setLeftOperand))
		.def_property("right_operand",
				&BinaryOpExpression::getRightOperand,
				py::overload_cast<const Expression::Ptr&>(&BinaryOpExpression::setRightOperand));
	binaryOpClass<AndExpression>(module, "AndExpression");
	binaryOpClass<OrExpression>(module, "OrExpression");
	binaryOpClass<LtExpression>(module, "LtExpression");
	binaryOpClass<GtExpression>(module, "GtExpression");
	binaryOpClass<LeExpression>(module, "LeExpression");
	binaryOpClass<GeExpression>(module, "GeExpression");
	binaryOpClass<EqExpression>(module, "EqExpression");
	binaryOpClass<NeqExpression>(module, "NeqExpression");
	binaryOpClass<ContainsExpression>(module, "ContainsExpression");
	binaryOpClass<MatchesExpression>(module, "MatchesExpression");
	binaryOpClass<PlusExpression>(module, "PlusExpression");
	binaryOpClass<MinusExpression>(module, "MinusExpression");
	binaryOpClass<MultiplyExpression>(module, "MultiplyExpression");
	binaryOpClass<DivideExpression>(module, "DivideExpression");
	binaryOpClass<ModuloExpression>(module, "ModuloExpression");
	binaryOpClass<BitwiseXorExpression>(module, "BitwiseXorExpression");
	binaryOpClass<BitwiseAndExpression>(module, "BitwiseAndExpression");
	binaryOpClass<BitwiseOrExpression>(module, "BitwiseOrExpression");
	binaryOpClass<ShiftLeftExpression>(module, "ShiftLeftExpression");
	binaryOpClass<ShiftRightExpression>(module, "ShiftRightExpression");

	exprClass<ForExpression>(module, "ForExpression")
		.def_property("variable",
				&ForExpression::getVariable,
				py::overload_cast<const Expression::Ptr&>(&ForExpression::setVariable))
		.def_property("iterated_set",
				&ForExpression::getIteratedSet,
				py::overload_cast<const Expression::Ptr&>(&ForExpression::setIteratedSet))
		.def_property("body",
				&ForExpression::getBody,
				py::overload_cast<const Expression::Ptr&>(&ForExpression::setBody));
	exprClass<ForIntExpression, ForExpression>(module, "ForIntExpression")
		.def_property("id",
				&ForIntExpression::getId,
				py::overload_cast<const std::string&>(&ForIntExpression::setId));
	exprClass<ForStringExpression, ForExpression>(module, "ForStringExpression");
	exprClass<OfExpression, ForExpression>(module, "OfExpression");

	exprClass<SetExpression>(module, "SetExpression")
		.def_property("elements",
				&SetExpression::getElements,
				py::overload_cast<const std::vector<Expression::Ptr>&>(&SetExpression::setElements));
	exprClass<RangeExpression>(module, "RangeExpression")
		.def_property("low",
				&RangeExpression::getLow,
				py::overload_cast<const Expression::Ptr&>(&RangeExpression::setLow))
		.def_property("high",
				&RangeExpression::getHigh,
				py::overload_cast<const Expression::Ptr&>(&RangeExpression::setHigh));
	exprClass<IdExpression>(module, "IdExpression")
		.def_property("symbol",
				&IdExpression::getSymbol,
				&IdExpression::setSymbol);
	exprClass<StructAccessExpression, IdExpression>(module, "StructAccessExpression")
		.def_property("structure",
				&StructAccessExpression::getStructure,
				py::overload_cast<const Expression::Ptr&>(&StructAccessExpression::setStructure));
	exprClass<ArrayAccessExpression, IdExpression>(module, "ArrayAccessExpression")
		.def_property("array",
				&ArrayAccessExpression::getArray,
				py::overload_cast<const Expression::Ptr&>(&ArrayAccessExpression::setArray))
		.def_property("accessor",
				&ArrayAccessExpression::getAccessor,
				py::overload_cast<const Expression::Ptr&>(&ArrayAccessExpression::setAccessor));
	exprClass<FunctionCallExpression, IdExpression>(module, "FunctionCallExpression")
		.def_property("function",
				&FunctionCallExpression::getFunction,
				py::overload_cast<const Expression::Ptr&>(&FunctionCallExpression::setFunction))
		.def_property("arguments",
				&FunctionCallExpression::getArguments,
				py::overload_cast<const std::vector<Expression::Ptr>&>(&FunctionCallExpression::setArguments));

	exprClass<LiteralExpression<bool>>(module, "_BoolLiteralExpression")
		.def_property_readonly("value", &LiteralExpression<bool>::getValue);
	exprClass<LiteralExpression<std::string>>(module, "_StringLiteralExpression")
		.def_property_readonly("value", &LiteralExpression<std::string>::getValue);
	exprClass<LiteralExpression<std::uint64_t>>(module, "_IntLiteralExpression")
		.def_property_readonly("value", &LiteralExpression<uint64_t>::getValue);
	exprClass<LiteralExpression<double>>(module, "_DoubleLiteralExpression")
		.def_property_readonly("value", &LiteralExpression<double>::getValue);

	exprClass<BoolLiteralExpression, LiteralExpression<bool>>(module, "BoolLiteralExpression");
	exprClass<StringLiteralExpression, LiteralExpression<std::string>>(module, "StringLiteralExpression");
	exprClass<IntLiteralExpression, LiteralExpression<std::uint64_t>>(module, "IntLiteralExpression");
	exprClass<DoubleLiteralExpression, LiteralExpression<double>>(module, "DoubleLiteralExpression");

	exprClass<KeywordExpression>(module, "KeywordExpression");
	keywordClass<FilesizeExpression>(module, "FilesizeExpression");
	keywordClass<EntrypointExpression>(module, "EntrypointExpression");
	keywordClass<AllExpression>(module, "AllExpression");
	keywordClass<AnyExpression>(module, "AnyExpression");
	keywordClass<ThemExpression>(module, "ThemExpression");

	exprClass<ParenthesesExpression>(module, "ParenthesesExpression")
		.def_property("enclosed_expr",
				&ParenthesesExpression::getEnclosedExpression,
				py::overload_cast<const Expression::Ptr&>(&ParenthesesExpression::setEnclosedExpression));
	exprClass<IntFunctionExpression>(module, "IntFunctionExpression")
		.def_property("function",
				&IntFunctionExpression::getFunction,
				py::overload_cast<const std::string&>(&IntFunctionExpression::setFunction))
		.def_property("argument",
				&IntFunctionExpression::getArgument,
				py::overload_cast<const Expression::Ptr&>(&IntFunctionExpression::setArgument));
	exprClass<RegexpExpression>(module, "RegexpExpression")
		.def_property("regexp_string",
				&RegexpExpression::getRegexpString,
				py::overload_cast<const std::shared_ptr<String>&>(&RegexpExpression::setRegexpString));
}

void addBuilderClasses(py::module& module)
{
	py::class_<YaraFileBuilder>(module, "YaraFileBuilder")
		.def(py::init<>())
		.def(py::init<const std::string&>())
		.def("get", [](YaraFileBuilder& self, bool recheck) {
				return self.get(recheck, nullptr);
			}, py::arg("recheck") = false)
		.def("with_module", &YaraFileBuilder::withModule)
		.def("with_rule", [](YaraFileBuilder& self, const Rule& rule) {
				return self.withRule(Rule{rule});
			});

	py::class_<YaraRuleBuilder>(module, "YaraRuleBuilder")
		.def(py::init<>())
		.def("get", [](YaraRuleBuilder& self) {
				// We use unique_ptr in YaraRuleBuilder::get() but pybind does not support unique_ptrs in return values
				return std::shared_ptr<Rule>(self.get());
			})
		.def("with_name", &YaraRuleBuilder::withName)
		.def("with_modifier", &YaraRuleBuilder::withModifier)
		.def("with_tag", &YaraRuleBuilder::withTag)
		.def("with_comment", &YaraRuleBuilder::withComment, py::arg("comment"), py::arg("multiline") = true)
		.def("with_string_meta", &YaraRuleBuilder::withStringMeta)
		.def("with_int_meta", &YaraRuleBuilder::withIntMeta)
		.def("with_uint_meta", &YaraRuleBuilder::withUIntMeta)
		.def("with_hex_int_meta", &YaraRuleBuilder::withHexIntMeta)
		.def("with_bool_meta", &YaraRuleBuilder::withBoolMeta)
		.def("with_plain_string", &YaraRuleBuilder::withPlainString, py::arg("id"), py::arg("value"))
		.def("with_hex_string", &YaraRuleBuilder::withHexString)
		.def("with_regexp", &YaraRuleBuilder::withRegexp, py::arg("id"), py::arg("value"), py::arg("suffix_mods") = std::string{})
		.def("with_condition", py::overload_cast<const Expression::Ptr&>(&YaraRuleBuilder::withCondition))
		.def("ascii", &YaraRuleBuilder::ascii)
		.def("wide", &YaraRuleBuilder::wide)
		.def("fullword", &YaraRuleBuilder::fullword)
		.def("nocase", &YaraRuleBuilder::nocase)
		.def("private", &YaraRuleBuilder::private_)
		.def("xor", [](YaraRuleBuilder& self, py::args args) {
			if (args.size() == 0)
				return self.xor_();
			else if (args.size() == 1)
				return self.xor_(args[0].cast<int>());
			else if (args.size() == 2)
				return self.xor_(args[0].cast<int>(), args[1].cast<int>());
			throw std::invalid_argument("xor() expects either 0, 1 or 2 arguments");
		});

	py::class_<YaraExpressionBuilder>(module, "YaraExpressionBuilder")
		.def(py::init<>())
		.def(py::init<const Expression::Ptr&>())
		.def("get", &YaraExpressionBuilder::get)
		.def("__invert__", &YaraExpressionBuilder::operator~)
		.def("__neg__", py::overload_cast<>(&YaraExpressionBuilder::operator-))
		.def("__eq__", &YaraExpressionBuilder::operator==)
		.def("__ne__", &YaraExpressionBuilder::operator!=)
		.def("__lt__", &YaraExpressionBuilder::operator<)
		.def("__gt__", &YaraExpressionBuilder::operator>)
		.def("__le__", &YaraExpressionBuilder::operator<=)
		.def("__ge__", &YaraExpressionBuilder::operator>=)
		.def("__add__", &YaraExpressionBuilder::operator+)
		.def("__sub__", py::overload_cast<const YaraExpressionBuilder&>(&YaraExpressionBuilder::operator-))
		.def("__mul__", &YaraExpressionBuilder::operator*)
		.def("__truediv__", &YaraExpressionBuilder::operator/)
		.def("__mod__", &YaraExpressionBuilder::operator%)
		.def("__xor__", &YaraExpressionBuilder::operator^)
		.def("__and__", &YaraExpressionBuilder::operator&)
		.def("__or__", &YaraExpressionBuilder::operator|)
		.def("__lshift__", &YaraExpressionBuilder::operator<<)
		.def("__rshift__", &YaraExpressionBuilder::operator>>)
		.def("__call__", [](YaraExpressionBuilder& self, py::args args) {
				std::vector<YaraExpressionBuilder> call_args;
				std::transform(args.begin(), args.end(), std::back_inserter(call_args),
						[](const auto& obj) {
							return py::cast<YaraExpressionBuilder>(obj);
						});
				return self.call(call_args);
			})
		.def("__getitem__", &YaraExpressionBuilder::operator[])
		.def("access", &YaraExpressionBuilder::access)
		.def("comment", &YaraExpressionBuilder::comment, py::arg("message"), py::arg("multiline") = false, py::arg("indent") = "")
		.def("contains", &YaraExpressionBuilder::contains)
		.def("matches", &YaraExpressionBuilder::matches)
		.def("read_int8", &YaraExpressionBuilder::readInt8)
		.def("read_int16", &YaraExpressionBuilder::readInt16)
		.def("read_int32", &YaraExpressionBuilder::readInt32)
		.def("read_uint8", &YaraExpressionBuilder::readUInt8)
		.def("read_uint16", &YaraExpressionBuilder::readUInt16)
		.def("read_uint32", &YaraExpressionBuilder::readUInt32);

	module.def("not_", [](YaraExpressionBuilder& exprBuilder) {
				return !exprBuilder;
			});

	module.def("int_val", &intVal, py::arg("value"), py::arg("mult") = IntMultiplier::None);
	module.def("uint_val", &uintVal, py::arg("value"), py::arg("mult") = IntMultiplier::None);
	module.def("hex_int_val", &hexIntVal);
	module.def("double_val", &doubleVal);
	module.def("string_val", &stringVal);
	module.def("bool_val", &boolVal);

	module.def("id", &id);
	module.def("paren", &paren, py::arg("enclosed_expr"), py::arg("linebreaks") = false);

	module.def("string_ref", &stringRef);
	module.def("match_count", py::overload_cast<const std::string&>(&matchCount));
	module.def("match_length", py::overload_cast<const std::string&>(&matchLength));
	module.def("match_offset", py::overload_cast<const std::string&>(&matchOffset));
	module.def("match_length", py::overload_cast<const std::string&, const YaraExpressionBuilder&>(&matchLength));
	module.def("match_offset", py::overload_cast<const std::string&, const YaraExpressionBuilder&>(&matchOffset));
	module.def("match_at", py::overload_cast<const std::string&, const YaraExpressionBuilder&>(&matchAt));
	module.def("match_in_range", py::overload_cast<const std::string&, const YaraExpressionBuilder&>(&matchInRange));

	module.def("for_loop", py::overload_cast<
			const YaraExpressionBuilder&,
			const std::string&,
			const YaraExpressionBuilder&,
			const YaraExpressionBuilder&
		>(&forLoop));
	module.def("for_loop", py::overload_cast<
			const YaraExpressionBuilder&,
			const YaraExpressionBuilder&,
			const YaraExpressionBuilder&
		>(&forLoop));
	module.def("of", &of);

	module.def("set", &set);
	module.def("range", &range);

	module.def("conjunction", py::overload_cast<const std::vector<YaraExpressionBuilder>&, bool>(&conjunction), py::arg("terms"), py::arg("linebreaks") = false);
	module.def("disjunction", py::overload_cast<const std::vector<YaraExpressionBuilder>&, bool>(&disjunction), py::arg("terms"), py::arg("linebreaks") = false);
	module.def("conjunction", py::overload_cast<const std::vector<std::pair<YaraExpressionBuilder, std::string>>&>(&conjunction), py::arg("terms"));
	module.def("disjunction", py::overload_cast<const std::vector<std::pair<YaraExpressionBuilder, std::string>>&>(&disjunction), py::arg("terms"));

	module.def("filesize", &filesize);
	module.def("entrypoint", &entrypoint);
	module.def("all", &all);
	module.def("any", &any);
	module.def("them", &them);

	module.def("regexp", &regexp);

	py::class_<YaraHexStringBuilder>(module, "YaraHexStringBuilder")
		.def(py::init<>())
		.def(py::init<std::uint8_t>())
		.def(py::init<const std::vector<std::uint8_t>&>())
		.def(py::init<const std::shared_ptr<HexStringUnit>&>())
		.def(py::init<const std::vector<std::shared_ptr<HexStringUnit>>&>())
		.def("get", [](YaraHexStringBuilder& self) {
				return self.get();
			})
		.def("add", [](YaraHexStringBuilder& self, const YaraHexStringBuilder& unit) {
				return self.add(unit);
			});

	module.def("wildcard", &wildcard);
	module.def("wildcard_low", &wildcardLow);
	module.def("wildcard_high", &wildcardHigh);

	module.def("jump_varying", &jumpVarying);
	module.def("jump_fixed", &jumpFixed);
	module.def("jump_varying_range", &jumpVaryingRange);
	module.def("jump_range", &jumpRange);

	module.def("alt", &alt<std::vector<YaraHexStringBuilder>>);
}

void addMainClass(py::module& module)
{
	py::class_<Yaramod>(module, "Yaramod")
		.def(py::init<>())
		.def(py::init<const std::string&>())
		.def("parse_file", &Yaramod::parseFile, py::arg("file_path"), py::arg("parser_mode") = ParserMode::Regular)
		.def("parse_string", [](Yaramod& self, const std::string& str, ParserMode parserMode) {
				std::istringstream stream(str);
				return self.parseStream(stream, parserMode);
			}, py::arg("str"), py::arg("parser_mode") = ParserMode::Regular)
		.def_property_readonly("yara_file", &Yaramod::getParsedFile);
}

PYBIND11_MODULE(yaramod, module)
{
	static py::exception<ParserError> exception(module, "ParserError");
	py::register_exception_translator(
		[](std::exception_ptr exPtr) {
			try
			{
				if (exPtr)
					std::rethrow_exception(exPtr);
			}
			catch (const ParserError& err)
			{
				exception(err.what());
			}
		});

	addVersionVariables(module);
	addEnums(module);
	addBasicClasses(module);
	addTokenStreamClass(module);
	addExpressionClasses(module);
	addMainClass(module);
	addVisitorClasses(module);
	addRegexpVisitorClasses(module);
	addBuilderClasses(module);
}
