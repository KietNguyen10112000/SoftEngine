#pragma once

#define MAIN_SYSTEM_TASK_BASE(MainSystemClassName, RunnerName, VTypes, ParamVTypesDecl, VTypesAssignment,TASK_SYSTEM_UNPACK_PARAM_REF_NUM, funcBody)	\
{																						\
	auto system = GetGameObject()->GetScene()->Get##MainSystemClassName();				\
	auto taskRunner = system->RunnerName();												\
	using SelfType = std::remove_reference<decltype(*this)>::type;						\
	VTypes																				\
	struct Param																		\
	{																					\
		SelfType* self;																	\
		ParamVTypesDecl																	\
	};																					\
	auto task = taskRunner->CreateTask(													\
		[](MainSystemClassName* system, void* p)										\
		{																				\
			TASK_SYSTEM_UNPACK_PARAM_REF_NUM											\
			funcBody																	\
		}																				\
	);																					\
	auto param = taskRunner->CreateParam<Param>(&task);									\
	param->self = this;																	\
	VTypesAssignment																	\
	taskRunner->RunAsync(this, &task);													\
}

//#define MAIN_SYSTEM_TASK_1(MainSystemClassName, RunnerName, v0, funcBody)				\
//{																						\
//	auto system = GetGameObject()->GetScene()->Get##MainSystemClassName();				\
//	auto taskRunner = system->RunnerName();												\
//	using SelfType = std::remove_reference<decltype(*this)>::type;						\
//	using V0Type = std::decay<decltype(v0)>::type;										\
//	struct Param																		\
//	{																					\
//		SelfType* self;																	\
//		V0Type v0;																		\
//	};																					\
//	auto task = taskRunner->CreateTask(													\
//		[](MainSystemClassName* system, void* p)										\
//		{																				\
//			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, self, v0);							\
//			funcBody																	\
//		}																				\
//	);																					\
//	auto param = taskRunner->CreateParam<Param>(&task);									\
//	param->self = this;																	\
//	param->v0 = v0;																		\
//	taskRunner->RunAsync(this, &task);													\
//}

#define MAIN_SYSTEM_TASK_0(MainSystemClassName, RunnerName, funcBody)				\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
 ,																					\
 ,																					\
 ,																					\
TASK_SYSTEM_UNPACK_PARAM_REF_1(Param, p, self);,									\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_1(MainSystemClassName, RunnerName, v0, funcBody)			\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;,										\
V0Type v0;,																			\
param->v0 = v0;,																	\
TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, self, v0);,								\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_2(MainSystemClassName, RunnerName, v0, v1, funcBody)		\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;,										\
V0Type v0; V1Type v1;,																\
param->v0 = v0; param->v1 = v1;,													\
TASK_SYSTEM_UNPACK_PARAM_REF_3(Param, p, self, v0, v1);,							\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_3(MainSystemClassName, RunnerName, v0, v1, v2, funcBody)	\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;										\
using V2Type = std::decay<decltype(v2)>::type;,										\
V0Type v0; V1Type v1; V2Type v2;,													\
param->v0 = v0; param->v1 = v1; param->v2 = v2;,									\
TASK_SYSTEM_UNPACK_PARAM_REF_4(Param, p, self, v0, v1, v2);,						\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_4(MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)	\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;										\
using V2Type = std::decay<decltype(v2)>::type;										\
using V3Type = std::decay<decltype(v3)>::type;,										\
V0Type v0; V1Type v1; V2Type v2; V3Type v3;,										\
param->v0 = v0; param->v1 = v1; param->v2 = v2; param->v3 = v3;,					\
TASK_SYSTEM_UNPACK_PARAM_REF_5(Param, p, self, v0, v1, v2, v3);,					\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_5(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)	\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;										\
using V2Type = std::decay<decltype(v2)>::type;										\
using V3Type = std::decay<decltype(v3)>::type;										\
using V4Type = std::decay<decltype(v4)>::type;,										\
V0Type v0; V1Type v1; V2Type v2; V3Type v3; V4Type v4;,								\
param->v0 = v0; param->v1 = v1; param->v2 = v2; param->v3 = v3; param->v4 = v4;,	\
TASK_SYSTEM_UNPACK_PARAM_REF_6(Param, p, self, v0, v1, v2, v3, v4);,				\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_6(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)	\
MAIN_SYSTEM_TASK_BASE(																\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;										\
using V2Type = std::decay<decltype(v2)>::type;										\
using V3Type = std::decay<decltype(v3)>::type;										\
using V4Type = std::decay<decltype(v4)>::type;										\
using V5Type = std::decay<decltype(v5)>::type;,										\
V0Type v0; V1Type v1; V2Type v2; V3Type v3; V4Type v4; V5Type v5;,					\
param->v0 = v0; param->v1 = v1; param->v2 = v2; param->v3 = v3; param->v4 = v4; param->v5 = v5;,	\
TASK_SYSTEM_UNPACK_PARAM_REF_7(Param, p, self, v0, v1, v2, v3, v4, v5);,			\
funcBody																			\
)