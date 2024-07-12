#pragma once

#define MAIN_SYSTEM_TASK_EXT_BASE_0(_system, mainComponent, MainSystemClassName, RunnerName, VTypes, ParamVTypesDecl, VTypesAssignment,TASK_SYSTEM_UNPACK_PARAM_REF_NUM, funcBody)	\
{																						\
	auto system = _system;																\
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
	taskRunner->RunAsync(mainComponent, &task);											\
}

#define MAIN_SYSTEM_TASK_EXT_BASE(mainComponent, MainSystemClassName, RunnerName, VTypes, ParamVTypesDecl, VTypesAssignment,TASK_SYSTEM_UNPACK_PARAM_REF_NUM, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(mainComponent->GetCommittedObject()->GetCommittedScene()->Get##MainSystemClassName(),										\
mainComponent, MainSystemClassName, RunnerName, VTypes, ParamVTypesDecl, VTypesAssignment,TASK_SYSTEM_UNPACK_PARAM_REF_NUM, funcBody)

#define MAIN_SYSTEM_TASK_IMPL_0(mainComponent, MainSystemClassName, RunnerName, funcBody)				\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
MainSystemClassName,																\
RunnerName,																			\
 ,																					\
 ,																					\
 ,																					\
TASK_SYSTEM_UNPACK_PARAM_REF_1(Param, p, self);,									\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_IMPL_1(mainComponent, MainSystemClassName, RunnerName, v0, funcBody)			\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;,										\
V0Type v0;,																			\
param->v0 = v0;,																	\
TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, self, v0);,								\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_IMPL_2(mainComponent, MainSystemClassName, RunnerName, v0, v1, funcBody)		\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;,										\
V0Type v0; V1Type v1;,																\
param->v0 = v0; param->v1 = v1;,													\
TASK_SYSTEM_UNPACK_PARAM_REF_3(Param, p, self, v0, v1);,							\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_IMPL_3(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
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

#define MAIN_SYSTEM_TASK_IMPL_4(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
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

#define MAIN_SYSTEM_TASK_IMPL_5(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
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

#define MAIN_SYSTEM_TASK_IMPL_6(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE(mainComponent,											\
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


#define _MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)														\
auto self = this;																									\
if (!mainComponent->GetCommittedObject() || !mainComponent->GetCommittedObject()->IsInAnyScene())					\
{																													\
	funcBody;																										\
	return;																											\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_0(mainComponent, MainSystemClassName, RunnerName, funcBody)							\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_0(mainComponent, MainSystemClassName, RunnerName, funcBody)											\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_1(mainComponent, MainSystemClassName, RunnerName, v0, funcBody)						\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_1(mainComponent, MainSystemClassName, RunnerName, v0, funcBody)										\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_2(mainComponent, MainSystemClassName, RunnerName, v0, v1, funcBody)					\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_2(mainComponent, MainSystemClassName, RunnerName, v0, v1, funcBody)									\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_3(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)				\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_3(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)								\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_4(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)			\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_4(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)							\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_5(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)		\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_5(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)						\
}

#define MAIN_SYSTEM_TASK_IMPL_COMMON_6(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)	\
{																															\
_MAIN_SYSTEM_TASK_DIRECT_IMPL_(mainComponent, funcBody)																		\
MAIN_SYSTEM_TASK_IMPL_6(mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)					\
}

// =====================================================================================================
// =====================================================================================================
// =====================================================================================================
// =====================================================================================================
// for use of class is a kind of MainComponent

#define MAIN_SYSTEM_TASK_0(MainSystemClassName, RunnerName, funcBody)									\
MAIN_SYSTEM_TASK_IMPL_0(this, MainSystemClassName, RunnerName, funcBody)

#define MAIN_SYSTEM_TASK_1(MainSystemClassName, RunnerName, v0, funcBody)								\
MAIN_SYSTEM_TASK_IMPL_1(this, MainSystemClassName, RunnerName, v0, funcBody)

#define MAIN_SYSTEM_TASK_2(MainSystemClassName, RunnerName, v0, v1, funcBody)							\
MAIN_SYSTEM_TASK_IMPL_2(this, MainSystemClassName, RunnerName, v0, v1, funcBody)

#define MAIN_SYSTEM_TASK_3(MainSystemClassName, RunnerName, v0, v1, v2, funcBody)						\
MAIN_SYSTEM_TASK_IMPL_3(this, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)

#define MAIN_SYSTEM_TASK_4(MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)					\
MAIN_SYSTEM_TASK_IMPL_4(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)

#define MAIN_SYSTEM_TASK_5(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)				\
MAIN_SYSTEM_TASK_IMPL_5(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)

#define MAIN_SYSTEM_TASK_6(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)			\
MAIN_SYSTEM_TASK_IMPL_6(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)

// ===========================================================================================================================
#define MAIN_SYSTEM_TASK_COMMON_0(MainSystemClassName, RunnerName, funcBody)									\
MAIN_SYSTEM_TASK_IMPL_COMMON_0(this, MainSystemClassName, RunnerName, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_1(MainSystemClassName, RunnerName, v0, funcBody)								\
MAIN_SYSTEM_TASK_IMPL_COMMON_1(this, MainSystemClassName, RunnerName, v0, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_2(MainSystemClassName, RunnerName, v0, v1, funcBody)							\
MAIN_SYSTEM_TASK_IMPL_COMMON_2(this, MainSystemClassName, RunnerName, v0, v1, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_3(MainSystemClassName, RunnerName, v0, v1, v2, funcBody)						\
MAIN_SYSTEM_TASK_IMPL_COMMON_3(this, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_4(MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)					\
MAIN_SYSTEM_TASK_IMPL_COMMON_4(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_5(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)				\
MAIN_SYSTEM_TASK_IMPL_COMMON_5(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)

#define MAIN_SYSTEM_TASK_COMMON_6(MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)			\
MAIN_SYSTEM_TASK_IMPL_COMMON_6(this, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)



// ========================================================================================================================
// for use of class isn't a kind of MainComponent
//GetCommittedObject()->GetCommittedScene()->Get##MainSystemClassName();
//=========================================================================================================================

#define MAIN_SYSTEM_TASK_EXT_0(system, mainComponent, MainSystemClassName, RunnerName, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
MainSystemClassName,																\
RunnerName,																			\
 ,																					\
 ,																					\
 ,																					\
TASK_SYSTEM_UNPACK_PARAM_REF_1(Param, p, self);,									\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_EXT_1(system, mainComponent, MainSystemClassName, RunnerName, v0, funcBody)			\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;,										\
V0Type v0;,																			\
param->v0 = v0;,																	\
TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, self, v0);,								\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_EXT_2(system, mainComponent, MainSystemClassName, RunnerName, v0, v1, funcBody)		\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
MainSystemClassName,																\
RunnerName,																			\
using V0Type = std::decay<decltype(v0)>::type;										\
using V1Type = std::decay<decltype(v1)>::type;,										\
V0Type v0; V1Type v1;,																\
param->v0 = v0; param->v1 = v1;,													\
TASK_SYSTEM_UNPACK_PARAM_REF_3(Param, p, self, v0, v1);,							\
funcBody																			\
)

#define MAIN_SYSTEM_TASK_EXT_3(system, mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
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

#define MAIN_SYSTEM_TASK_EXT_4(system, mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
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

#define MAIN_SYSTEM_TASK_EXT_5(system, mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
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

#define MAIN_SYSTEM_TASK_EXT_6(system, mainComponent, MainSystemClassName, RunnerName, v0, v1, v2, v3, v4, v5, funcBody)	\
MAIN_SYSTEM_TASK_EXT_BASE_0(system, mainComponent,									\
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