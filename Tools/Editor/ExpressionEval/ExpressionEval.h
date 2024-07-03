#pragma once

#ifdef EXPORTS_ExpressionEval1D
#    define API_ExpressionEval1D __declspec(dllexport)
#else
#    define API_ExpressionEval1D __declspec(dllimport)
#endif

class API_ExpressionEval1D ExpressionEval1D
{
public:
	static ExpressionEval1D* New(); 
	static void Delete(ExpressionEval1D* p);

	virtual ~ExpressionEval1D();

	virtual bool SetExpression(const char* exprStr) = 0;
	virtual float Test(float x) const = 0;

};