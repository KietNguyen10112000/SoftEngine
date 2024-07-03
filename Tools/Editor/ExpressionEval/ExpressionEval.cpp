#include "pch.h"
#include "ExpressionEval.h"

#include "exprtk/exprtk.hpp"

#include <iostream>

class _ExprtkEval : public ExpressionEval1D
{
public:
	typedef exprtk::symbol_table<float> symbol_table_t;
	typedef exprtk::expression<float>   expression_t;
	typedef exprtk::parser<float>       parser_t;

	symbol_table_t m_symbolTable;
	expression_t m_expression;
	mutable float m_x = 0;

	_ExprtkEval()
	{
		m_symbolTable.add_variable("x", m_x);
		m_symbolTable.add_constants();

		m_expression.register_symbol_table(m_symbolTable);
	}

	// Inherited via ExpressionEval1D
	bool SetExpression(const char* exprStr) override
	{
		parser_t parser;
		if (!parser.compile(exprStr, m_expression))
		{
			std::cerr << "[ERROR]: ExpressionEval1D::SetExpression() failed.\n";
			return false;
		}
		return true;
	}

	float Test(float x) const override
	{
		m_x = x;
		return m_expression.value();
	}

};

ExpressionEval1D* ExpressionEval1D::New()
{
	return new _ExprtkEval();
}

void ExpressionEval1D::Delete(ExpressionEval1D* p)
{
	delete p;
}

ExpressionEval1D::~ExpressionEval1D()
{
}
